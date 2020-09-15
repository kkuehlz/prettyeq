#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "macro.h"
#include "fft.h"

static bool initialized = false;
static complex float omega_vec[K][MAX_SAMPLES];

static inline unsigned int reverse_bits(unsigned int n, unsigned int num_bits) {
    unsigned int i, j;
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
	static const unsigned int b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
    static const unsigned int S[] = {1, 2, 4, 8, 16};
	int i;

	register unsigned int r = 0;

    if (v & b[4])
    {
        v >>= S[4];
        r |= S[4];
    }
    if (v & b[3])
    {
        v >>= S[3];
        r |= S[3];
    }
    if (v & b[2])
    {
        v >>= S[2];
        r |= S[2];
    }
    if (v & b[1])
    {
        v >>= S[1];
        r |= S[1];
    }
    if (v & b[0])
    {
        v >>= S[0];
        r |= S[0];
    }
    return r;
}

void fft_init() {
#pragma omp parallel for
    for (unsigned int n = 0; n < MAX_SAMPLES; n++) {
        for (unsigned int k = 0; k < K; k++) {
            complex float exp = -(2 * M_PI * I * k) / n;
            omega_vec[k][n] = cpowf(M_E, exp);
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
    /* Sigh... In no world should pulse really be handing us anything more than
     * uint16_t in a rapid callback. Fuck it, we downcast. */
    assert(N <= UINT_MAX);

    {
        unsigned int msb;

        for (unsigned int i = 0, j = 0; i < N; j++, i+=channels)
            /* Taking just the left channel for now... */
            output_data[j] = input_data[i];

        N = N / channels;
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
        while (wingspan < N) {
            unsigned int n = wingspan * 2;
            for (unsigned int j = 0; j < N; j+=wingspan*2) {
                for (unsigned int k = 0; k < wingspan; k++) {
                    complex float omega = omega_vec[k][n];
                    complex float a0 = output_data[k+j];
                    complex float a1 = output_data[k+j+wingspan];
                    output_data[k+j] = a0 + omega*a1;
                    output_data[k+j+wingspan] = a0 - omega*a1;
                }
            }
            wingspan *= 2;
        }
    }

    {
        // deltaF = 1 / (N*deltaT)

        /* Get power spectrum, ignoring samples above Nyquist frequency. */
        //for (int k = 0; k < N / 2; k++) {
        //    output_data[k] = cabsf(output_data[k]);
        //    printf("k=%d, p=%f\n", k, crealf(output_data[k]));
        //}
    }
}
