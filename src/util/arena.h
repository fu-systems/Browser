/*
 * Pane — Arena Allocator
 *
 * Bump allocator with linked-list block growth. All allocations are freed
 * together when the arena is destroyed. Individual frees are not supported.
 * This is the primary allocator for DOM nodes, style data, and layout boxes.
 */

#ifndef PANE_ARENA_H
#define PANE_ARENA_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define ARENA_DEFAULT_BLOCK_SIZE (64 * 1024)  /* 64 KB */

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t             size;
    size_t             used;
    uint8_t            data[];  /* flexible array member */
} ArenaBlock;

typedef struct {
    ArenaBlock *first;
    ArenaBlock *current;
    size_t      default_block_size;
    size_t      total_allocated;
} Arena;

/* Initialize an arena with the given default block size (0 = use default). */
void arena_init(Arena *a, size_t block_size);

/* Allocate `size` bytes aligned to `align` (must be power of 2). */
void *arena_alloc(Arena *a, size_t size, size_t align);

/* Convenience: allocate zero-initialized memory. */
void *arena_calloc(Arena *a, size_t count, size_t elem_size);

/* Convenience: duplicate a string into the arena. */
char *arena_strdup(Arena *a, const char *s);

/* Convenience: duplicate n bytes of a string (adds null terminator). */
char *arena_strndup(Arena *a, const char *s, size_t n);

/* Free all memory in the arena. */
void arena_destroy(Arena *a);

/* Reset the arena (keep blocks allocated, just reset usage). */
void arena_reset(Arena *a);

#endif /* PANE_ARENA_H */
