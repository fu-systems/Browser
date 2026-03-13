/*
 * Pane — Flex Layout
 *
 * Implements basic flexbox layout: arranges children along the main axis
 * with support for flex-direction, justify-content, align-items, flex-grow,
 * and flex-wrap.
 */

#ifndef PANE_LAYOUT_FLEX_H
#define PANE_LAYOUT_FLEX_H

#include "box.h"

/* Lay out a flex container and all its children. */
void layout_flex(LayoutBox *box, float containing_width, float containing_height,
                 Arena *arena);

#endif /* PANE_LAYOUT_FLEX_H */
