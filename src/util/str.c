/*
 * Pane — String Implementation
 */

#include "str.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bool pstr_eq(PaneStr a, PaneStr b)
{
    if (a.len != b.len) return false;
    if (a.data == b.data) return true;
    return memcmp(a.data, b.data, a.len) == 0;
}

bool pstr_eq_ci(PaneStr a, PaneStr b)
{
    if (a.len != b.len) return false;
    for (size_t i = 0; i < a.len; i++) {
        if (tolower((unsigned char)a.data[i]) !=
            tolower((unsigned char)b.data[i]))
            return false;
    }
    return true;
}

bool pstr_eq_cstr(PaneStr a, const char *b)
{
    size_t blen = strlen(b);
    return pstr_eq(a, pstr_from(b, blen));
}

bool pstr_eq_cstr_ci(PaneStr a, const char *b)
{
    size_t blen = strlen(b);
    return pstr_eq_ci(a, pstr_from(b, blen));
}

void str_ascii_lower(char *s, size_t len)
{
    for (size_t i = 0; i < len; i++)
        s[i] = (char)tolower((unsigned char)s[i]);
}

void pstr_to_cstr(PaneStr s, char *buf, size_t buf_size)
{
    if (buf_size == 0) return;
    size_t n = s.len < buf_size - 1 ? s.len : buf_size - 1;
    memcpy(buf, s.data, n);
    buf[n] = '\0';
}

/* FNV-1a hash. */
static uint64_t str_hash(const char *s, size_t len)
{
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint8_t)s[i];
        h *= 1099511628211ULL;
    }
    return h;
}

#define INTERN_INITIAL_CAP 256

void strintern_init(StrIntern *si)
{
    arena_init(&si->arena, 0);
    si->capacity = INTERN_INITIAL_CAP;
    si->count = 0;
    si->buckets = calloc(si->capacity, sizeof(char *));
}

void strintern_destroy(StrIntern *si)
{
    arena_destroy(&si->arena);
    free(si->buckets);
    memset(si, 0, sizeof(*si));
}

static void strintern_grow(StrIntern *si)
{
    size_t new_cap = si->capacity * 2;
    char **new_buckets = calloc(new_cap, sizeof(char *));

    for (size_t i = 0; i < si->capacity; i++) {
        char *entry = si->buckets[i];
        if (!entry) continue;

        size_t len = strlen(entry);
        uint64_t h = str_hash(entry, len);
        size_t idx = h & (new_cap - 1);
        while (new_buckets[idx])
            idx = (idx + 1) & (new_cap - 1);
        new_buckets[idx] = entry;
    }

    free(si->buckets);
    si->buckets = new_buckets;
    si->capacity = new_cap;
}

const char *strintern_get(StrIntern *si, const char *s, size_t len)
{
    if (!s) return NULL;

    if (si->count * 4 >= si->capacity * 3)
        strintern_grow(si);

    uint64_t h = str_hash(s, len);
    size_t idx = h & (si->capacity - 1);

    while (si->buckets[idx]) {
        char *entry = si->buckets[idx];
        if (strlen(entry) == len && memcmp(entry, s, len) == 0)
            return entry;
        idx = (idx + 1) & (si->capacity - 1);
    }

    char *interned = arena_strndup(&si->arena, s, len);
    si->buckets[idx] = interned;
    si->count++;
    return interned;
}

const char *strintern_cstr(StrIntern *si, const char *s)
{
    if (!s) return NULL;
    return strintern_get(si, s, strlen(s));
}
