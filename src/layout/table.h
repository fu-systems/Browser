/*
 * Pane — Table Layout
 *
 * Implements table formatting context: distributes column widths,
 * lays out cells horizontally within rows, and equalizes row heights.
 */

#ifndef PANE_LAYOUT_TABLE_H
#define PANE_LAYOUT_TABLE_H

#include "box.h"

/* Lay out a table box and all its descendants. */
void layout_table(LayoutBox *box, float containing_width, float containing_height,
                  Arena *arena);

#endif /* PANE_LAYOUT_TABLE_H */
