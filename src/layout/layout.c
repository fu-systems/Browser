/*
 * Pane — Layout Engine Implementation
 *
 * Pipeline: DOM → Style (cascade + computed) → Context transitions → Layout
 *
 * For each element:
 *   1. Collect matching CSS declarations (cascade)
 *   2. Resolve computed style (inheritance + initial values)
 *   3. Compute pairwise context transition
 *   4. Generate layout box
 *   5. Run formatting-context-specific layout
 */

#include "layout.h"
#include "block.h"
#include "inline.h"
#include "../style/computed.h"
#include "../style/context.h"
#include "../css/cascade.h"
#include <string.h>

/* ── Box type from display ─────────────────────────────────────────── */

static LayoutBoxType box_type_for_display(Display display)
{
    switch (display) {
    case DISPLAY_NONE:              return BOX_BLOCK; /* shouldn't create */
    case DISPLAY_BLOCK:
    case DISPLAY_FLOW_ROOT:
    case DISPLAY_LIST_ITEM:         return BOX_BLOCK;
    case DISPLAY_INLINE:            return BOX_INLINE;
    case DISPLAY_INLINE_BLOCK:      return BOX_INLINE_BLOCK;
    case DISPLAY_FLEX:
    case DISPLAY_INLINE_FLEX:       return BOX_FLEX;
    case DISPLAY_GRID:
    case DISPLAY_INLINE_GRID:       return BOX_GRID;
    case DISPLAY_TABLE:
    case DISPLAY_INLINE_TABLE:      return BOX_TABLE;
    case DISPLAY_TABLE_ROW:
    case DISPLAY_TABLE_ROW_GROUP:
    case DISPLAY_TABLE_HEADER_GROUP:
    case DISPLAY_TABLE_FOOTER_GROUP:
        return BOX_TABLE_ROW;
    case DISPLAY_TABLE_CELL:        return BOX_TABLE_CELL;
    case DISPLAY_TABLE_COLUMN:
    case DISPLAY_TABLE_COLUMN_GROUP:
    case DISPLAY_TABLE_CAPTION:     return BOX_BLOCK;
    case DISPLAY_CONTENTS:          return BOX_BLOCK;
    }
    return BOX_BLOCK;
}

/* ── Recursive tree builder ────────────────────────────────────────── */

static LayoutBox *build_layout_box(Document *doc,
                                    const DomNode *node,
                                    Stylesheet **stylesheets, int ss_count,
                                    const ComputedStyle *parent_style,
                                    const PairwiseContext *parent_ctx,
                                    float root_font_size,
                                    Arena *arena)
{
    if (!node) return NULL;

    /* Text nodes. */
    if (node->type == NODE_TEXT) {
        if (!node->text.data || node->text.len == 0) return NULL;

        /* Skip whitespace-only text in block contexts. */
        bool all_ws = true;
        for (size_t i = 0; i < node->text.len; i++) {
            unsigned char c = (unsigned char)node->text.data[i];
            if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
                all_ws = false;
                break;
            }
        }
        if (all_ws && parent_ctx->formatting_context == FC_BLOCK)
            return NULL;

        ComputedStyle *text_style = computed_style_create(arena);
        *text_style = *parent_style; /* inherit everything */

        LayoutBox *text_box = layout_box_create(arena, BOX_TEXT, node, text_style);
        text_box->text = node->text.data;
        text_box->text_len = node->text.len;
        return text_box;
    }

    /* Skip non-element nodes. */
    if (node->type != NODE_ELEMENT) return NULL;

    /* ── 1. Cascade: collect matching declarations ─────────────── */

    CascadeResult cascade = {0};
    for (int i = 0; i < ss_count; i++) {
        cascade_collect(stylesheets[i], node, &cascade, arena);
    }

    /* Inline styles. */
    const char *style_attr = elem_get_attr(node, "style");
    if (style_attr) {
        DeclBlock inline_db = css_parse_inline_style(style_attr, strlen(style_attr), arena);
        cascade_add_inline(&inline_db, &cascade, arena);
    }

    cascade_resolve(&cascade);

    /* ── 2. Computed style resolution ──────────────────────────── */

    ComputedStyle *style = computed_style_create(arena);

    /* Apply UA default display for this tag. */
    style->display = display_for_tag(node->elem.tag);

    computed_style_resolve(style, &cascade, parent_style, root_font_size);

    /* If cascade didn't set display, use the UA default. */
    if (!cascade_get(&cascade, CSS_PROP_DISPLAY)) {
        style->display = display_for_tag(node->elem.tag);
    }

    /* Skip display:none. */
    if (style->display == DISPLAY_NONE) return NULL;

    /* ── 3. Pairwise context transition ────────────────────────── */

    PairwiseContext child_ctx = context_transition(parent_ctx, node, style);

    /* ── 4. Create layout box ──────────────────────────────────── */

    LayoutBoxType box_type = box_type_for_display(style->display);
    LayoutBox *box = layout_box_create(arena, box_type, node, style);
    box->ctx = child_ctx;

    /* Attach computed style to DOM node for later access. */
    ((DomNode *)node)->computed_style = style;

    /* ── 5. Recursively build child boxes ──────────────────────── */

    for (DomNode *child = node->first_child; child; child = child->next_sibling) {
        LayoutBox *child_box = build_layout_box(
            doc, child, stylesheets, ss_count,
            style, &child_ctx, root_font_size, arena);

        if (child_box) {
            layout_box_append(box, child_box);
        }
    }

    return box;
}

/* ── Public API ────────────────────────────────────────────────────── */

LayoutTree *layout_build(Document *doc,
                         Stylesheet **stylesheets, int ss_count,
                         float viewport_width, float viewport_height)
{
    LayoutTree *tree = calloc(1, sizeof(LayoutTree));
    arena_init(&tree->arena, 0);
    tree->viewport_width = viewport_width;
    tree->viewport_height = viewport_height;

    /* Initial context. */
    PairwiseContext root_ctx = context_initial(viewport_width, viewport_height);
    const ComputedStyle *initial = computed_style_initial();
    float root_font_size = initial->font_size;

    /* Find the root element (usually <html>). */
    DomNode *root_elem = doc->html ? doc->html : doc->root->first_child;
    if (!root_elem) {
        tree->root = layout_box_create(&tree->arena, BOX_BLOCK, NULL, NULL);
        return tree;
    }

    /* Build layout tree. */
    tree->root = build_layout_box(doc, root_elem, stylesheets, ss_count,
                                   initial, &root_ctx, root_font_size,
                                   &tree->arena);

    if (!tree->root) {
        tree->root = layout_box_create(&tree->arena, BOX_BLOCK, NULL, NULL);
    }

    /* Run layout from root. */
    tree->root->rect.x = 0;
    tree->root->rect.y = 0;
    layout_block(tree->root, viewport_width, viewport_height, &tree->arena);

    return tree;
}

void layout_tree_free(LayoutTree *tree)
{
    if (!tree) return;
    arena_destroy(&tree->arena);
    free(tree);
}
