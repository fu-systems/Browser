/*
 * Pane — CSS Value Representation
 *
 * Represents parsed CSS values: keywords, numbers, lengths, colors, etc.
 */

#ifndef PANE_CSS_VALUES_H
#define PANE_CSS_VALUES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Length Units ───────────────────────────────────────────────────── */

typedef enum {
    UNIT_PX,
    UNIT_EM,
    UNIT_REM,
    UNIT_PERCENT,
    UNIT_VW,
    UNIT_VH,
    UNIT_VMIN,
    UNIT_VMAX,
    UNIT_CM,
    UNIT_MM,
    UNIT_IN,
    UNIT_PT,
    UNIT_PC,
    UNIT_CH,
    UNIT_EX,
    UNIT_LH,
    UNIT_FR,    /* grid fraction */
    UNIT_DEG,
    UNIT_RAD,
    UNIT_S,
    UNIT_MS,
    UNIT_NONE,  /* for unitless numbers */
} CssUnit;

/* ── Value Types ────────────────────────────────────────────────────── */

typedef enum {
    VAL_KEYWORD,
    VAL_NUMBER,
    VAL_LENGTH,
    VAL_PERCENTAGE,
    VAL_COLOR,
    VAL_STRING,
    VAL_URL,
    VAL_FUNCTION,
    VAL_INITIAL,
    VAL_INHERIT,
    VAL_UNSET,
    VAL_REVERT,
    VAL_AUTO,
    VAL_NONE,
    VAL_LIST,       /* multiple values (e.g., background layers) */
    VAL_PAIR,       /* two values (e.g., background-position) */
} CssValueType;

/* ── Color ──────────────────────────────────────────────────────────── */

typedef struct {
    uint8_t r, g, b, a;
} CssColor;

#define CSS_COLOR(r,g,b) ((CssColor){(r),(g),(b),255})
#define CSS_COLOR_TRANSPARENT ((CssColor){0,0,0,0})

/* ── Value ──────────────────────────────────────────────────────────── */

typedef struct CssValue CssValue;
struct CssValue {
    CssValueType type;
    union {
        int          keyword;     /* interned or enum */
        float        number;
        struct {
            float    magnitude;
            CssUnit  unit;
        } length;
        float        percentage;  /* 0-100 */
        CssColor     color;
        const char  *string;
        const char  *url;
        struct {
            const char *name;
            CssValue   *args;
            int         arg_count;
        } function;
        struct {
            CssValue *items;
            int       count;
        } list;
        struct {
            CssValue *a;
            CssValue *b;
        } pair;
    };
};

/* Predefined values. */
extern const CssValue css_value_auto;
extern const CssValue css_value_none;
extern const CssValue css_value_initial;
extern const CssValue css_value_inherit;
extern const CssValue css_value_zero;

/* Resolve a length to pixels given context. */
float css_length_to_px(CssValue val, float font_size, float root_font_size,
                       float viewport_w, float viewport_h,
                       float containing_block_size);

/* Parse a named color. Returns false if not recognized. */
bool css_color_from_name(const char *name, size_t len, CssColor *out);

/* Parse a hex color. Returns false if not valid. */
bool css_color_from_hex(const char *hex, size_t len, CssColor *out);

#endif /* PANE_CSS_VALUES_H */
