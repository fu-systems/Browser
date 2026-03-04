/*
 * Pane — Layout Box Implementation
 */

#include "box.h"
#include <string.h>

LayoutBox *layout_box_create(Arena *arena, LayoutBoxType type,
                             const DomNode *node, ComputedStyle *style)
{
    LayoutBox *box = arena_calloc(arena, 1, sizeof(LayoutBox));
    box->type = type;
    box->node = node;
    box->style = style;
    return box;
}

void layout_box_append(LayoutBox *parent, LayoutBox *child)
{
    child->parent = parent;
    child->next_sibling = NULL;

    if (parent->last_child) {
        parent->last_child->next_sibling = child;
    } else {
        parent->first_child = child;
    }
    parent->last_child = child;
    parent->child_count++;
}

float layout_box_outer_width(const LayoutBox *box)
{
    return box->margin.left + box->border.left + box->padding.left +
           box->rect.width +
           box->padding.right + box->border.right + box->margin.right;
}

float layout_box_outer_height(const LayoutBox *box)
{
    return box->margin.top + box->border.top + box->padding.top +
           box->rect.height +
           box->padding.bottom + box->border.bottom + box->margin.bottom;
}

Rect layout_box_margin_rect(const LayoutBox *box)
{
    return (Rect){
        .x = box->rect.x - box->padding.left - box->border.left - box->margin.left,
        .y = box->rect.y - box->padding.top - box->border.top - box->margin.top,
        .width = layout_box_outer_width(box),
        .height = layout_box_outer_height(box),
    };
}

Rect layout_box_border_rect(const LayoutBox *box)
{
    return (Rect){
        .x = box->rect.x - box->padding.left - box->border.left,
        .y = box->rect.y - box->padding.top - box->border.top,
        .width = box->border.left + box->padding.left + box->rect.width +
                 box->padding.right + box->border.right,
        .height = box->border.top + box->padding.top + box->rect.height +
                  box->padding.bottom + box->border.bottom,
    };
}
