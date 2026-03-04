/*
 * Pane — Font Rendering (FreeType + Fontconfig)
 *
 * Provides font loading, glyph rasterization, and text measurement
 * using FreeType2 for rendering and Fontconfig for font discovery.
 */

#ifndef PANE_FONT_H
#define PANE_FONT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ── Font Style ────────────────────────────────────────────────────── */

typedef enum {
    FONT_STYLE_NORMAL,
    FONT_STYLE_ITALIC,
    FONT_STYLE_BOLD,
    FONT_STYLE_BOLD_ITALIC,
} FontStyle;

/* ── Glyph Bitmap ──────────────────────────────────────────────────── */

typedef struct {
    uint8_t *buffer;      /* 8-bit grayscale bitmap */
    int      width;
    int      height;
    int      bearing_x;   /* offset from pen position to left edge */
    int      bearing_y;   /* offset from baseline to top */
    int      advance_x;   /* horizontal advance in pixels */
} GlyphBitmap;

/* ── Text Metrics ──────────────────────────────────────────────────── */

typedef struct {
    float width;          /* total width of measured text */
    float height;         /* line height (ascent + descent + leading) */
    float ascent;         /* distance from baseline to top */
    float descent;        /* distance from baseline to bottom (positive down) */
} TextMetrics;

/* ── Font System ───────────────────────────────────────────────────── */

/* Opaque font handle. */
typedef struct PaneFont PaneFont;

/* Initialize the font system (call once at startup). */
bool font_system_init(void);

/* Shut down the font system. */
void font_system_shutdown(void);

/* Load a font by family name and style. Returns NULL on failure.
 * Falls back to a default sans-serif if family not found. */
PaneFont *font_load(const char *family, float size_px, FontStyle style);

/* Free a loaded font. */
void font_free(PaneFont *font);

/* Change the pixel size of a font. */
void font_set_size(PaneFont *font, float size_px);

/* Measure a UTF-8 text string without rendering. */
TextMetrics font_measure(PaneFont *font, const char *text, size_t len);

/* Measure a single character's advance width. */
float font_char_width(PaneFont *font, uint32_t codepoint);

/* Render a single glyph. The returned bitmap is valid until the next
 * call to font_render_glyph. Do not free the buffer. */
const GlyphBitmap *font_render_glyph(PaneFont *font, uint32_t codepoint);

/* Get the line height for the current font size. */
float font_line_height(PaneFont *font);

/* Get font ascent. */
float font_ascent(PaneFont *font);

/* Get font descent. */
float font_descent(PaneFont *font);

#endif /* PANE_FONT_H */
