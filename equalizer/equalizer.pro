TEMPLATE = lib
QMAKE_CFLAGS_WARN_ON += -Wno-unused-parameter
CONFIG += staticlib
HEADERS = \
    arena.h \
    pretty.h
SOURCES = \
    arena.c \
    pretty.c
