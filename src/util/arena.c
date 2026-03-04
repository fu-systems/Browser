/*
 * Pane — Arena Allocator Implementation
 */

#include "arena.h"
#include <stdlib.h>
#include <string.h>

static ArenaBlock *arena_new_block(size_t data_size)
{
    ArenaBlock *b = malloc(sizeof(ArenaBlock) + data_size);
    if (!b) return NULL;
    b->next = NULL;
    b->size = data_size;
    b->used = 0;
    return b;
}

void arena_init(Arena *a, size_t block_size)
{
    memset(a, 0, sizeof(*a));
    a->default_block_size = block_size ? block_size : ARENA_DEFAULT_BLOCK_SIZE;
}

static size_t align_up(size_t n, size_t align)
{
    return (n + align - 1) & ~(align - 1);
}

void *arena_alloc(Arena *a, size_t size, size_t al)
{
    if (al == 0) al = 8;

    /* Try to fit in current block. */
    if (a->current) {
        size_t offset = align_up(a->current->used, al);
        if (offset + size <= a->current->size) {
            void *ptr = a->current->data + offset;
            a->current->used = offset + size;
            a->total_allocated += size;
            return ptr;
        }
    }

    /* Need a new block. */
    size_t block_data = a->default_block_size;
    if (size + al > block_data)
        block_data = size + al;

    ArenaBlock *b = arena_new_block(block_data);
    if (!b) return NULL;

    if (a->current)
        a->current->next = b;
    else
        a->first = b;
    a->current = b;

    size_t offset = align_up(b->used, al);
    void *ptr = b->data + offset;
    b->used = offset + size;
    a->total_allocated += size;
    return ptr;
}

void *arena_calloc(Arena *a, size_t count, size_t elem_size)
{
    size_t total = count * elem_size;
    void *p = arena_alloc(a, total, 8);
    if (p) memset(p, 0, total);
    return p;
}

char *arena_strdup(Arena *a, const char *s)
{
    if (!s) return NULL;
    size_t len = strlen(s);
    char *d = arena_alloc(a, len + 1, 1);
    if (d) memcpy(d, s, len + 1);
    return d;
}

char *arena_strndup(Arena *a, const char *s, size_t n)
{
    if (!s) return NULL;
    char *d = arena_alloc(a, n + 1, 1);
    if (d) {
        memcpy(d, s, n);
        d[n] = '\0';
    }
    return d;
}

void arena_destroy(Arena *a)
{
    ArenaBlock *b = a->first;
    while (b) {
        ArenaBlock *next = b->next;
        free(b);
        b = next;
    }
    memset(a, 0, sizeof(*a));
}

void arena_reset(Arena *a)
{
    for (ArenaBlock *b = a->first; b; b = b->next)
        b->used = 0;
    a->current = a->first;
    a->total_allocated = 0;
}
