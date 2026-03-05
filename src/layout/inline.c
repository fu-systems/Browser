/*
 * Pane — Inline Layout Implementation
 *
 * Simplified inline formatting context. Text is laid out as a sequence
 * of words that wrap at the available width boundary.
 */

#include "inline.h"
#include <string.h>
#include <ctype.h>

/* Approximate character width based on font size. */
static float char_width(float font_size)
{
    return font_size * 0.6f;  /* rough average for proportional fonts */
}

/* Measure a text run width (simplified: character count × avg width). */
static float measure_text(const char *text, size_t len, float font_size)
{
    return (float)len * char_width(font_size);
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
            float text_w = measure_text(box->text, box->text_len, font_size);
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

        float x = 0;
        float y = 0;
        float max_x = 0;
        size_t pos = 0;

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

            float word_w = measure_text(text + word_start, word_end - word_start, font_size);

            /* Wrap if needed. */
            if (x + word_w > available_width && x > 0) {
                if (x > max_x) max_x = x;
                x = 0;
                y += line_h;
            }

            x += word_w;
            if (word_end < len && isspace((unsigned char)text[word_end - 1])) {
                x += char_width(font_size); /* space after word */
            }

            pos = word_end;
        }

        if (x > max_x) max_x = x;
        if (x > 0) y += line_h; /* last line */

        box->rect.width = max_x > 0 ? max_x : available_width;
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
