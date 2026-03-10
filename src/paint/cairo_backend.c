/*
 * Pane — Cairo Paint Backend Implementation
 *
 * Renders layout boxes to a Cairo surface using FreeType fonts.
 */

#include "cairo_backend.h"
#include "../style/computed.h"
#include <string.h>
#include <ctype.h>
#include <math.h>

/* ── Init/Destroy ──────────────────────────────────────────────────── */

void cairo_renderer_init(CairoRenderer *r, cairo_t *cr, float font_size)
{
    memset(r, 0, sizeof(*r));
    r->cr = cr;
    r->fonts.base_size = font_size;

    r->fonts.regular     = font_load("sans-serif", font_size, FONT_STYLE_NORMAL);
    r->fonts.bold        = font_load("sans-serif", font_size, FONT_STYLE_BOLD);
    r->fonts.italic      = font_load("sans-serif", font_size, FONT_STYLE_ITALIC);
    r->fonts.bold_italic = font_load("sans-serif", font_size, FONT_STYLE_BOLD_ITALIC);
    r->fonts.monospace   = font_load("monospace",  font_size, FONT_STYLE_NORMAL);
}

void cairo_renderer_destroy(CairoRenderer *r)
{
    font_free(r->fonts.regular);
    font_free(r->fonts.bold);
    font_free(r->fonts.italic);
    font_free(r->fonts.bold_italic);
    font_free(r->fonts.monospace);
    memset(r, 0, sizeof(*r));
}

/* ── Helpers ───────────────────────────────────────────────────────── */

static void set_color(cairo_t *cr, CssColor c)
{
    cairo_set_source_rgba(cr,
        c.r / 255.0, c.g / 255.0, c.b / 255.0, c.a / 255.0);
}

static PaneFont *select_font(CairoRenderer *r, const ComputedStyle *style)
{
    if (!style) return r->fonts.regular;

    /* Check for monospace family. */
    if (style->font_family &&
        (strcmp(style->font_family, "monospace") == 0 ||
         strcmp(style->font_family, "Courier") == 0 ||
         strcmp(style->font_family, "Courier New") == 0)) {
        PaneFont *f = r->fonts.monospace;
        if (f) {
            font_set_size(f, style->font_size);
            return f;
        }
    }

    bool bold = style->font_weight >= 600;
    /* Simplified italic detection. */
    bool italic = false;

    PaneFont *f;
    if (bold && italic)     f = r->fonts.bold_italic;
    else if (bold)          f = r->fonts.bold;
    else if (italic)        f = r->fonts.italic;
    else                    f = r->fonts.regular;

    if (!f) f = r->fonts.regular;
    if (f) font_set_size(f, style->font_size);
    return f;
}

/* ── UTF-8 decoder ─────────────────────────────────────────────────── */

static uint32_t utf8_next(const char **p, const char *end)
{
    const unsigned char *s = (const unsigned char *)*p;
    if (s >= (const unsigned char *)end) return 0;

    uint32_t cp;
    int extra;

    if (*s < 0x80)      { cp = *s; extra = 0; }
    else if (*s < 0xC0) { cp = '?'; extra = 0; }
    else if (*s < 0xE0) { cp = *s & 0x1F; extra = 1; }
    else if (*s < 0xF0) { cp = *s & 0x0F; extra = 2; }
    else                { cp = *s & 0x07; extra = 3; }

    s++;
    for (int i = 0; i < extra && s < (const unsigned char *)end; i++, s++) {
        if ((*s & 0xC0) != 0x80) break;
        cp = (cp << 6) | (*s & 0x3F);
    }

    *p = (const char *)s;
    return cp;
}

/* ── Glyph blitting helper ─────────────────────────────────────────── */

static void render_glyph_at(CairoRenderer *r, PaneFont *font,
                            uint32_t cp, float pen_x, float baseline_y,
                            CssColor color)
{
    const GlyphBitmap *glyph = font_render_glyph(font, cp);
    if (!glyph || !glyph->buffer) return;

    cairo_t *cr = r->cr;
    int gw = glyph->width;
    int gh = glyph->height;

    if (gw > 0 && gh > 0) {
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, gw);
        unsigned char *pixels = calloc(1, stride * gh);

        for (int row = 0; row < gh; row++) {
            uint32_t *dst = (uint32_t *)(pixels + row * stride);
            const uint8_t *src = glyph->buffer + row * gw;
            for (int col = 0; col < gw; col++) {
                uint8_t a = src[col];
                if (a == 0) continue;
                uint8_t pr = (uint8_t)(color.r * a / 255);
                uint8_t pg = (uint8_t)(color.g * a / 255);
                uint8_t pb = (uint8_t)(color.b * a / 255);
                dst[col] = ((uint32_t)a << 24) | ((uint32_t)pr << 16) |
                           ((uint32_t)pg << 8) | pb;
            }
        }

        cairo_surface_t *surf = cairo_image_surface_create_for_data(
            pixels, CAIRO_FORMAT_ARGB32, gw, gh, stride);

        float gx = pen_x + glyph->bearing_x;
        float gy = baseline_y - glyph->bearing_y;

        cairo_set_source_surface(cr, surf, gx - r->scroll_x, gy - r->scroll_y);
        cairo_paint(cr);

        cairo_surface_destroy(surf);
        free(pixels);
    }
}

/* Advance pen_x for a codepoint, returning the advance width. */
static float glyph_advance(PaneFont *font, uint32_t cp)
{
    const GlyphBitmap *glyph = font_render_glyph(font, cp);
    return glyph ? (float)glyph->advance_x : font_char_width(font, cp);
}

/* ── Text rendering with FreeType glyphs blitted via Cairo ─────────── */

/* max_width > 0 enables word-wrapping at that width. */
static void render_text_ft(CairoRenderer *r, PaneFont *font,
                           const char *text, size_t len,
                           float x, float y, CssColor color,
                           float max_width)
{
    if (!font || !text || len == 0) return;

    float line_h = font_line_height(font);
    float baseline_y = y + font_ascent(font);
    float pen_x = x;
    float start_x = x;

    const char *p = text;
    const char *end = text + len;

    /* No wrapping: simple character-by-character render. */
    if (max_width <= 0) {
        while (p < end) {
            uint32_t cp = utf8_next(&p, end);
            if (cp == 0) break;
            if (cp < 32 && cp != '\t') continue;
            if (cp == '\n' || cp == '\r') continue;
            if (cp == '\t') {
                pen_x += font_char_width(font, ' ') * 4;
                continue;
            }
            render_glyph_at(r, font, cp, pen_x, baseline_y, color);
            pen_x += glyph_advance(font, cp);
        }
        return;
    }

    /* Word-wrapping render. */
    while (p < end) {
        /* Skip leading spaces at start of line. */
        if (pen_x == start_x) {
            while (p < end && *p == ' ') p++;
        }
        if (p >= end) break;

        /* Remember where we are — we'll scan ahead to measure the word. */
        const char *seg_start = p;

        /* Count leading whitespace (inter-word spaces). */
        int space_count = 0;
        while (p < end && *p == ' ') { p++; space_count++; }

        /* Scan the word (non-space, non-newline characters). */
        const char *word_begin = p;
        float word_w = 0;
        const char *scan = p;
        while (scan < end && *scan != ' ' && *scan != '\n' && *scan != '\r') {
            const char *prev = scan;
            uint32_t cp = utf8_next(&scan, end);
            if (cp == 0) break;
            if (cp >= 32)
                word_w += font_char_width(font, cp);
        }
        const char *word_end = scan;
        p = scan;

        /* Measure the inter-word space. */
        float space_w = 0;
        if (space_count > 0 && pen_x > start_x)
            space_w = font_char_width(font, ' ') * (float)space_count;

        /* Wrap if this word would exceed the line. */
        if (pen_x + space_w + word_w > start_x + max_width && pen_x > start_x) {
            pen_x = start_x;
            baseline_y += line_h;
            space_w = 0;
        }

        pen_x += space_w;

        /* Render word glyphs. */
        const char *rp = word_begin;
        while (rp < word_end) {
            uint32_t cp = utf8_next(&rp, end);
            if (cp == 0) break;
            if (cp < 32) continue;
            render_glyph_at(r, font, cp, pen_x, baseline_y, color);
            pen_x += glyph_advance(font, cp);
        }

        /* Handle explicit newlines. */
        if (p < end && (*p == '\n' || *p == '\r')) {
            pen_x = start_x;
            baseline_y += line_h;
            p++;
            if (p < end && *(p-1) == '\r' && *p == '\n') p++;
        }
    }
}

/* ── Render a single layout box ────────────────────────────────────── */

static void render_box(CairoRenderer *r, const LayoutBox *box,
                       float ox, float oy)
{
    if (!box) return;
    if (box->style && box->style->display == DISPLAY_NONE) return;
    if (box->style && box->style->visibility != VIS_VISIBLE) return;

    cairo_t *cr = r->cr;
    ComputedStyle *s = box->style;

    float x = ox + box->rect.x;
    float y = oy + box->rect.y;

    float bx = x - (box->padding.left + box->border.left);
    float by = y - (box->padding.top + box->border.top);
    float bw = box->border.left + box->padding.left + box->rect.width +
               box->padding.right + box->border.right;
    float bh = box->border.top + box->padding.top + box->rect.height +
               box->padding.bottom + box->border.bottom;

    /* Apply scroll offset. */
    float sx = bx - r->scroll_x;
    float sy = by - r->scroll_y;

    /* Background. */
    if (s && s->background_color.a > 0) {
        set_color(cr, s->background_color);
        cairo_rectangle(cr, sx, sy, bw, bh);
        cairo_fill(cr);
    }

    /* Borders. */
    if (box->border.top > 0 && s) {
        set_color(cr, s->border_top_color);
        cairo_rectangle(cr, sx, sy, bw, box->border.top);
        cairo_fill(cr);
    }
    if (box->border.bottom > 0 && s) {
        set_color(cr, s->border_bottom_color);
        cairo_rectangle(cr, sx, sy + bh - box->border.bottom,
                        bw, box->border.bottom);
        cairo_fill(cr);
    }
    if (box->border.left > 0 && s) {
        set_color(cr, s->border_left_color);
        cairo_rectangle(cr, sx, sy, box->border.left, bh);
        cairo_fill(cr);
    }
    if (box->border.right > 0 && s) {
        set_color(cr, s->border_right_color);
        cairo_rectangle(cr, sx + bw - box->border.right, sy,
                        box->border.right, bh);
        cairo_fill(cr);
    }

    /* Text content (BOX_TEXT nodes and replaced elements with text like buttons/inputs). */
    if (box->text && box->text_len > 0 && s) {
        PaneFont *font = select_font(r, s);
        if (font) {
            /* BOX_TEXT nodes use word wrapping at the box content width. */
            float wrap_w = (box->type == BOX_TEXT) ? box->rect.width : 0;
            render_text_ft(r, font, box->text, box->text_len,
                           x, y, s->color, wrap_w);
        }
    }

    /* Clip children if overflow is hidden. */
    bool needs_clip = s && (s->overflow_x == OVERFLOW_HIDDEN ||
                            s->overflow_y == OVERFLOW_HIDDEN);
    if (needs_clip) {
        cairo_save(cr);
        cairo_rectangle(cr, sx, sy, bw, bh);
        cairo_clip(cr);
    }

    /* Render children. */
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        render_box(r, child, x, y);
    }

    if (needs_clip) {
        cairo_restore(cr);
    }
}

/* ── Public API ────────────────────────────────────────────────────── */

void cairo_render_clear(CairoRenderer *r, float bg_r, float bg_g, float bg_b)
{
    cairo_set_source_rgb(r->cr, bg_r, bg_g, bg_b);
    cairo_paint(r->cr);
}

void cairo_render_layout(CairoRenderer *r, const LayoutBox *root)
{
    render_box(r, root, 0, 0);
}

void cairo_render_display_list(CairoRenderer *r, const DisplayList *dl)
{
    if (!dl) return;
    cairo_t *cr = r->cr;

    for (int i = 0; i < dl->count; i++) {
        const PaintCmd *cmd = &dl->cmds[i];
        float sx = cmd->rect.x - r->scroll_x;
        float sy = cmd->rect.y - r->scroll_y;

        switch (cmd->type) {
        case PAINT_RECT:
            set_color(cr, cmd->fill_rect.color);
            cairo_rectangle(cr, sx, sy, cmd->rect.width, cmd->rect.height);
            cairo_fill(cr);
            break;

        case PAINT_BORDER: {
            float w = cmd->rect.width;
            float h = cmd->rect.height;
            EdgeSizes bw = cmd->border.widths;

            if (bw.top > 0) {
                set_color(cr, cmd->border.top_color);
                cairo_rectangle(cr, sx, sy, w, bw.top);
                cairo_fill(cr);
            }
            if (bw.bottom > 0) {
                set_color(cr, cmd->border.bottom_color);
                cairo_rectangle(cr, sx, sy + h - bw.bottom, w, bw.bottom);
                cairo_fill(cr);
            }
            if (bw.left > 0) {
                set_color(cr, cmd->border.left_color);
                cairo_rectangle(cr, sx, sy, bw.left, h);
                cairo_fill(cr);
            }
            if (bw.right > 0) {
                set_color(cr, cmd->border.right_color);
                cairo_rectangle(cr, sx + w - bw.right, sy, bw.right, h);
                cairo_fill(cr);
            }
            break;
        }

        case PAINT_TEXT:
            if (cmd->text.text && cmd->text.len > 0 && r->fonts.regular) {
                PaneFont *f = r->fonts.regular;
                font_set_size(f, cmd->text.font_size);
                render_text_ft(r, f, cmd->text.text, cmd->text.len,
                              cmd->rect.x, cmd->rect.y, cmd->text.color,
                              cmd->rect.width);
            }
            break;
        }
    }
}
