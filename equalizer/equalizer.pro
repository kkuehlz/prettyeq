TEMPLATE = lib
QMAKE_CFLAGS += -ffast-math -fopenmp -march=native
QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
CONFIG += staticlib
HEADERS = \
    arena.h \
    fft.h \
    macro.h \
    pretty.h
    pretty.h
SOURCES = \
    arena.c \
    fft.c \
    pretty.c
