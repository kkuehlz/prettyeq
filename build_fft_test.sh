#!/bin/sh
mkdir -p build
gcc -lm -ffast-math -fopenmp -O3 \
  -o build/fft_test \
  equalizer/fft.c equalizer/fft_test.c
