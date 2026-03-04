/*
 * Pane — Inline Layout
 *
 * Handles inline formatting context: flows content horizontally,
 * wraps lines, and positions inline boxes and text.
 */

#ifndef PANE_LAYOUT_INLINE_H
#define PANE_LAYOUT_INLINE_H

#include "box.h"

/* Lay out an inline or text box within the given available width. */
void layout_inline(LayoutBox *box, float available_width, Arena *arena);

#endif /* PANE_LAYOUT_INLINE_H */
