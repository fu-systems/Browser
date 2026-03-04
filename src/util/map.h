/*
 * Pane — Hash Map (string keys → void* values)
 *
 * Open-addressing hash map with linear probing.
 */

#ifndef PANE_MAP_H
#define PANE_MAP_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const char *key;
    size_t      key_len;
    uint64_t    hash;
    void       *value;
    bool        occupied;
} MapEntry;

typedef struct {
    MapEntry *entries;
    size_t    capacity;
    size_t    count;
} Map;

void  map_init(Map *m);
void  map_free(Map *m);

/* Set a key-value pair. Returns the previous value (or NULL). */
void *map_set(Map *m, const char *key, size_t key_len, void *value);

/* Get a value by key. Returns NULL if not found. */
void *map_get(const Map *m, const char *key, size_t key_len);

/* Convenience for C strings. */
void *map_set_cstr(Map *m, const char *key, void *value);
void *map_get_cstr(const Map *m, const char *key);

/* Check if a key exists. */
bool  map_has(const Map *m, const char *key, size_t key_len);

/* Iteration. Returns false from callback to stop. */
typedef bool (*MapIterFn)(const char *key, size_t key_len, void *value, void *ctx);
void  map_iter(const Map *m, MapIterFn fn, void *ctx);

#endif /* PANE_MAP_H */
