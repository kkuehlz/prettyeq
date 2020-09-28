#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdalign.h>
#include <string.h>
#include <time.h>

#include "macro.h"
#include "fft.h"

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
            msb = msb + 1;
            unsigned int new_N = 1 << msb;
            for (unsigned int i = N; i < new_N; i++)
                output_data[i] = 0.0f;

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

    {
        /* Simple radix-2 DIT FFT */
        unsigned int wingspan = 1;
        unsigned int nl = 1;
        while (wingspan < N) {
            for (unsigned int j = 0; j < N; j+=wingspan*2) {
                for (unsigned int k = 0; k < wingspan; k++) {
                    complex float omega = omega_vec_log2[nl][k];
                    complex float a0 = output_data[k+j];
                    complex float a1 = output_data[k+j+wingspan];
                    output_data[k+j] = a0 + omega*a1;
                    output_data[k+j+wingspan] = a0 - omega*a1;
                }
            }
            nl++;
            wingspan *= 2;
        }
    }
}
