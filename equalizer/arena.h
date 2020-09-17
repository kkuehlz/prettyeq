#pragma once

#include <stddef.h>

typedef struct chunk_t chunk_t;
struct chunk_t {
    chunk_t *next;
};

typedef struct arena_t {
    void *mem;
    size_t chunk_size;
    size_t map_size;
    chunk_t *avail_chunk;
} arena_t;

arena_t* arena_new(size_t num_chunks, size_t chunk_size);
void* arena_alloc(arena_t *arena);
void arena_dealloc(arena_t *arena, void *mem);
void arena_destroy(arena_t **arena);
