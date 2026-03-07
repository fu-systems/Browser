/*
 * Pane — Inline Layout Implementation
 *
 * Simplified inline formatting context. Text is laid out as a sequence
 * of words that wrap at the available width boundary.
 */

#include "inline.h"
#include "../style/computed.h"
#include <string.h>
#include <ctype.h>

/* ── Text measurement callback ─────────────────────────────────────── */

static LayoutMeasureTextFn g_measure_fn = NULL;

void layout_set_measure_fn(LayoutMeasureTextFn fn)
{
    g_measure_fn = fn;
}

/* Approximate character width based on font size. */
static float char_width(float font_size)
{
    return font_size * 0.6f;  /* rough average for proportional fonts */
}

/* Measure a text run width using real font metrics if available. */
static float measure_text_ex(const char *text, size_t len, float font_size,
                              bool bold, bool monospace)
{
    if (g_measure_fn) {
        return g_measure_fn(text, len, font_size, bold, monospace);
    }
    return (float)len * char_width(font_size);
}

/* Convenience: measure with style info from a box. */
static float measure_text_styled(const char *text, size_t len,
                                  const ComputedStyle *style)
{
    float fs = style ? style->font_size : 16.0f;
    bool bold = style && style->font_weight >= 600;
    bool mono = style && style->font_family &&
                (strcmp(style->font_family, "monospace") == 0 ||
                 strcmp(style->font_family, "Courier") == 0 ||
                 strcmp(style->font_family, "Courier New") == 0);
    return measure_text_ex(text, len, fs, bold, mono);
}

/* Find the next word break position. */
static size_t next_word_break(const char *text, size_t len, size_t start)
{
    size_t i = start;
    /* Skip whitespace. */
    while (i < len && isspace((unsigned char)text[i])) i++;
    /* Skip word. */
    while (i < len && !isspace((unsigned char)text[i])) i++;
    return i;
}

void layout_inline(LayoutBox *box, float available_width, Arena *arena)
{
    (void)arena;

    if (!box->style) {
        box->rect.width = 0;
        box->rect.height = 0;
        return;
    }

    float font_size = box->style->font_size;
    float line_h = font_size * box->style->line_height;

    /* Replaced inline-block elements (input, img, etc.) with preset dimensions. */
    if (box->type == BOX_INLINE_BLOCK && box->rect.width > 0 && box->rect.height > 0
        && !box->first_child) {
        /* Dimensions already set by apply_replaced_defaults. */
        /* If there's text to display (value/alt), measure it for the text run. */
        if (box->text && box->text_len > 0) {
            float text_w = measure_text_styled(box->text, box->text_len, box->style);
            /* For button-style elements, auto-size width to text. */
            float total_pad = box->padding.left + box->padding.right +
                              box->border.left + box->border.right;
            if (text_w + total_pad > box->rect.width)
                box->rect.width = text_w + total_pad;
        }
        return;
    }

    if (box->type == BOX_TEXT && box->text && box->text_len > 0) {
        /* Simple text layout: wrap words within available_width. */
        const char *text = box->text;
        size_t len = box->text_len;
        WhiteSpace ws = box->style->white_space;
        bool no_wrap = (ws == WS_NOWRAP || ws == WS_PRE);
        bool preserve_spaces = (ws == WS_PRE || ws == WS_PRE_WRAP || ws == WS_BREAK_SPACES);

        float x = 0;
        float y = 0;
        float max_x = 0;
        size_t pos = 0;

        if (preserve_spaces) {
            /* Pre: preserve all whitespace and newlines. */
            while (pos < len) {
                if (text[pos] == '\n') {
                    if (x > max_x) max_x = x;
                    x = 0;
                    y += line_h;
                    pos++;
                    continue;
                }
                /* Measure until next newline or end. */
                size_t line_end = pos;
                while (line_end < len && text[line_end] != '\n') line_end++;
                float run_w = measure_text_styled(text + pos, line_end - pos, box->style);
                x += run_w;
                pos = line_end;
            }
        } else {
            while (pos < len) {
                /* Skip leading whitespace on a new line. */
                if (x == 0) {
                    while (pos < len && text[pos] == ' ') pos++;
                }

                size_t word_end = next_word_break(text, len, pos);
                size_t word_start = pos;
                /* Skip whitespace before word. */
                while (word_start < word_end && isspace((unsigned char)text[word_start]))
                    word_start++;

                float word_w = measure_text_styled(text + word_start, word_end - word_start, box->style);

                /* Wrap if needed (unless nowrap). */
                if (!no_wrap && x + word_w > available_width && x > 0) {
                    if (x > max_x) max_x = x;
                    x = 0;
                    y += line_h;
                }

                x += word_w;
                if (word_end < len && isspace((unsigned char)text[word_end])) {
                    x += measure_text_styled(" ", 1, box->style); /* space after word */
                }

                pos = word_end;
            }
        }

        if (x > max_x) max_x = x;
        if (x > 0) y += line_h; /* last line */

        box->rect.width = max_x > 0 ? max_x : (no_wrap ? 0 : available_width);
        box->rect.height = y;
    } else if (box->type == BOX_INLINE || box->type == BOX_INLINE_BLOCK) {
        /* Inline box: lay out inline children. */
        float x = 0;
        float y = 0;
        float max_x = 0;
        float cur_line_h = line_h; /* track max height on current line */

        for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
            layout_inline(child, available_width - x, arena);

            float child_outer_w = child->rect.width + child->padding.left + child->padding.right
                                + child->border.left + child->border.right
                                + child->margin.left + child->margin.right;
            float child_h = child->rect.height + child->padding.top + child->padding.bottom
                          + child->border.top + child->border.bottom
                          + child->margin.top + child->margin.bottom;

            if (x + child_outer_w > available_width && x > 0) {
                if (x > max_x) max_x = x;
                x = 0;
                y += cur_line_h;
                cur_line_h = line_h;
            }

            child->rect.x = x + child->margin.left + child->border.left + child->padding.left;
            child->rect.y = y + child->margin.top + child->border.top + child->padding.top;
            x += child_outer_w;
            if (child_h > cur_line_h) cur_line_h = child_h;
        }

        if (x > max_x) max_x = x;
        if (x > 0 || box->first_child) y += cur_line_h;

        box->rect.width = max_x;
        box->rect.height = y;
    } else {
        /* Fallback. */
        box->rect.width = available_width;
        box->rect.height = line_h;
    }
}
