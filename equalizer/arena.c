#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

#include "arena.h"

#define CHUNK_OFFSET(memory) ((void*)(memory)-(sizeof(chunk_t)))

#define MEMORY_OFFSET(chunk) ((void*)(chunk)+(sizeof(chunk_t)))

#define NEXT_CHUNK(chunk, size) (((void*)MEMORY_OFFSET((chunk)))+(size))

arena_t* arena_new(size_t num_chunks, size_t chunk_size) {
    assert(num_chunks > 0);
    assert(chunk_size > 0);

    arena_t *arena = calloc(sizeof(arena_t), 1);
    if (!arena)
        return NULL;

    size_t map_size = num_chunks * (chunk_size + sizeof(chunk_t));
    void *mem = mmap(
            NULL,
            map_size,
            PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON,
            -1, 0);

    if (mem == MAP_FAILED)
        return NULL;

    chunk_t *chunk = mem;
    for (unsigned int i = 0; i < num_chunks - 1; i++) {
        chunk->next = NEXT_CHUNK(chunk, chunk_size);
        chunk = chunk->next;
    }
    chunk->next = NULL;

    arena->chunk_size = chunk_size;
    arena->map_size = map_size;
    arena->mem = mem;
    arena->avail_chunk = mem;

    return arena;
}

void* arena_alloc(arena_t *arena) {
    assert(arena);
    assert(arena->avail_chunk);

    void *p = MEMORY_OFFSET(arena->avail_chunk);
    assert(p);
    arena->avail_chunk = arena->avail_chunk->next;
    return p;
}

void arena_dealloc(arena_t *arena, void *mem) {
    assert(arena);
    assert(mem);

    chunk_t *chunk = CHUNK_OFFSET(mem);
    chunk->next = arena->avail_chunk;
    arena->avail_chunk = chunk;
}

void arena_destroy(arena_t **arena) {
    assert(arena);
    assert((*arena)->mem);
    munmap((*arena)->mem, (*arena)->map_size);
    free(*arena);
    *arena = NULL;
}
