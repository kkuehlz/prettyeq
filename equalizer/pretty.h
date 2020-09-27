#pragma once

#include "macro.h"

#include <complex.h>
#include <stdbool.h>
#include <stdint.h>

#define SINK_NAME "prettyeq"

#define DRAIN_NO_CB(c) (pa_operation_unref(pa_context_drain(c, NULL, NULL)))

#define IS_DENORMAL(f)          \
    ({                          \
        typeof(f) *_f_ = &(f);  \
        ((*(unsigned int *)_f_) & 0x7f800000) == 0 || ((*(unsigned int *)_f_) & 0xff800000) == 0; \
    })

/* accomodating the stupid C++ template redefinition... */
#ifdef __cplusplus
    typedef std::complex<float>* FFTComplexCompat;
#else
    typedef complex float* FFTComplexCompat;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PrettyFilter PrettyFilter;

typedef struct _AudioFFT AudioFFT;

PRETTY_EXPORT
int pretty_init();

PRETTY_EXPORT
int pretty_exit();

PRETTY_EXPORT
void pretty_setup_sink_io();

PRETTY_EXPORT
int pretty_new_filter(PrettyFilter **filter);

PRETTY_EXPORT
void pretty_set_peaking_eq(PrettyFilter *filter, float f0, float bandwidth, float db_gain);

PRETTY_EXPORT
void pretty_set_low_shelf(PrettyFilter *filter, float f0, float S, float db_gain);

PRETTY_EXPORT
void pretty_set_high_shelf(PrettyFilter *filter, float f0, float S, float db_gain);

PRETTY_EXPORT
void pretty_enable_bypass(bool should_bypass);

PRETTY_EXPORT
void pretty_acquire_audio_data(FFTComplexCompat *data, unsigned int *N);

PRETTY_EXPORT
void pretty_release_audio_data();

#ifdef __cplusplus
}
#endif
