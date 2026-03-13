/*
 * Pane — Grid Layout
 *
 * Implements CSS Grid Layout: two-dimensional placement of children
 * on explicit/implicit row and column tracks with gap support,
 * grid-auto-flow, and item alignment.
 */

#ifndef PANE_LAYOUT_GRID_H
#define PANE_LAYOUT_GRID_H

#include "box.h"

/* Lay out a grid container and all its children. */
void layout_grid(LayoutBox *box, float containing_width, float containing_height,
                 Arena *arena);

#endif /* PANE_LAYOUT_GRID_H */
