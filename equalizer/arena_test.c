#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "arena.h"

int main(int argc, char **argv) {
    arena_t *arena = arena_new(4, 5);
    assert(arena);
    assert(arena->avail_chunk != NULL);

    char *a = arena_alloc(arena);
    memset(a, 'a', 4);
    a[4] = '\0';

    char *b = arena_alloc(arena);
    memset(b, 'b', 4);
    b[4] = '\0';

    char *c = arena_alloc(arena);
    memset(c, 'c', 4);
    c[4] = '\0';

    char *d = arena_alloc(arena);
    memset(d, 'd', 4);
    d[4] = '\0';

    assert(arena->avail_chunk == NULL);
    assert(memcmp(a, "aaaa", 5) == 0);
    assert(memcmp(b, "bbbb", 5) == 0);
    assert(memcmp(c, "cccc", 5) == 0);
    assert(memcmp(d, "dddd", 5) == 0);

    arena_dealloc(arena, c);
    arena_dealloc(arena, a);

    char *e = arena_alloc(arena);
    memset(e, 'e', 4);
    e[4] = '\0';

    char *f = arena_alloc(arena);
    memset(f, 'f', 4);
    f[4] = '\0';

    assert(memcmp(e, "eeee", 5) == 0);
    assert(memcmp(a, "eeee", 5) == 0);
    assert(memcmp(f, "ffff", 5) == 0);
    assert(memcmp(c, "ffff", 5) == 0);

    arena_dealloc(arena, b);
    arena_dealloc(arena, e);
    arena_dealloc(arena, f);
    arena_dealloc(arena, d);

    assert((void *) arena->avail_chunk == (void *) d - sizeof(chunk_t));
    return 0;
}
