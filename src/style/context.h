/*
 * Pane — Pairwise Context Transition Engine
 *
 * Implements the core pairwise model: (parent_context, child_element) →
 * child_context. The context object carries 21 fields that propagate
 * down the DOM tree, determining formatting contexts, containing blocks,
 * stacking contexts, and inherited property values.
 *
 * Based on the rules defined in:
 *   catalog/html_pair_chain_handling.md  (62 compressed HTML rules)
 *   catalog/css_pair_chain_handling.md   (31 compressed CSS rules)
 *   catalog/context_creators.json       (21 context fields)
 */

#ifndef PANE_CONTEXT_H
#define PANE_CONTEXT_H

#include "computed.h"
#include "../dom/dom.h"
#include "../util/arena.h"

/* ── Formatting Context Type ───────────────────────────────────────── */

typedef enum {
    FC_BLOCK,
    FC_INLINE,
    FC_FLEX,
    FC_GRID,
    FC_TABLE,
    FC_TABLE_ROW,
    FC_TABLE_CELL,
    FC_MULTICOL,
    FC_RUBY,
    FC_SVG,
    FC_MATHML,
} FormattingContextType;

/* ── Pairwise Context ──────────────────────────────────────────────── */
/* This is THE context object that flows down the DOM tree. */

typedef struct PairwiseContext PairwiseContext;
struct PairwiseContext {
    /* 1. Formatting context established by parent */
    FormattingContextType formatting_context;

    /* 2-3. Containing block dimensions */
    float containing_block_width;
    float containing_block_height;

    /* 4-5. Positioned containing blocks (references up the tree) */
    const DomNode *positioned_containing_block;  /* for absolute */
    const DomNode *fixed_containing_block;        /* for fixed */

    /* 6. Stacking context */
    const DomNode *stacking_context;

    /* 7-8. Writing mode / direction (inherited) */
    WritingMode writing_mode;
    Direction   direction;

    /* 9-12. Inherited text/font properties */
    float       font_size;
    float       line_height;   /* as a multiplier */
    CssColor    color;
    TextAlign   text_align;

    /* 13. White space handling */
    WhiteSpace  white_space;

    /* 14. Visibility (inherited) */
    Visibility  visibility;

    /* 15-16. Available space for child layout */
    float       available_width;
    float       available_height;  /* -1 = auto/indefinite */

    /* 17. Is this the root element? */
    bool        is_root;

    /* 18. Border collapse (inherited in table context) */
    bool        border_collapse;

    /* 19. List style (inherited) */
    const char *list_style_type;

    /* 20. Column count for multicol */
    int         column_count;  /* 0 = not multicol */

    /* 21. Depth in DOM (for debugging) */
    int         depth;
};

/* ── API ────────────────────────────────────────────────────────────── */

/* Create the initial root context. */
PairwiseContext context_initial(float viewport_w, float viewport_h);

/* Perform a pairwise transition: given the parent's context and the
 * child element's computed style, produce the child's context.
 * This is the core operation of the engine.
 *
 *   child_ctx = context_transition(parent_ctx, child_node, child_style)
 */
PairwiseContext context_transition(const PairwiseContext *parent_ctx,
                                   const DomNode *child,
                                   const ComputedStyle *child_style);

/* Does this element establish a new block formatting context? */
bool context_creates_bfc(const ComputedStyle *style, HtmlTag tag);

/* Does this element establish a new stacking context? */
bool context_creates_stacking_ctx(const ComputedStyle *style);

/* Does this element become a containing block for positioned children? */
bool context_creates_containing_block(const ComputedStyle *style);

#endif /* PANE_CONTEXT_H */
