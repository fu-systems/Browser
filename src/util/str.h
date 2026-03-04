/*
 * Pane — String Type + Interning
 *
 * PaneStr is a length-delimited string (pointer + length). All PaneStr
 * instances created through the interning table are deduplicated and can
 * be compared by pointer equality.
 */

#ifndef PANE_STR_H
#define PANE_STR_H

#include <stddef.h>
#include <stdbool.h>
#include "arena.h"

/* A non-owning string reference. */
typedef struct {
    const char *data;
    size_t      len;
} PaneStr;

/* Construct from C string literal or pointer. */
#define PSTR(lit) ((PaneStr){ (lit), sizeof(lit) - 1 })
#define pstr_from(s, n) ((PaneStr){ (s), (n) })
#define pstr_null() ((PaneStr){ NULL, 0 })

static inline bool pstr_is_null(PaneStr s) { return s.data == NULL; }
static inline bool pstr_is_empty(PaneStr s) { return s.len == 0; }

/* Compare two PaneStr for content equality. */
bool pstr_eq(PaneStr a, PaneStr b);

/* Case-insensitive content equality (ASCII only). */
bool pstr_eq_ci(PaneStr a, PaneStr b);

/* Compare with C string. */
bool pstr_eq_cstr(PaneStr a, const char *b);
bool pstr_eq_cstr_ci(PaneStr a, const char *b);

/* String interning table. */
typedef struct {
    Arena  arena;
    char **buckets;
    size_t capacity;
    size_t count;
} StrIntern;

void strintern_init(StrIntern *si);
void strintern_destroy(StrIntern *si);

/* Intern a string: returns a pointer that is unique for this content.
 * Two calls with the same content return the same pointer. */
const char *strintern_get(StrIntern *si, const char *s, size_t len);

/* Convenience for C strings. */
const char *strintern_cstr(StrIntern *si, const char *s);

/* Utility: convert ASCII to lowercase in-place. */
void str_ascii_lower(char *s, size_t len);

/* Utility: copy a PaneStr to a null-terminated C string in a buffer. */
void pstr_to_cstr(PaneStr s, char *buf, size_t buf_size);

#endif /* PANE_STR_H */
