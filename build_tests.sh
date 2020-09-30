#!/bin/sh
mkdir -p build
gcc -g -lm -ffast-math -mavx -fopenmp -O2 \
  -o build/fft_test \
  equalizer/fft.c equalizer/fft_test.c

gcc -g -o build/arena_test \
  equalizer/arena.c equalizer/arena_test.c
