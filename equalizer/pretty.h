#pragma once

#include <stdint.h>

#define SINK_NAME "prettyeq"

#define PRETTY_EXPORT __attribute__ ((visibility ("default")))
#define MAY_ALIAS __attribute__((__may_alias__))

#define DRAIN_NO_CB(c) (pa_operation_unref(pa_context_drain(c, NULL, NULL)))

#define IS_DENORMAL(f)          \
    ({                          \
        typeof(f) *_f_ = &(f);  \
        ((*(unsigned int *)_f_) & 0x7f800000) == 0 || ((*(unsigned int *)_f_) & 0xff800000) == 0; \
    })

#define SWAP_PTR(a, b)          \
    ({                          \
        typeof(a) _tmp_ = b;    \
        b = a;                  \
        a = _tmp_;              \
    })

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PrettyFilter PrettyFilter;

PRETTY_EXPORT
int pretty_init();

PRETTY_EXPORT
int pretty_exit();

PRETTY_EXPORT
int pretty_new_filter(PrettyFilter **filter);

PRETTY_EXPORT
void pretty_set_peaking_eq(PrettyFilter *filter, float f0, float bandwidth, float db_gain);

PRETTY_EXPORT
void pretty_set_low_shelf(PrettyFilter *filter, float f0, float S, float db_gain);

PRETTY_EXPORT
void pretty_set_high_shelf(PrettyFilter *filter, float f0, float S, float db_gain);

#ifdef __cplusplus
}
#endif
