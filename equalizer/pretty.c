#include "pretty.h"

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <pulse/pulseaudio.h>
#include <pulse/sample.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#define NUM_FILTERS 7
#define LATENCY_MAX_MS 100

//=============================================================================

static pa_mainloop_api *mainloop_api = NULL;
static pa_context *context = NULL;
static uint32_t prettyeq_module_index;
static pa_stream *read_stream = NULL, *write_stream = NULL;
static pa_threaded_mainloop *m = NULL;

static const pa_sample_spec sample_spec = {
    // https://freedesktop.org/software/pulseaudio/doxygen/sample_8h.html
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = 2,
};

//=============================================================================

typedef struct FilterParams {
    float a[3];
    float b[3];
} FilterParams;

struct _PrettyFilter {
    /* Filter parameters used in the audio loop. */
    float xwin[3];
    float ywin[2];

    /* Temporary parameters for compare-exchange. */
    _Atomic (FilterParams*) params;
    FilterParams* __cx_params;
};
static PrettyFilter filters[NUM_FILTERS];
static int user_enabled_filters = 0;

//=============================================================================

static void quit(int ret) {
    fprintf(stderr, "debug: quit(%d) called!", ret);
    mainloop_api->quit(mainloop_api, ret);
}

static void cleanup() {
    if (read_stream)
        pa_stream_unref(read_stream);

    if (write_stream)
        pa_stream_unref(write_stream);

    if (context)
        pa_context_unref(context);

    if (m)
        pa_threaded_mainloop_free(m);

    for (int i = 0; i < NUM_FILTERS; i++) {
        if (filters[i].params)
            free(filters[i].params);

        if (filters[i].__cx_params)
            free(filters[i].__cx_params);
    }

    munlockall();
}

//=============================================================================

static void success_callback(pa_context *c, int success, void *userdata) {
    assert(c);

    if ( !success) {
        fprintf(stderr, "Failure: %s", pa_strerror(pa_context_errno(c)));
        quit(1);
    }
}

static void sink_input_callback(pa_context *c, const pa_sink_input_info *i, int is_last, void *userdata) {
    assert(c);

    if (is_last < 0) {
        fprintf(stderr, "Failed to get sink information: %s",
                        pa_strerror(pa_context_errno(c)));
        quit(1);
        return;
    }

    if (is_last)
        return;

    assert(i);

    if (strcmp(SINK_NAME, i->name) != 0) {
        const char *app_name;
        app_name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
        fprintf(stderr, "new sink-input: name=%s, index=%d, sink=%d\n", app_name, i->index, i->sink);
        pa_operation_unref(pa_context_move_sink_input_by_name(c, i->index, SINK_NAME, success_callback, NULL));
    } else {
        /* Pulseaudio keeps sink state, so if the user changed the prettyeq
         * playback stream volume, we reset back to 100% here. This is so the
         * equalizer "at rest" doesn't modify the input stream. */
        pa_cvolume volume;
        volume = i->volume;
        pa_cvolume_reset(&volume, i->sample_spec.channels);
        pa_operation_unref(pa_context_set_sink_input_volume(c,
                                                            i->index,
                                                            &volume,
                                                            success_callback,
                                                            NULL));
    }
}

static void subscribe_callback(
        pa_context *c,
        pa_subscription_event_type_t t,
        uint32_t idx,
        void *userdata) {
    assert(c);
    assert((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SINK_INPUT);

    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_NEW)
        pa_operation_unref(pa_context_get_sink_input_info(c, idx, sink_input_callback, NULL));
}

static void null_sink_info_callback(pa_context *c, const pa_sink_info *i, int is_last, void *userdata) {
    pa_buffer_attr buffer_attr;

    assert(c);

    if (is_last < 0) {
        fprintf(stderr, "Failed to get null sink information: %s",
                         pa_strerror(pa_context_errno(c)));
        quit(1);
        return;
    }

    if (is_last)
        return;

    assert(i);

    /* Hook up the read stream to the null sink monitor output. */
    memset(&buffer_attr, 0, sizeof(buffer_attr));
    buffer_attr.maxlength = (uint32_t) -1;
    buffer_attr.prebuf = (uint32_t) -1;
    buffer_attr.fragsize = pa_usec_to_bytes(LATENCY_MAX_MS * PA_USEC_PER_MSEC, &sample_spec);
    buffer_attr.tlength = pa_usec_to_bytes(LATENCY_MAX_MS * PA_USEC_PER_MSEC, &sample_spec);
    if (pa_stream_connect_record(read_stream, i->monitor_source_name, &buffer_attr, PA_STREAM_ADJUST_LATENCY) < 0) {
        fprintf(stderr, "pa_stream_connect_record() failed: %s\n",
                pa_strerror(pa_context_errno(c)));
        quit(1);
    }
    fprintf(stderr, "monitor source name=%s\n", i->monitor_source_name);
}

static void unload_module_callback(pa_context *c, int success, void *userdata) {
    pa_threaded_mainloop *m;
    m = userdata;
    assert(m);

    if (! success)
        fprintf(stderr, "pa_context_unload_module() failed: %s",
                pa_strerror(pa_context_errno(c)));

    pa_threaded_mainloop_signal(m, 0);
}

static void load_module_callback(pa_context *c, uint32_t idx, void *userdata) {
    assert(c);

    if (idx == PA_INVALID_INDEX) {
        fprintf(stderr, "Bad index\n");
        quit(1);
        return;
    }

    prettyeq_module_index = idx;
    pa_operation_unref(pa_context_get_sink_input_info_list(c, sink_input_callback, NULL));
    pa_operation_unref(pa_context_get_sink_info_by_name(c, SINK_NAME, null_sink_info_callback, NULL));
}

static void read_stream_callback(pa_stream *s, size_t length, void *userdata) {
    assert(s);
    assert(length > 0);

    while (pa_stream_readable_size(s) > 0) {
        const void *input_data = NULL;

        if (pa_stream_peek(s, &input_data, &length) < 0) {
            fprintf(stderr, "pa_stream_peek() failed: %s\n",
                            pa_strerror(pa_context_errno(context)));
            quit(1);
            return;
        }

#ifndef EQ_DISABLE
        for (int k = 0; k < NUM_FILTERS; k++) {
            FilterParams *params = atomic_load_explicit(&filters[k].params, __ATOMIC_RELAXED);
            short *fp = (short*) input_data;
            float *xwin = filters[k].xwin;
            float *ywin = filters[k].ywin;
            float *a = params->a;
            float *b = params->b;

            for (unsigned long i = 0; i < length / sizeof(short); i++) {
                float f;

                /* Slide x-window */
                xwin[2] = xwin[1];
                xwin[1] = xwin[0];
                xwin[0] = fp[i];

                f = (float) fp[i];
                f = (b[0] / a[0] * xwin[0]) +
                    (b[1] / a[0] * xwin[1]) +
                    (b[2] / a[0] * xwin[2]) -
                    (a[1] / a[0] * ywin[0]) -
                    (a[2] / a[0] * ywin[1]);

                if (IS_DENORMAL(f)) {
                    f = 0.0f;
                } else if (f > SHRT_MAX) {
                    f = SHRT_MAX;
                } else if (f < SHRT_MIN) {
                    f = SHRT_MIN;
                }
                fp[i] = f;

                /* Slide y-window */
                ywin[1] = ywin[0];
                ywin[0] = f;
            }
        }
#endif // EQ_DISABLE

        for (;;) {
            size_t data_length = length;
            uint8_t *output_data = NULL;

            if (pa_stream_begin_write(write_stream, (void **)&output_data, &data_length) < 0) {
                fprintf(stderr, "pa_stream_begin_write() failed: %s\n",
                                pa_strerror(pa_context_errno(context)));
                quit(1);
                return;
            }

            memcpy(output_data, input_data, data_length);
            pa_stream_write(write_stream, output_data, data_length, NULL, 0, PA_SEEK_RELATIVE);

            if (data_length >= length)
                break;

            length -= data_length;
            input_data += data_length;
        }

        if (pa_stream_drop(s) < 0) {
            fprintf(stderr, "pa_stream_drop() failed: %s\n",
                            pa_strerror(pa_context_errno(context)));
            quit(1);
            return;
        }
    }
}

static void context_state_callback(pa_context *c, void *userdata) {
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {

            if ( !(read_stream = pa_stream_new(c, "prettyeq", &sample_spec, NULL))) {
                fprintf(stderr, "pa_stream_new() failed.\n");
                quit(1);
                return;
            }

            if ( !(write_stream = pa_stream_new(c, "prettyeq", &sample_spec, NULL))) {
                fprintf(stderr, "pa_stream_new() failed.\n");
                quit(1);
                return;
            }

            /* Setup the playback stream. We create the recording stream in the null sink callback. */
            if (pa_stream_connect_playback(write_stream, NULL, NULL, 0, NULL, NULL) < 0) {
                fprintf(stderr, "pa_stream_connect_playback() failed: %s\n",
                        pa_strerror(pa_context_errno(c)));
                quit(1);
                return;
            }

            /* Audio event callback functions. */
            pa_stream_set_read_callback(read_stream, read_stream_callback, NULL);

            /* Load in the null sink to act as audio IO. */
            pa_operation_unref(pa_context_load_module(c,
                                                      "module-null-sink",
                                                      "sink_name=" SINK_NAME,
                                                      load_module_callback,
                                                      NULL));

            /* Subscribe to new sink-inputs so we can send them through the equalizer. */
            pa_context_set_subscribe_callback(c, subscribe_callback, NULL);
            pa_operation_unref(pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL));

            fprintf(stderr, "context is ready!\n");
            break;
        }
        case PA_CONTEXT_FAILED:
            fprintf(stderr, "Unclean termination\n");
            quit(1);
            break;
        case PA_CONTEXT_TERMINATED:
            fprintf(stderr, "Clean termination\n");
            quit(0);
            break;

    }
}

//=============================================================================

int pretty_init() {
    int r;

    /* Initialize filters. */
    for (int i = 0; i < NUM_FILTERS; i++) {
        FilterParams *params;
        FilterParams *__cx_params;
        PrettyFilter *filter = &filters[i];

        filter->xwin[0] = 0.0f;
        filter->xwin[1] = 0.0f;
        filter->xwin[2] = 0.0f;
        filter->ywin[0] = 0.0f;
        filter->ywin[1] = 0.0f;

        params = calloc(sizeof(FilterParams), 1);
        if (!params) {
            fprintf(stderr, "Could not calloc()");
            r = -ENOMEM;
            goto err;
        }
        atomic_store(&filter->params, params);

        __cx_params = calloc(sizeof(FilterParams), 1);
        if (!__cx_params) {
            fprintf(stderr, "Could not calloc()");
            r = -ENOMEM;
            goto err;
        }
        filter->__cx_params = __cx_params;

        /* We don't want to page fault reading filters in the audio loop. */
        r = mlock(&filter, sizeof(PrettyFilter));
        if (r < 0) {
            fprintf(stderr, "mlock() failed: %s.", strerror(errno));
            goto err;
        }
    }

    /* Initialize pulseaudio mainloop. */
    if ( !(m = pa_threaded_mainloop_new())) {
        fprintf(stderr, "pa_mainloop_new() failed.");
        r = -1;
        goto err;
    }

    mainloop_api = pa_threaded_mainloop_get_api(m);
    if ( !(context = pa_context_new_with_proplist(mainloop_api, NULL, NULL))) {
        fprintf(stderr, "pa_context_new() failed.");
        r = -1;
        goto err;
    }

    pa_context_set_state_callback(context, context_state_callback, NULL);
    if (pa_context_connect(context, NULL, 0, NULL) < 0) {
        fprintf(stderr, "pa_context_connect() failed.");
        r = -1;
        goto err;
    }

    r = pa_threaded_mainloop_start(m);
    if (r < 0) {
        fprintf(stderr, "pa_threaded_mainloop_start() failed.");
        goto err;
    }

    return 0;

err:
    cleanup();
    return r;
}

int pretty_exit() {
    if (m) {
        pa_operation *o;

        pa_threaded_mainloop_lock(m);
        o = pa_context_unload_module(context, prettyeq_module_index, unload_module_callback, m);
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m);
        pa_operation_unref(o);
        pa_threaded_mainloop_unlock(m);
        pa_threaded_mainloop_stop(m);
    }
    cleanup();

    return 0;
}

int pretty_new_filter(PrettyFilter **filter) {
    assert(user_enabled_filters < NUM_FILTERS);
    *filter = &filters[user_enabled_filters++];
    return 0;
}

void pretty_set_peaking_eq(PrettyFilter *filter, float f0, float bandwidth, float db_gain) {
    float A, alpha, w0, sinw0, cosw0;
    float *a, *b;

    a = filter->__cx_params->a;
    b = filter->__cx_params->b;

    A = powf(10, db_gain/40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    alpha = sinw0*sinhf(logf(2)/2 * bandwidth * w0/sinw0);
    b[0] = 1 + alpha*A;
    b[1] = -2 * cosw0;
    b[2] = 1 - alpha*A;
    a[0] = 1 + alpha/A;
    a[1] = -2 * cosw0;
    a[2] = 1 - alpha/A;
    filter->__cx_params = atomic_exchange_explicit(&filter->params,
                                                   filter->__cx_params,
                                                   __ATOMIC_RELAXED);
}

void pretty_set_low_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
    float A, alpha, w0, sinw0, cosw0, sqrtA;
    float *a, *b;

    a = filter->__cx_params->a;
    b = filter->__cx_params->b;
    A = powf(10, db_gain / 40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    sqrtA = sqrtf(A);
    alpha = sinw0/2 * sqrtf((A + 1/A) * (1/S - 1) + 2);
    b[0] = A*((A + 1) - (A - 1)*cosw0 + 2*sqrtA*alpha);
    b[1] = 2*A*((A - 1) - (A + 1)*cosw0);
    b[2] = A*((A + 1) - (A - 1)*cosw0 - 2*sqrtA*alpha);
    a[0] = (A + 1) + (A - 1)*cosw0 + 2*sqrtA*alpha;
    a[1] = -2*((A - 1) + (A + 1)*cosw0);
    a[2] = (A + 1) + (A - 1)*cosw0 - 2*sqrtA*alpha;
    filter->__cx_params = atomic_exchange_explicit(&filter->params,
                                                   filter->__cx_params,
                                                   __ATOMIC_RELAXED);
}

void pretty_set_high_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
    float A, alpha, w0, sinw0, cosw0, sqrtA;
    float *a, *b;

    a = filter->__cx_params->a;
    b = filter->__cx_params->b;
    A = powf(10, db_gain / 40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    sqrtA = sqrtf(A);
    alpha = sinw0/2 * sqrtf((A + 1/A) * (1/S - 1) + 2);
    b[0] = A*((A + 1) + (A - 1)*cosw0 + 2*sqrtA*alpha);
    b[1] = -2*A*((A - 1) + (A + 1)*cosw0);
    b[2] = A*((A + 1) + (A - 1)*cosw0 - 2*sqrtA*alpha);
    a[0] = (A + 1) - (A - 1)*cosw0 + 2*sqrtA*alpha;
    a[1] = 2*((A - 1) - (A + 1)*cosw0);
    a[2] = (A + 1) - (A - 1)*cosw0 - 2*sqrtA*alpha;
    filter->__cx_params = atomic_exchange_explicit(&filter->params,
                                                   filter->__cx_params,
                                                   __ATOMIC_RELAXED);
}
