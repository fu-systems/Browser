/*
 * Pane — Block Layout
 *
 * Implements block formatting context layout: stacks children vertically,
 * resolves widths/heights, handles margin collapsing.
 */

#ifndef PANE_LAYOUT_BLOCK_H
#define PANE_LAYOUT_BLOCK_H

#include "box.h"

/* Lay out a block box and all its descendants. */
void layout_block(LayoutBox *box, float containing_width, float containing_height,
                  Arena *arena);

#endif /* PANE_LAYOUT_BLOCK_H */
