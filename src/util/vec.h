/*
 * Pane — Type-Safe Dynamic Array (Header-Only Macros)
 *
 * Usage:
 *   Vec(int) numbers;
 *   vec_init(&numbers);
 *   vec_push(&numbers, 42);
 *   int x = numbers.data[0];
 *   vec_free(&numbers);
 */

#ifndef PANE_VEC_H
#define PANE_VEC_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Declare a typed vector. */
#define Vec(T) struct { T *data; size_t len; size_t cap; }

/* Initialize an empty vector. */
#define vec_init(v) do { (v)->data = NULL; (v)->len = 0; (v)->cap = 0; } while(0)

/* Free a vector's memory. */
#define vec_free(v) do { free((v)->data); vec_init(v); } while(0)

/* Ensure capacity for at least `n` more elements. */
#define vec_reserve(v, n) do {                                          \
    if ((v)->len + (n) > (v)->cap) {                                    \
        size_t _new_cap = (v)->cap ? (v)->cap * 2 : 8;                 \
        while (_new_cap < (v)->len + (n)) _new_cap *= 2;               \
        (v)->data = realloc((v)->data, _new_cap * sizeof(*(v)->data));  \
        (v)->cap = _new_cap;                                            \
    }                                                                    \
} while(0)

/* Push an element to the end. */
#define vec_push(v, item) do {                   \
    vec_reserve(v, 1);                           \
    (v)->data[(v)->len++] = (item);              \
} while(0)

/* Pop the last element (undefined if empty). */
#define vec_pop(v) ((v)->data[--(v)->len])

/* Get last element without removing. */
#define vec_last(v) ((v)->data[(v)->len - 1])

/* Clear without freeing. */
#define vec_clear(v) ((v)->len = 0)

/* Remove element at index, shifting remaining elements. */
#define vec_remove(v, idx) do {                                         \
    if ((idx) < (v)->len - 1)                                           \
        memmove(&(v)->data[(idx)], &(v)->data[(idx)+1],                 \
                ((v)->len - (idx) - 1) * sizeof(*(v)->data));           \
    (v)->len--;                                                          \
} while(0)

/* Insert element at index, shifting remaining elements. */
#define vec_insert(v, idx, item) do {                                   \
    vec_reserve(v, 1);                                                  \
    if ((idx) < (v)->len)                                               \
        memmove(&(v)->data[(idx)+1], &(v)->data[(idx)],                 \
                ((v)->len - (idx)) * sizeof(*(v)->data));               \
    (v)->data[(idx)] = (item);                                          \
    (v)->len++;                                                          \
} while(0)

/* Find first index of element matching a predicate. Returns -1 if not found. */
#define vec_find(v, pred, ctx, out_idx) do {     \
    *(out_idx) = -1;                             \
    for (size_t _i = 0; _i < (v)->len; _i++) {  \
        if (pred(&(v)->data[_i], ctx)) {         \
            *(out_idx) = (int)_i;                \
            break;                               \
        }                                        \
    }                                            \
} while(0)

#endif /* PANE_VEC_H */
