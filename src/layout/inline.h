/*
 * Pane — Inline Layout
 *
 * Handles inline formatting context: flows content horizontally,
 * wraps lines, and positions inline boxes and text.
 */

#ifndef PANE_LAYOUT_INLINE_H
#define PANE_LAYOUT_INLINE_H

#include "box.h"
#include <stddef.h>

/* ── Text measurement callback ─────────────────────────────────────── */

/* Callback for measuring text width using real font metrics.
 * Parameters: text, length, font_size, is_bold, is_monospace.
 * Returns width in pixels. If NULL, falls back to estimate. */
typedef float (*LayoutMeasureTextFn)(const char *text, size_t len,
                                      float font_size, bool bold,
                                      bool monospace);

/* Set the text measurement callback. Call before layout_build(). */
void layout_set_measure_fn(LayoutMeasureTextFn fn);

/* Lay out an inline or text box within the given available width. */
void layout_inline(LayoutBox *box, float available_width, Arena *arena);

#endif /* PANE_LAYOUT_INLINE_H */
