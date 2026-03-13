/*
 * Pane — Pairwise Context Transition Engine Implementation
 *
 * Core rule: (parent_context, child_element) → child_context
 *
 * The transition function applies rules from:
 *   - context_creators.json (formatting/stacking/containing block creators)
 *   - css_pair_chain_handling.md (CSS property interaction rules)
 *   - html_pair_chain_handling.md (HTML element nesting rules)
 */

#include "context.h"
#include <string.h>
#include <math.h>

/* ── Initial Context ───────────────────────────────────────────────── */

PairwiseContext context_initial(float viewport_w, float viewport_h)
{
    return (PairwiseContext){
        .formatting_context = FC_BLOCK,
        .containing_block_width = viewport_w,
        .containing_block_height = viewport_h,
        .positioned_containing_block = NULL,
        .fixed_containing_block = NULL,
        .stacking_context = NULL,
        .writing_mode = WM_HORIZONTAL_TB,
        .direction = DIR_LTR,
        .font_size = 16.0f,
        .line_height = 1.2f,
        .color = {0, 0, 0, 255},
        .text_align = TEXT_ALIGN_START,
        .white_space = WS_NORMAL,
        .visibility = VIS_VISIBLE,
        .available_width = viewport_w,
        .available_height = viewport_h,
        .is_root = true,
        .border_collapse = false,
        .list_style_type = "disc",
        .column_count = 0,
        .depth = 0,
    };
}

/* ── BFC Creation Detection ────────────────────────────────────────── */
/* Per context_creators.json: formatting_context_creators */

bool context_creates_bfc(const ComputedStyle *style, HtmlTag tag)
{
    /* Display-based BFC: flex, grid, table, flow-root. */
    switch (style->display) {
    case DISPLAY_FLEX: case DISPLAY_INLINE_FLEX:
    case DISPLAY_GRID: case DISPLAY_INLINE_GRID:
    case DISPLAY_TABLE: case DISPLAY_INLINE_TABLE:
    case DISPLAY_FLOW_ROOT:
        return true;
    default:
        break;
    }

    /* Position-based BFC. */
    if (style->position == POSITION_ABSOLUTE ||
        style->position == POSITION_FIXED)
        return true;

    /* Float-based BFC. */
    if (style->float_val != FLOAT_NONE)
        return true;

    /* Overflow-based BFC (not visible). */
    if (style->overflow_x != OVERFLOW_VISIBLE ||
        style->overflow_y != OVERFLOW_VISIBLE)
        return true;

    /* Inline-block. */
    if (style->display == DISPLAY_INLINE_BLOCK)
        return true;

    /* Table cell. */
    if (style->display == DISPLAY_TABLE_CELL)
        return true;

    /* Element-based BFC (from context_creators.json). */
    switch (tag) {
    case TAG_BODY: case TAG_TD: case TAG_TH:
    case TAG_BUTTON: case TAG_INPUT: case TAG_SELECT:
    case TAG_TEXTAREA: case TAG_FIELDSET: case TAG_DIALOG:
        return true;
    default:
        break;
    }

    return false;
}

/* ── Stacking Context Detection ────────────────────────────────────── */
/* Per context_creators.json: stacking_context_creators */

bool context_creates_stacking_ctx(const ComputedStyle *style)
{
    /* Positioned with z-index != auto. */
    if ((style->position == POSITION_RELATIVE ||
         style->position == POSITION_ABSOLUTE ||
         style->position == POSITION_FIXED ||
         style->position == POSITION_STICKY) &&
        style->z_index.type != VAL_AUTO)
        return true;

    /* Opacity < 1. */
    if (style->opacity < 1.0f)
        return true;

    /* Transform. */
    if (style->has_transform)
        return true;

    /* Flex/grid items with z-index != auto. */
    /* (Would need parent context to check — approximated by z_index check above.) */

    return false;
}

/* ── Containing Block Detection ────────────────────────────────────── */
/* Per context_creators.json: containing_block_creators */

bool context_creates_containing_block(const ComputedStyle *style)
{
    /* Positioned elements. */
    if (style->position == POSITION_RELATIVE ||
        style->position == POSITION_ABSOLUTE ||
        style->position == POSITION_FIXED ||
        style->position == POSITION_STICKY)
        return true;

    /* Transform/filter/perspective create CB for fixed/absolute descendants. */
    if (style->has_transform)
        return true;

    return false;
}

/* ── Formatting Context Determination ──────────────────────────────── */

static FormattingContextType fc_for_display(Display display)
{
    switch (display) {
    case DISPLAY_FLEX:
    case DISPLAY_INLINE_FLEX:
        return FC_FLEX;
    case DISPLAY_GRID:
    case DISPLAY_INLINE_GRID:
        return FC_GRID;
    case DISPLAY_TABLE:
    case DISPLAY_INLINE_TABLE:
        return FC_TABLE;
    case DISPLAY_TABLE_ROW:
    case DISPLAY_TABLE_ROW_GROUP:
    case DISPLAY_TABLE_HEADER_GROUP:
    case DISPLAY_TABLE_FOOTER_GROUP:
        return FC_TABLE_ROW;
    case DISPLAY_TABLE_CELL:
        return FC_TABLE_CELL;
    case DISPLAY_INLINE:
        return FC_INLINE;
    default:
        return FC_BLOCK;
    }
}

/* ── Core Transition Function ──────────────────────────────────────── */

PairwiseContext context_transition(const PairwiseContext *parent_ctx,
                                   const DomNode *child,
                                   const ComputedStyle *child_style)
{
    PairwiseContext ctx;

    /* Start by inheriting from parent. */
    ctx = *parent_ctx;
    ctx.is_root = false;
    ctx.depth = parent_ctx->depth + 1;

    /* Guard against NULL child_style. */
    if (!child_style) return ctx;

    /* Depth limit to prevent stack overflow on deeply nested DOM trees. */
    if (ctx.depth > 512) return ctx;

    /* ── Inherited property propagation ──────────────────────────── */

    /* These flow from parent → child unless overridden by the child's style. */
    ctx.font_size    = child_style->font_size;
    ctx.line_height  = child_style->line_height;
    ctx.color        = child_style->color;
    ctx.text_align   = child_style->text_align;
    ctx.white_space  = child_style->white_space;
    ctx.visibility   = child_style->visibility;
    ctx.writing_mode = child_style->writing_mode;
    ctx.direction    = child_style->direction;

    /* ── Display: none — skip subtree ────────────────────────────── */

    if (child_style->display == DISPLAY_NONE)
        return ctx;

    /* ── Formatting context transition ───────────────────────────── */
    /* Rule: The child element's display determines what formatting
     * context its OWN children see. */

    HtmlTag tag = (child->type == PANE_NODE_ELEMENT) ? child->elem.tag : TAG_UNKNOWN;

    /* Element-based FC overrides (SVG, MathML). */
    if (tag == TAG_SVG) {
        ctx.formatting_context = FC_SVG;
    } else if (tag == TAG_MATH) {
        ctx.formatting_context = FC_MATHML;
    } else if (child_style->display == DISPLAY_FLEX ||
               child_style->display == DISPLAY_INLINE_FLEX) {
        ctx.formatting_context = FC_FLEX;
    } else if (child_style->display == DISPLAY_GRID ||
               child_style->display == DISPLAY_INLINE_GRID) {
        ctx.formatting_context = FC_GRID;
    } else if (child_style->display == DISPLAY_TABLE ||
               child_style->display == DISPLAY_INLINE_TABLE) {
        ctx.formatting_context = FC_TABLE;
    } else if (child_style->display == DISPLAY_TABLE_ROW ||
               child_style->display == DISPLAY_TABLE_ROW_GROUP ||
               child_style->display == DISPLAY_TABLE_HEADER_GROUP ||
               child_style->display == DISPLAY_TABLE_FOOTER_GROUP) {
        ctx.formatting_context = FC_TABLE_ROW;
    } else if (child_style->display == DISPLAY_TABLE_CELL) {
        ctx.formatting_context = FC_TABLE_CELL;
    } else if (context_creates_bfc(child_style, tag)) {
        ctx.formatting_context = FC_BLOCK;
    }
    /* Otherwise: inherits parent's formatting context. */

    /* ── Containing block transition ─────────────────────────────── */
    /* The child's containing block depends on its position property. */

    if (child_style->position == POSITION_ABSOLUTE) {
        /* CB is the nearest positioned ancestor. */
        if (parent_ctx->positioned_containing_block) {
            /* Dimensions come from that ancestor; simplified here. */
        }
    } else if (child_style->position == POSITION_FIXED) {
        /* CB is viewport (or ancestor with transform). */
        ctx.containing_block_width = parent_ctx->containing_block_width;
        ctx.containing_block_height = parent_ctx->containing_block_height;
    }

    /* If this element creates a containing block, update for children. */
    if (context_creates_containing_block(child_style)) {
        ctx.positioned_containing_block = child;
    }

    /* If this element has transform, it becomes CB for fixed descendants too. */
    if (child_style->has_transform) {
        ctx.fixed_containing_block = child;
    }

    /* ── Stacking context transition ─────────────────────────────── */

    if (context_creates_stacking_ctx(child_style)) {
        ctx.stacking_context = child;
    }

    /* ── Available space calculation ──────────────────────────────── */
    /* The available width for children = parent's available width
     * minus this element's horizontal margins/padding/borders. */

    float h_padding = child_style->padding.left + child_style->padding.right;
    float h_border  = child_style->border_width.left + child_style->border_width.right;

    if (child_style->width.type != VAL_AUTO) {
        /* Explicit width. */
        float w = 0;
        if (child_style->width.type == VAL_LENGTH) {
            w = css_length_to_px(child_style->width, ctx.font_size, 16.0f,
                                  parent_ctx->containing_block_width,
                                  parent_ctx->containing_block_height,
                                  parent_ctx->available_width);
        } else if (child_style->width.type == VAL_PERCENTAGE) {
            w = child_style->width.percentage * parent_ctx->available_width / 100.0f;
        }
        if (child_style->box_sizing == BOX_BORDER_BOX) {
            ctx.available_width = w - h_padding - h_border;
        } else {
            ctx.available_width = w;
        }
        ctx.containing_block_width = ctx.available_width;
    } else {
        /* Auto width: fill available space. */
        float margin_h = child_style->margin.left + child_style->margin.right;
        ctx.available_width = parent_ctx->available_width - h_padding - h_border - margin_h;
        if (ctx.available_width < 0) ctx.available_width = 0;
        ctx.containing_block_width = ctx.available_width;
    }

    /* Available height: usually auto (indefinite). */
    if (child_style->height.type != VAL_AUTO) {
        float h = 0;
        if (child_style->height.type == VAL_LENGTH) {
            h = css_length_to_px(child_style->height, ctx.font_size, 16.0f,
                                  parent_ctx->containing_block_width,
                                  parent_ctx->containing_block_height,
                                  parent_ctx->available_height);
        } else if (child_style->height.type == VAL_PERCENTAGE &&
                   parent_ctx->available_height >= 0) {
            h = child_style->height.percentage * parent_ctx->available_height / 100.0f;
        }
        float v_padding = child_style->padding.top + child_style->padding.bottom;
        float v_border  = child_style->border_width.top + child_style->border_width.bottom;
        if (child_style->box_sizing == BOX_BORDER_BOX) {
            ctx.available_height = h - v_padding - v_border;
        } else {
            ctx.available_height = h;
        }
        ctx.containing_block_height = ctx.available_height;
    } else {
        ctx.available_height = -1; /* indefinite */
    }

    /* ── Table-specific context ──────────────────────────────────── */

    if (child_style->display == DISPLAY_TABLE ||
        child_style->display == DISPLAY_INLINE_TABLE) {
        /* border-collapse is inherited within tables. */
        if (child_style->values[CSS_PROP_BORDER_COLLAPSE].type == VAL_KEYWORD &&
            child_style->values[CSS_PROP_BORDER_COLLAPSE].string &&
            strcmp(child_style->values[CSS_PROP_BORDER_COLLAPSE].string, "collapse") == 0) {
            ctx.border_collapse = true;
        } else {
            ctx.border_collapse = false;
        }
    }

    /* ── Multicol context ────────────────────────────────────────── */

    if (child_style->values[CSS_PROP_COLUMN_COUNT].type == VAL_NUMBER) {
        ctx.column_count = (int)child_style->values[CSS_PROP_COLUMN_COUNT].number;
    }

    return ctx;
}
