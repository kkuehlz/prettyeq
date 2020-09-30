#pragma once

#include <x86intrin.h>

#include "macro.h"

#define XMVECTOR2 __m128
#define XMVECTOR __m128

// Load 2D vector from memory
_forceinline_ XMVECTOR load_float2(const void *ptr) {
    return _mm_castpd_ps(_mm_load_sd((const double *) ptr));
}

// Load and duplicate a 2D vector from memory
// [ a, b, a, b ] where [ a, b ] are 2 floats at the address
_forceinline_ XMVECTOR2 load_float2_dup(const void *ptr) {
    // Duplicating while loading is free, compare these two instructions:
    // https://uops.info/html-instr/MOVDDUP_XMM_M64.html
    // https://uops.info/html-instr/MOVSD_XMM_XMM_M64.html
    return _mm_castpd_ps( _mm_loaddup_pd((const double *) ptr));
}

// Store a 2D vector
_forceinline_ void store_float2(void *ptr, XMVECTOR v) {
    _mm_store_sd((double *) ptr, _mm_castps_pd(v));
}

// Input:
//      v = [a, b, c, d]
// Output:
//      [c, d, c, d]
_forceinline_ XMVECTOR get_top64(XMVECTOR v) {
    return _mm_movehl_ps(v, v);
}

_forceinline_ XMVECTOR negate_top64(__m128 v) {
    return _mm_xor_ps(v, _mm_setr_ps(0, 0, -0.0f, -0.0f));
}

//=============================================================================


#define SHUFFLE_REV64Q _MM_SHUFFLE(2, 3, 0, 1)

// Specialization of fft_mainloop with omega = 1
_forceinline_ void fft_mainloop_j1(complex float *a0, complex float *a1) {
    const XMVECTOR2 a0dup = load_float2_dup(a0);
    const XMVECTOR2 a1dup = load_float2_dup(a1);
    const XMVECTOR product = negate_top64(a1dup);
    const XMVECTOR result = _mm_add_ps(a0dup, product);
    store_float2(a0, result);
    store_float2(a1, get_top64(result));
}

// Specialization of fft_mainloop with omega = [1, 0], [0, -1]
_forceinline_ void fft_mainloop_j2(complex float *a0, complex float *a1) {
    const XMVECTOR v_a0 = _mm_loadu_ps((const float*) a0);
    const XMVECTOR v_a1 = _mm_loadu_ps((const float*) a1);
    // The first omega is real 1.0, multiplication does nothing
    // The second number is [0, -1], multiplication by that number flips values
    // [a, b] * [0, -1] = [-b, a]
    XMVECTOR product = _mm_shuffle_ps(v_a1, v_a1, _MM_SHUFFLE(2, 3, 1, 0));
    product = _mm_xor_ps(product, _mm_setr_ps(0, 0, 0, -0.0f));
    _mm_storeu_ps((float *) a0, _mm_add_ps(v_a0, product));
    _mm_storeu_ps((float *) a1, _mm_sub_ps(v_a0, product));
}

// Inputs are [a, b] x 2 and [c, d] x 2.
// We are computing [ac - bd, ad + bc]
_forceinline_ XMVECTOR complex_multiply_x2(const XMVECTOR x, const XMVECTOR y) {
    const XMVECTOR x0 = _mm_moveldup_ps(x);                        // [a, a]
    const XMVECTOR x1 = _mm_movehdup_ps(x);                        // [b, b]
	const XMVECTOR yrev = _mm_shuffle_ps(y, y, SHUFFLE_REV64Q);    // [d, c]

    // TODO(keur): Disucss the multiplication simplification using
    // _mm_fmaddsub_ps() with Konstantin.
    const XMVECTOR prod0 = _mm_mul_ps(x0, y);       // [ac, ad]
    const XMVECTOR prod1 = _mm_mul_ps(x1, yrev);    // [bd, bc]
    return _mm_addsub_ps(prod0, prod1);             // [ac - bd, ad + bc]
}

_forceinline_ void fft_mainloop_x2(const complex float *omega, complex float *a0, complex float *a1) {
    const XMVECTOR v_omega = _mm_load_ps((const float *) omega);
    const XMVECTOR v_a0 = _mm_loadu_ps((const float *) a0);
    const XMVECTOR v_a1 = _mm_loadu_ps((const float *) a1);

    const XMVECTOR product = complex_multiply_x2(v_omega, v_a1); // omega*a1
    _mm_storeu_ps((float *) a0, _mm_add_ps(v_a1, product));      // a0 = a0 + omega*a1
    _mm_storeu_ps((float *) a1, _mm_sub_ps(v_a1, product));      // a1 = a1 - omega*a1
}

#ifdef __AVX__

// Inputs are [a, b] x 4 and [c, d] x 4.
// We are computing [ac - bd, ad + bc]
_forceinline_ __m256 complex_multiply_x4(const __m256 x, const __m256 y) {
    const __m256 x0 = _mm256_moveldup_ps(x);                   // [a, a]
    const __m256 x1 = _mm256_movehdup_ps(x);                   // [b, b]
    const __m256 yrev = _mm256_permute_ps(y, SHUFFLE_REV64Q ); // [d, c]

    // TODO(keur): Disucss the multiplication simplification using
    // _mm256_fmaddsub_ps() with Konstantin.
    const __m256 prod0 = _mm256_mul_ps(x0, y);                 // [ac, ad]
    const __m256 prod1 = _mm256_mul_ps(x1, yrev);              // [bd, bc]
    return _mm256_addsub_ps(prod0, prod1);                     // [ac - bd, ad + bc]
}

_forceinline_ void fft_mainloop_x4(const complex float *omega, complex float *a0, complex float *a1) {
    const __m256 v_omega = _mm256_load_ps((const float *) omega);
    const __m256 v_a0 = _mm256_loadu_ps((const float *) a0);
    const __m256 v_a1 = _mm256_loadu_ps((const float *) a1);
    const __m256 product = complex_multiply_x4(v_omega, v_a1);    // omega*a1
    _mm256_storeu_ps((float *) a0, _mm256_add_ps(v_a1, product)); // a0 = a0 + omega*a1
    _mm256_storeu_ps((float *) a1, _mm256_sub_ps(v_a1, product)); // a1 = a1 - omega*a1
}
#else
_forceinline_ void fft_mainloop_x4(const complex float *omega, complex float *a0, complex float*a1) {
	// When AVX is disabled in project settings, call fftMainLoop_x2 twice for the same effect.
	// The SSE version is only ~15% slower, BTW.
    fft_mainloop_x2(omega, a0, a1);
    fft_mainloop_x2(omega + 2, a0 + 2, a1 + 2);
}
#endif