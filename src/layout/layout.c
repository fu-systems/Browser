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
#include "flex.h"
#include "../style/computed.h"
#include "../style/context.h"
#include "../css/cascade.h"
#include <string.h>
#include <stdlib.h>

/* ── Replaced element intrinsic sizes ────────────────────────────── */

static bool is_replaced_element(HtmlTag tag)
{
    switch (tag) {
    case TAG_IMG: case TAG_INPUT: case TAG_BUTTON:
    case TAG_SELECT: case TAG_TEXTAREA:
        return true;
    default:
        return false;
    }
}

static void apply_replaced_defaults(LayoutBox *box, const DomNode *node,
                                    Arena *arena)
{
    HtmlTag tag = node->elem.tag;
    float fs = box->style ? box->style->font_size : 16.0f;

    switch (tag) {
    case TAG_INPUT: {
        const char *type = elem_get_attr(node, "type");
        if (!type) type = "text";

        if (strcmp(type, "hidden") == 0) {
            box->style->display = DISPLAY_NONE;
            return;
        }
        if (strcmp(type, "submit") == 0 || strcmp(type, "button") == 0 ||
            strcmp(type, "reset") == 0) {
            const char *val = elem_get_attr(node, "value");
            if (!val) {
                if (strcmp(type, "submit") == 0) val = "Submit";
                else if (strcmp(type, "reset") == 0) val = "Reset";
                else val = "";
            }
            if (val[0]) {
                box->text = val;
                box->text_len = strlen(val);
            }
            box->type = BOX_INLINE_BLOCK;
            if (box->style) {
                if (box->padding.top < 4) box->padding.top = 4;
                if (box->padding.bottom < 4) box->padding.bottom = 4;
                if (box->padding.left < 8) box->padding.left = 8;
                if (box->padding.right < 8) box->padding.right = 8;
                if (box->border.top < 1) box->border.top = 1;
                if (box->border.bottom < 1) box->border.bottom = 1;
                if (box->border.left < 1) box->border.left = 1;
                if (box->border.right < 1) box->border.right = 1;
                box->style->border_top_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
                box->style->border_right_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
                box->style->border_bottom_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
                box->style->border_left_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
                box->style->background_color = (CssColor){0xEE,0xEE,0xEE,0xFF};
            }
            return;
        }
        if (strcmp(type, "checkbox") == 0 || strcmp(type, "radio") == 0) {
            box->type = BOX_INLINE_BLOCK;
            box->rect.width = fs;
            box->rect.height = fs;
            if (box->style) {
                if (box->border.top < 1) box->border.top = 1;
                if (box->border.bottom < 1) box->border.bottom = 1;
                if (box->border.left < 1) box->border.left = 1;
                if (box->border.right < 1) box->border.right = 1;
                box->style->border_top_color = (CssColor){0x99,0x99,0x99,0xFF};
                box->style->border_right_color = (CssColor){0x99,0x99,0x99,0xFF};
                box->style->border_bottom_color = (CssColor){0x99,0x99,0x99,0xFF};
                box->style->border_left_color = (CssColor){0x99,0x99,0x99,0xFF};
                box->style->background_color = (CssColor){0xFF,0xFF,0xFF,0xFF};
            }
            return;
        }
        /* Text-like input (text, search, email, password, etc.) */
        box->type = BOX_INLINE_BLOCK;
        box->rect.width = fs * 12;
        box->rect.height = fs * 1.4f;
        if (box->style) {
            if (box->padding.top < 2) box->padding.top = 2;
            if (box->padding.bottom < 2) box->padding.bottom = 2;
            if (box->padding.left < 4) box->padding.left = 4;
            if (box->padding.right < 4) box->padding.right = 4;
            if (box->border.top < 1) box->border.top = 1;
            if (box->border.bottom < 1) box->border.bottom = 1;
            if (box->border.left < 1) box->border.left = 1;
            if (box->border.right < 1) box->border.right = 1;
            box->style->border_top_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_right_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_bottom_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_left_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->background_color = (CssColor){0xFF,0xFF,0xFF,0xFF};
        }
        {
            const char *val = elem_get_attr(node, "value");
            if (!val || !val[0]) val = elem_get_attr(node, "placeholder");
            if (val && val[0]) {
                box->text = val;
                box->text_len = strlen(val);
            }
        }
        return;
    }
    case TAG_BUTTON:
        box->type = BOX_INLINE_BLOCK;
        if (box->style) {
            if (box->padding.top < 4) box->padding.top = 4;
            if (box->padding.bottom < 4) box->padding.bottom = 4;
            if (box->padding.left < 12) box->padding.left = 12;
            if (box->padding.right < 12) box->padding.right = 12;
            if (box->border.top < 1) box->border.top = 1;
            if (box->border.bottom < 1) box->border.bottom = 1;
            if (box->border.left < 1) box->border.left = 1;
            if (box->border.right < 1) box->border.right = 1;
            box->style->border_top_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
            box->style->border_right_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
            box->style->border_bottom_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
            box->style->border_left_color = (CssColor){0xAA,0xAA,0xAA,0xFF};
            box->style->background_color = (CssColor){0xEE,0xEE,0xEE,0xFF};
        }
        return;
    case TAG_SELECT:
        box->type = BOX_INLINE_BLOCK;
        box->rect.width = fs * 10;
        box->rect.height = fs * 1.4f;
        if (box->style) {
            if (box->padding.left < 4) box->padding.left = 4;
            if (box->padding.right < 16) box->padding.right = 16;
            if (box->border.top < 1) box->border.top = 1;
            if (box->border.bottom < 1) box->border.bottom = 1;
            if (box->border.left < 1) box->border.left = 1;
            if (box->border.right < 1) box->border.right = 1;
            box->style->border_top_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_right_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_bottom_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_left_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->background_color = (CssColor){0xFF,0xFF,0xFF,0xFF};
        }
        return;
    case TAG_TEXTAREA:
        box->type = BOX_INLINE_BLOCK;
        box->rect.width = fs * 20;
        box->rect.height = fs * 4;
        if (box->style) {
            if (box->padding.top < 2) box->padding.top = 2;
            if (box->padding.bottom < 2) box->padding.bottom = 2;
            if (box->padding.left < 4) box->padding.left = 4;
            if (box->padding.right < 4) box->padding.right = 4;
            if (box->border.top < 1) box->border.top = 1;
            if (box->border.bottom < 1) box->border.bottom = 1;
            if (box->border.left < 1) box->border.left = 1;
            if (box->border.right < 1) box->border.right = 1;
            box->style->border_top_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_right_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_bottom_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->border_left_color = (CssColor){0x99,0x99,0x99,0xFF};
            box->style->background_color = (CssColor){0xFF,0xFF,0xFF,0xFF};
        }
        return;
    case TAG_IMG: {
        box->type = BOX_INLINE_BLOCK;
        const char *alt = elem_get_attr(node, "alt");
        const char *w_attr = elem_get_attr(node, "width");
        const char *h_attr = elem_get_attr(node, "height");
        float w = w_attr ? (float)atoi(w_attr) : 0;
        float h = h_attr ? (float)atoi(h_attr) : 0;
        if (w <= 0) w = alt && alt[0] ? (float)strlen(alt) * fs * 0.6f + 8 : 50;
        if (h <= 0) h = alt && alt[0] ? fs * 1.4f + 4 : 50;
        box->rect.width = w;
        box->rect.height = h;
        if (alt && alt[0]) {
            box->text = alt;
            box->text_len = strlen(alt);
        }
        if (box->style) {
            if (box->border.top < 1) box->border.top = 1;
            if (box->border.bottom < 1) box->border.bottom = 1;
            if (box->border.left < 1) box->border.left = 1;
            if (box->border.right < 1) box->border.right = 1;
            box->style->border_top_color = (CssColor){0xCC,0xCC,0xCC,0xFF};
            box->style->border_right_color = (CssColor){0xCC,0xCC,0xCC,0xFF};
            box->style->border_bottom_color = (CssColor){0xCC,0xCC,0xCC,0xFF};
            box->style->border_left_color = (CssColor){0xCC,0xCC,0xCC,0xFF};
            box->style->background_color = (CssColor){0xF0,0xF0,0xF0,0xFF};
        }
        return;
    }
    default:
        return;
    }
}

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
    if (node->type == PANE_NODE_TEXT) {
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
    if (node->type != PANE_NODE_ELEMENT) return NULL;

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

    /* Attach computed style to DOM node for later access. */
    ((DomNode *)node)->computed_style = style;

    /* ── 3b. display:contents — skip this box, return children
     *    wrapped in an anonymous block so the parent can adopt them. */
    if (style->display == DISPLAY_CONTENTS) {
        LayoutBox *wrapper = layout_box_create(arena, BOX_ANONYMOUS_BLOCK, NULL,
                                                computed_style_create(arena));
        /* Inherit key properties for the wrapper. */
        *wrapper->style = *style;
        wrapper->style->display = DISPLAY_BLOCK;
        wrapper->style->margin = (EdgeSizes){0};
        wrapper->style->padding = (EdgeSizes){0};
        wrapper->style->border_width = (EdgeSizes){0};
        wrapper->style->background_color = (CssColor){0,0,0,0};
        wrapper->style->width = (CssValue){ .type = VAL_AUTO };
        wrapper->style->height = (CssValue){ .type = VAL_AUTO };
        wrapper->ctx = child_ctx;

        for (DomNode *child = node->first_child; child; child = child->next_sibling) {
            LayoutBox *child_box = build_layout_box(
                doc, child, stylesheets, ss_count,
                style, &child_ctx, root_font_size, arena);
            if (child_box) {
                layout_box_append(wrapper, child_box);
            }
        }
        return wrapper;
    }

    /* ── 4. Create layout box ──────────────────────────────────── */

    LayoutBoxType box_type = box_type_for_display(style->display);
    LayoutBox *box = layout_box_create(arena, box_type, node, style);
    box->ctx = child_ctx;

    /* ── 4b. Replaced element defaults ────────────────────────── */

    if (is_replaced_element(node->elem.tag)) {
        apply_replaced_defaults(box, node, arena);
        /* display:none may have been set (e.g. hidden input). */
        if (style->display == DISPLAY_NONE) return NULL;
    }

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
    if (tree->root->type == BOX_FLEX) {
        layout_flex(tree->root, viewport_width, viewport_height, &tree->arena);
    } else {
        layout_block(tree->root, viewport_width, viewport_height, &tree->arena);
    }

    return tree;
}

void layout_tree_free(LayoutTree *tree)
{
    if (!tree) return;
    arena_destroy(&tree->arena);
    free(tree);
}
