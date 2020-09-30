#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <x86intrin.h>

#include "fft.h"
#include "macro.h"
#include "simd_utils.h"

static bool initialized = false;
alignas(32) static complex float omega_vec_log2[MAX_SAMPLES][K];

static inline unsigned int reverse_bits(unsigned int n, unsigned int num_bits) {
    int i, j;
    register unsigned int res = 0;

    i = 0;
    j = num_bits;
    while (i <= j) {
        unsigned int lower_mask = 1 << i;
        unsigned int upper_mask = (1 << num_bits) >> i;
        unsigned int shift = j - i;
        res |= ((n >> shift) & lower_mask) | ((n << shift) & upper_mask);
        i++;
        j--;
    }
    return res;
}

static inline unsigned int get_msb(unsigned int v) {
    return 31 - __builtin_clz(v);
}

void fft_init() {
    for (unsigned int nl = 0; nl < MAX_SAMPLES_LOG_2; nl++) {
        unsigned int n = (1u << nl);
        const double mul_div_n = (-2.0 * M_PI) / n;
        for (unsigned int k = 0; k < K; k++) {
            complex float *res = &omega_vec_log2[nl][k];
            complex double theta = mul_div_n * k;
            _real_(*res) = cosf(theta);
            _imag_(*res) = sinf(theta);
        }
    }
    initialized = true;
}

static _forceinline_ void write_zeros(complex float *ptr, unsigned int count) {
    static_assert(sizeof(complex float) == 8);

    const complex float *ptr_aligned_pow_two = ptr + ( (count & (~1u)) );
    const __m128 zero = _mm_setzero_ps();
    for ( ; ptr < ptr_aligned_pow_two;
            ptr+=2 /* NOTE: the assert guarantees ptr+=2 will add 128 bits */ )
        _mm_storeu_ps((float*)ptr, zero);

    /* Zero the last complex struct if we aren't divisible by 2. */
    if ( (count % 2) != 0)
        store_float2(ptr, zero);
}

static _forceinline_ void fft_run_main_unroll_1(unsigned int N, complex float *output_data) {
    static const unsigned int wingspan = 1;
    for (unsigned int j = 0; j < N; j+=2) {
        complex float *a0 = &output_data[j];
        complex float *a1 = &output_data[j + wingspan];
        fft_mainloop_j1(a0, a1);
    }
}

static _forceinline_ void fft_run_main_unroll_2(unsigned int N, complex float *output_data) {
    static const unsigned int wingspan = 2;
    for (unsigned int j = 0; j < N; j+=2) {
        complex float *a0 = &output_data[j];
        complex float *a1 = &output_data[j + wingspan];
        fft_mainloop_j2(a0, a1);
    }
}

static _forceinline_ void fft_run_main_unroll_4(unsigned int N, complex float *output_data) {
	static const unsigned int wingspan = 4;
	static const unsigned int n = wingspan * 2;	// 8 = 2^3
	// Loading these things into registers outside of the inner loop is what gives the performance win
	const complex float *omega_src = &omega_vec_log2[3][0];
#ifdef __AVX__
	// Note we don't need AVX2 for this part, it only does float math and AVX1 is enough.
	const complex float *omega = omega_src;
#else
	const complex float *omega_low = omega_src;
	const complex float *omega_high = omega_src + 4;
#endif

	for (unsigned int j = 0; j < N; j+=wingspan*2) {
		complex float *a0 = &output_data[j];
		complex float *a1 = &output_data[j + wingspan];
#ifdef __AVX__
		fft_mainloop_x4( omega, a0, a1);
#else
		fft_mainloop_x2(omega_low, a0, a1);
		fft_mainloop_x2(omega_high, a0 + 2, a1 + 2);
#endif
	}
}

static _forceinline_ void fft_run_main_unroll_8(unsigned int N, complex float *output_data) {
	static const unsigned int wingspan = 8;
	static const unsigned int n = wingspan * 2;	// 16 = 2^4
	const complex float *omega_src = &omega_vec_log2[4][0];
#ifdef __AVX__
	const complex float *omega0 = omega_src;
	const complex float *omega1 = omega_src + 8;
#else
	const complex float *omega0 = omega_src;
	const complex float *omega1 = omega_src + 4;
	const complex float *omega2 = omega_src + 8;
	const complex float *omega3 = omega_src + 12;
#endif

	for (unsigned int j = 0; j < N; j+=wingspan*2) {
		complex float *a0 = &output_data[j];
		complex float *a1 = &output_data[j + wingspan];
#ifdef __AVX__
		fft_mainloop_x4(omega0, a0,     a1);
		fft_mainloop_x4(omega1, a0 + 4, a1 + 4);
#else
		fft_mainloop_x2(omega0, a0,     a1);
		fft_mainloop_x2(omega1, a0 + 2, a1 + 2);
		fft_mainloop_x2(omega2, a0 + 4, a1 + 4);
		fft_mainloop_x2(omega3, a0 + 6, a1 + 6);
#endif
	}
}

static _forceinline_ void fft_run_main_n(unsigned int wingspan, unsigned int N, complex float *output_data) {
    /* We compute 8 items per iteration. */
    assert(wingspan >= 8 && (wingspan % 8) == 0);
    const unsigned int n = wingspan * 2;
    const complex float *omega_begin = &omega_vec_log2[get_msb(n)][0];
    const complex float *omega_end = omega_begin + wingspan;

    for (unsigned int j = 0; j < N; j+=wingspan*2) {
        complex float *a0 = &output_data[j];
        complex float *a1 = &output_data[j+wingspan];
        for (const complex float *omega = omega_begin;
                omega < omega_end;
                omega+=8, a0+=8, a1+=8) {
            fft_mainloop_x4(omega, a0, a1);
            fft_mainloop_x4(omega + 4, a0 + 4, a1 + 4);
        }
    }
}

void fft_run(
        const float *input_data,
        complex float *output_data,
        unsigned int N,
        unsigned int channels) {

    assert(initialized);

    {
        unsigned int msb;

        for (unsigned int i = 0, j = 0; i < N; j++, i+=channels)
            /* Taking just the left channel for now... */
            output_data[j] = input_data[i];

        N = N / channels;
        assert(N <= MAX_SAMPLES);
        msb = get_msb(N);

        if (_unlikely_((N & (N-1)))) {
            /* Pad out so FFT is a power of 2. */
            msb++;
            unsigned int new_N = 1 << msb;
            write_zeros(&output_data[N], new_N - N);
            N = new_N;
        }

        /* Reverse the input array. */
        unsigned int hi_bit = msb - 1;
        for (unsigned int i = 0; i < N; i++) {
            unsigned int r = reverse_bits(i, hi_bit);
            if (i < r)
                SWAP(output_data[i], output_data[r]);
        }
    }

    /* Vectorized implementation of the following radix-2 DIT FFT algorithm
     *
    {
        for (unsigned int nl = 1, wingspan = 1;
                wingspan < N;
                nl++, wingspan*=2) {
            for (unsigned int j = 0; j < N; j+=wingspan*2) {
                for (unsigned int k = 0; k < wingspan; k++) {
                    complex float omega = omega_vec_log2[nl][k];
                    complex float a0 = output_data[k+j];
                    complex float a1 = output_data[k+j+wingspan];
                    output_data[k+j] = a0 + omega*a1;
                    output_data[k+j+wingspan] = a0 - omega*a1;
                }
            }
        }
    }
    *
    */

   // wingspan 1
   if (N <= 1) return; fft_run_main_unroll_1(N, output_data);
   // wingspan 2
   if (N <= 2) return; fft_run_main_unroll_2(N, output_data);
   // wingspan 4
   if (N <= 4) return; fft_run_main_unroll_4(N, output_data);
   // wingspan 8
   if (N <= 8) return; fft_run_main_unroll_8(N, output_data);
   // wingspan >= 16
   for (unsigned int wingspan = 16; wingspan < N; wingspan*=2) fft_run_main_n(wingspan, N, output_data);
}
