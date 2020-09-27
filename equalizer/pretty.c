#include <assert.h>
#include <complex.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <pulse/pulseaudio.h>
#include <pulse/sample.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "arena.h"
#include "fft.h"
#include "pretty.h"

#define NUM_FILTERS 7
#define LATENCY_MAX_MS 30

//=============================================================================

static pa_mainloop_api *mainloop_api = NULL;
static pa_context *context = NULL;
static uint32_t prettyeq_module_index = PA_INVALID_INDEX;
static pa_stream *read_stream = NULL, *write_stream = NULL;
static pa_threaded_mainloop *m = NULL;

static const pa_sample_spec sample_spec = {
    // https://freedesktop.org/software/pulseaudio/doxygen/sample_8h.html
    .format = PA_SAMPLE_FLOAT32LE,
    .rate = 44100,
    .channels = 2,
};

//=============================================================================

static _Atomic bool bypass;

static arena_t *arena = NULL;

struct _AudioFFT {
    pthread_spinlock_t lock;
    complex float data[MAX_SAMPLES];
    unsigned int N;
};
static AudioFFT audio_fft = {0};

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
    FilterParams *storage;
};
static PrettyFilter filters[NUM_FILTERS];
static int user_enabled_filters = 0;

//=============================================================================

static void quit(int ret) {
    fprintf(stderr, "debug: quit(%d) called!", ret);
    if (prettyeq_module_index != PA_INVALID_INDEX)
        pa_operation_unref(pa_context_unload_module(context, prettyeq_module_index, NULL, NULL));
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

    if (arena)
        arena_destroy(&arena);

    if (audio_fft.lock) {
        pthread_spin_destroy(&audio_fft.lock);
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
        return;
    }

    if (is_last)
        return;

    assert(i);

    if (strcmp(SINK_NAME, i->name) != 0) {
        const char *app_name;
        app_name = pa_proplist_gets(i->proplist, PA_PROP_APPLICATION_NAME);
        fprintf(stderr, "new sink-input: name=%s, index=%d, sink=%d\n", app_name, i->index, i->sink);
        pa_operation_unref(pa_context_move_sink_input_by_name(c, i->index, SINK_NAME, NULL, NULL));
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

static void read_stream_callback(pa_stream *s, size_t length, void *userdata) {
    assert(s);
    assert(length > 0);

    while (pa_stream_readable_size(s) > 0) {
        const void *input_data = NULL;
        float *fp = NULL;
        size_t num_samples;

        if (pa_stream_peek(s, &input_data, &length) < 0) {
            fprintf(stderr, "pa_stream_peek() failed: %s\n",
                            pa_strerror(pa_context_errno(context)));
            quit(1);
            return;
        }

        fp = (float *) input_data;
        num_samples = length / sizeof(float);

        if (bypass)
            goto play_frame;

#ifndef EQ_DISABLE
        for (int k = 0; k < NUM_FILTERS; k++) {
            /* Swap in NULL to block a filter param update mid-iteration. */
            FilterParams *params = atomic_exchange(&filters[k].params, NULL);

            float *xwin = filters[k].xwin;
            float *ywin = filters[k].ywin;
            float *a = params->a;
            float *b = params->b;

            for (unsigned long i = 0; i < length / sizeof(float); i++) {
                float f;

                /* Slide x-window */
                xwin[2] = xwin[1];
                xwin[1] = xwin[0];
                xwin[0] = fp[i];

                f = fp[i];
                f = (b[0] / a[0] * xwin[0]) +
                    (b[1] / a[0] * xwin[1]) +
                    (b[2] / a[0] * xwin[2]) -
                    (a[1] / a[0] * ywin[0]) -
                    (a[2] / a[0] * ywin[1]);

                if (IS_DENORMAL(f))
                    f = 0.0f;

                fp[i] = f;

                /* Slide y-window */
                ywin[1] = ywin[0];
                ywin[0] = f;
            }
            /* Unblock any pending filter param updates. */
            atomic_store(&filters[k].params, params);
        }
#endif // EQ_DISABLE

play_frame:
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

        if (pthread_spin_trylock(&audio_fft.lock) >= 0) {
            /* Sigh... In no world should pulse really be handing us anything more than
             * uint16_t in a rapid callback. Fuck it, we downcast. */
            assert(num_samples <= UINT_MAX);
            fft_run(fp, audio_fft.data, (unsigned int) num_samples, sample_spec.channels);
            audio_fft.N = (unsigned int) num_samples / sample_spec.channels;
            pthread_spin_unlock(&audio_fft.lock);
        }

        if (pa_stream_drop(s) < 0) {
            fprintf(stderr, "pa_stream_drop() failed: %s\n",
                            pa_strerror(pa_context_errno(context)));
            quit(1);
            return;
        }
    }
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

    /* Subscribe to new sink-inputs so we can send them through the equalizer. */
    pa_context_set_subscribe_callback(c, subscribe_callback, NULL);
    pa_operation_unref(pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SINK_INPUT, NULL, NULL));
}

static void module_list_callback(pa_context *c, const pa_module_info *i, int is_last, void *userdata) {
    pa_threaded_mainloop *m = userdata;
    static bool signal_on_last = true;

    assert(m);
    assert(c);

    if (is_last) {
        if (signal_on_last)
            pa_threaded_mainloop_signal(m, 0);
        return;
    }

    assert(i);
    if (strcmp(i->name, "module-null-sink") == 0 && i->argument && strcmp(i->argument, "sink_name=" SINK_NAME) == 0) {
        pa_operation_unref(pa_context_unload_module(c, i->index, unload_module_callback, m));
        signal_on_last = false;
    }

}

static void drain_signal_callback(pa_context *c, void *userdata) {
    pa_threaded_mainloop *m = userdata;

    assert(c);
    assert(m);

    pa_threaded_mainloop_signal(m, 0);
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
            pa_threaded_mainloop_signal(m, 0);

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

    /* Initialize the fft and user exposed data structures. */
    fft_init();
    audio_fft.N = 0;
    r = pthread_spin_init(&audio_fft.lock, PTHREAD_PROCESS_PRIVATE);
    if (r < 0) {
        fprintf(stderr, "Could not pthread_spin_init()");
        goto err;
    }

    /* Initialize the memory arena used to create new filters. */
    arena = arena_new(NUM_FILTERS * 2, sizeof(FilterParams));
    if (!arena) {
        fprintf(stderr, "Could not arena_new()");
        r = -ENOMEM;
        goto err;
    }

    /* Initialize filters. */
    for (int i = 0; i < NUM_FILTERS; i++) {
        FilterParams *start_params;
        PrettyFilter *filter = &filters[i];

        filter->xwin[0] = 0.0f;
        filter->xwin[1] = 0.0f;
        filter->xwin[2] = 0.0f;
        filter->ywin[0] = 0.0f;
        filter->ywin[1] = 0.0f;

        start_params = arena_alloc(arena);
        atomic_store(&filter->params, start_params);
        filter->storage = start_params;

        /* We don't want to page fault reading filters in the audio loop. */
        r = mlock(&filter, sizeof(PrettyFilter));
        if (r < 0) {
            fprintf(stderr, "mlock() failed: %s.", strerror(errno));
            goto err;
        }
    }

    /* Start with bypass mode disabled. */
    atomic_store(&bypass, false);

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

void pretty_setup_sink_io() {
    pa_operation *o;

    pa_threaded_mainloop_lock(m);

    /* Wait until context is ready. */
    while(pa_context_get_state(context) != PA_CONTEXT_READY)
        pa_threaded_mainloop_wait(m);

    /* Handle an unclean termination and cleanup an existing prettyeq module. */
    o = pa_context_get_module_info_list(context, module_list_callback, m);
    while (pa_operation_get_state(o) == PA_OPERATION_RUNNING)
        pa_threaded_mainloop_wait(m);
    pa_operation_cancel(o);
    pa_operation_unref(o);

    /* Drain the context. TODO(keur): Not sure if we actually need this. Poorly documented function */
    if ((o = pa_context_drain(context, drain_signal_callback, m))) {
        while (pa_operation_get_state(o) == PA_OPERATION_RUNNING)
            pa_threaded_mainloop_wait(m);
        pa_operation_unref(o);
    }

    pa_threaded_mainloop_unlock(m);

    /* Load in the null sink to act as audio IO. */
    pa_operation_unref(pa_context_load_module(context,
                                              "module-null-sink",
                                              "sink_name=" SINK_NAME,
                                              load_module_callback,
                                              NULL));
}

int pretty_exit() {
    if (m) {
        pa_operation *o;
        if (prettyeq_module_index != PA_INVALID_INDEX) {
            pa_threaded_mainloop_lock(m);
            o = pa_context_unload_module(context, prettyeq_module_index, unload_module_callback, m);
            while (pa_operation_get_state(o) == PA_OPERATION_RUNNING)
                pa_threaded_mainloop_wait(m);
            pa_operation_unref(o);
            pa_threaded_mainloop_unlock(m);
        }
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

static inline void safe_update_audio_loop(PrettyFilter *filter, FilterParams *new_params) {
    FilterParams *expected;

    assert(filter);
    assert(new_params);

    do {
        expected = filter->storage;
        atomic_compare_exchange_strong(&filter->params, &expected, new_params);
    } while (!expected);

    arena_dealloc(arena, filter->storage);
    filter->storage = new_params;
}

void pretty_set_peaking_eq(PrettyFilter *filter, float f0, float bandwidth, float db_gain) {
    FilterParams *new_params;
    float A, alpha, w0, sinw0, cosw0;

    new_params = arena_alloc(arena);

    A = powf(10, db_gain/40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    alpha = sinw0*sinhf(logf(2)/2 * bandwidth * w0/sinw0);
    new_params->b[0] = 1 + alpha*A;
    new_params->b[1] = -2 * cosw0;
    new_params->b[2] = 1 - alpha*A;
    new_params->a[0] = 1 + alpha/A;
    new_params->a[1] = -2 * cosw0;
    new_params->a[2] = 1 - alpha/A;

    safe_update_audio_loop(filter, new_params);
}

void pretty_set_low_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
    FilterParams *new_params;
    float A, alpha, w0, sinw0, cosw0, sqrtA;

    new_params = arena_alloc(arena);

    A = powf(10, db_gain / 40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    sqrtA = sqrtf(A);
    alpha = sinw0/2 * sqrtf((A + 1/A) * (1/S - 1) + 2);
    new_params->b[0] = A*((A + 1) - (A - 1)*cosw0 + 2*sqrtA*alpha);
    new_params->b[1] = 2*A*((A - 1) - (A + 1)*cosw0);
    new_params->b[2] = A*((A + 1) - (A - 1)*cosw0 - 2*sqrtA*alpha);
    new_params->a[0] = (A + 1) + (A - 1)*cosw0 + 2*sqrtA*alpha;
    new_params->a[1] = -2*((A - 1) + (A + 1)*cosw0);
    new_params->a[2] = (A + 1) + (A - 1)*cosw0 - 2*sqrtA*alpha;

    safe_update_audio_loop(filter, new_params);
}

void pretty_set_high_shelf(PrettyFilter *filter, float f0, float S, float db_gain) {
    FilterParams *new_params;
    float A, alpha, w0, sinw0, cosw0, sqrtA;

    new_params = arena_alloc(arena);

    A = powf(10, db_gain / 40);
    w0 = 2*M_PI*f0/sample_spec.rate;
    sinw0 = sinf(w0);
    cosw0 = cosf(w0);
    sqrtA = sqrtf(A);
    alpha = sinw0/2 * sqrtf((A + 1/A) * (1/S - 1) + 2);
    new_params->b[0] = A*((A + 1) + (A - 1)*cosw0 + 2*sqrtA*alpha);
    new_params->b[1] = -2*A*((A - 1) + (A + 1)*cosw0);
    new_params->b[2] = A*((A + 1) + (A - 1)*cosw0 - 2*sqrtA*alpha);
    new_params->a[0] = (A + 1) - (A - 1)*cosw0 + 2*sqrtA*alpha;
    new_params->a[1] = 2*((A - 1) - (A + 1)*cosw0);
    new_params->a[2] = (A + 1) - (A - 1)*cosw0 - 2*sqrtA*alpha;

    safe_update_audio_loop(filter, new_params);
}

void pretty_enable_bypass(bool should_bypass)
{
    atomic_store(&bypass, should_bypass);
}

void pretty_acquire_audio_data(complex float **data, unsigned int *N) {
    int r = pthread_spin_lock(&audio_fft.lock);
    assert(r == 0); /* No deadlock. */
    *data = audio_fft.data;
    *N = audio_fft.N;
}

void pretty_release_audio_data() {
    int r = pthread_spin_unlock(&audio_fft.lock);
    assert(r == 0); /* No deadlock. */
}
