/*
 * Pane — Hash Map Implementation
 */

#include "map.h"
#include <stdlib.h>
#include <string.h>

#define MAP_INITIAL_CAP 16

static uint64_t map_hash(const char *key, size_t len)
{
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)key[i];
        h *= 1099511628211ULL;
    }
    return h;
}

void map_init(Map *m)
{
    m->entries = NULL;
    m->capacity = 0;
    m->count = 0;
}

void map_free(Map *m)
{
    free(m->entries);
    memset(m, 0, sizeof(*m));
}

static void map_grow(Map *m)
{
    size_t new_cap = m->capacity ? m->capacity * 2 : MAP_INITIAL_CAP;
    MapEntry *new_entries = calloc(new_cap, sizeof(MapEntry));
    if (!new_entries) return; /* OOM: keep existing table */

    for (size_t i = 0; i < m->capacity; i++) {
        MapEntry *e = &m->entries[i];
        if (!e->occupied) continue;

        size_t idx = e->hash & (new_cap - 1);
        while (new_entries[idx].occupied)
            idx = (idx + 1) & (new_cap - 1);
        new_entries[idx] = *e;
    }

    free(m->entries);
    m->entries = new_entries;
    m->capacity = new_cap;
}

void *map_set(Map *m, const char *key, size_t key_len, void *value)
{
    if (m->count * 4 >= m->capacity * 3)
        map_grow(m);

    uint64_t h = map_hash(key, key_len);
    size_t idx = h & (m->capacity - 1);

    while (m->entries[idx].occupied) {
        MapEntry *e = &m->entries[idx];
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0) {
            void *old = e->value;
            e->value = value;
            return old;
        }
        idx = (idx + 1) & (m->capacity - 1);
    }

    m->entries[idx] = (MapEntry){
        .key = key, .key_len = key_len,
        .hash = h, .value = value, .occupied = true
    };
    m->count++;
    return NULL;
}

void *map_get(const Map *m, const char *key, size_t key_len)
{
    if (m->count == 0) return NULL;

    uint64_t h = map_hash(key, key_len);
    size_t idx = h & (m->capacity - 1);

    while (m->entries[idx].occupied) {
        MapEntry *e = &m->entries[idx];
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0)
            return e->value;
        idx = (idx + 1) & (m->capacity - 1);
    }
    return NULL;
}

void *map_set_cstr(Map *m, const char *key, void *value)
{
    return map_set(m, key, strlen(key), value);
}

void *map_get_cstr(const Map *m, const char *key)
{
    return map_get(m, key, strlen(key));
}

bool map_has(const Map *m, const char *key, size_t key_len)
{
    if (m->count == 0) return false;

    uint64_t h = map_hash(key, key_len);
    size_t idx = h & (m->capacity - 1);

    while (m->entries[idx].occupied) {
        MapEntry *e = &m->entries[idx];
        if (e->hash == h && e->key_len == key_len &&
            memcmp(e->key, key, key_len) == 0)
            return true;
        idx = (idx + 1) & (m->capacity - 1);
    }
    return false;
}

void map_iter(const Map *m, MapIterFn fn, void *ctx)
{
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->entries[i].occupied) {
            if (!fn(m->entries[i].key, m->entries[i].key_len,
                    m->entries[i].value, ctx))
                return;
        }
    }
}
