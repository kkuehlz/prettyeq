#pragma once
#include <complex.h>
#include <math.h>

#define MAX_SAMPLES_LOG_2 12
#define MAX_SAMPLES (1 << MAX_SAMPLES_LOG_2)
#define K (MAX_SAMPLES / 2)

#define FFT_BUCKET_WIDTH(NUM_SAMPLES) (44100/(NUM_SAMPLES))
#define FFT_SAMPLE_TO_FREQ(NUM_SAMPLES, SAMPLE_INDEX) (44100*(SAMPLE_INDEX)/(NUM_SAMPLES))
#define FFT_FREQ_TO_SAMPLE(NUM_SAMPLES, FREQ) ((int)roundf((FREQ)*(NUM_SAMPLES)/44100))
#define FFT_PSD(SAMPLE) ((float)crealf(cabsf((SAMPLE))))

void fft_init();
void fft_run(
        const float *input_data,
        complex float *output_data,
        unsigned int N,
        unsigned int channels);
