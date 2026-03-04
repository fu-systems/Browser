/*
 * Pane — Font Rendering Implementation (FreeType + Fontconfig)
 */

#include "font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Global FreeType library ───────────────────────────────────────── */

static FT_Library ft_lib = NULL;

/* ── Font structure ────────────────────────────────────────────────── */

struct PaneFont {
    FT_Face      face;
    float        size_px;
    GlyphBitmap  cached_glyph;  /* reused for font_render_glyph */
};

/* ── System init/shutdown ──────────────────────────────────────────── */

bool font_system_init(void)
{
    if (ft_lib) return true;

    FT_Error err = FT_Init_FreeType(&ft_lib);
    if (err) {
        fprintf(stderr, "pane: FreeType init failed (error %d)\n", err);
        return false;
    }

    if (!FcInit()) {
        fprintf(stderr, "pane: Fontconfig init failed\n");
        FT_Done_FreeType(ft_lib);
        ft_lib = NULL;
        return false;
    }

    return true;
}

void font_system_shutdown(void)
{
    FcFini();
    if (ft_lib) {
        FT_Done_FreeType(ft_lib);
        ft_lib = NULL;
    }
}

/* ── Font discovery via Fontconfig ─────────────────────────────────── */

static char *find_font_path(const char *family, FontStyle style)
{
    FcPattern *pat = FcPatternCreate();

    if (family && *family) {
        FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)family);
    } else {
        FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)"sans-serif");
    }

    /* Style mapping. */
    if (style == FONT_STYLE_BOLD || style == FONT_STYLE_BOLD_ITALIC) {
        FcPatternAddInteger(pat, FC_WEIGHT, FC_WEIGHT_BOLD);
    }
    if (style == FONT_STYLE_ITALIC || style == FONT_STYLE_BOLD_ITALIC) {
        FcPatternAddInteger(pat, FC_SLANT, FC_SLANT_ITALIC);
    }

    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    FcResult result;
    FcPattern *match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);

    if (!match) return NULL;

    FcChar8 *file = NULL;
    if (FcPatternGetString(match, FC_FILE, 0, &file) != FcResultMatch || !file) {
        FcPatternDestroy(match);
        return NULL;
    }

    char *path = strdup((const char *)file);
    FcPatternDestroy(match);
    return path;
}

/* ── Font loading ──────────────────────────────────────────────────── */

PaneFont *font_load(const char *family, float size_px, FontStyle style)
{
    if (!ft_lib) return NULL;

    char *path = find_font_path(family, style);
    if (!path) {
        /* Ultimate fallback. */
        path = find_font_path("sans-serif", FONT_STYLE_NORMAL);
        if (!path) return NULL;
    }

    FT_Face face;
    FT_Error err = FT_New_Face(ft_lib, path, 0, &face);
    free(path);

    if (err) return NULL;

    PaneFont *font = calloc(1, sizeof(PaneFont));
    font->face = face;
    font->size_px = size_px;

    FT_Set_Pixel_Sizes(face, 0, (FT_UInt)size_px);

    return font;
}

void font_free(PaneFont *font)
{
    if (!font) return;
    if (font->face) FT_Done_Face(font->face);
    free(font->cached_glyph.buffer);
    free(font);
}

void font_set_size(PaneFont *font, float size_px)
{
    if (!font) return;
    font->size_px = size_px;
    FT_Set_Pixel_Sizes(font->face, 0, (FT_UInt)size_px);
}

/* ── Metrics ───────────────────────────────────────────────────────── */

float font_line_height(PaneFont *font)
{
    if (!font) return 16.0f;
    return (float)font->face->size->metrics.height / 64.0f;
}

float font_ascent(PaneFont *font)
{
    if (!font) return 12.0f;
    return (float)font->face->size->metrics.ascender / 64.0f;
}

float font_descent(PaneFont *font)
{
    if (!font) return 4.0f;
    return (float)(-font->face->size->metrics.descender) / 64.0f;
}

float font_char_width(PaneFont *font, uint32_t codepoint)
{
    if (!font) return 8.0f;

    FT_UInt glyph_idx = FT_Get_Char_Index(font->face, codepoint);
    if (!glyph_idx) glyph_idx = FT_Get_Char_Index(font->face, '?');

    FT_Error err = FT_Load_Glyph(font->face, glyph_idx, FT_LOAD_DEFAULT);
    if (err) return font->size_px * 0.6f;

    return (float)font->face->glyph->advance.x / 64.0f;
}

/* ── Text measurement ──────────────────────────────────────────────── */

/* Simple UTF-8 decoder. */
static uint32_t utf8_decode(const char **p, const char *end)
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

TextMetrics font_measure(PaneFont *font, const char *text, size_t len)
{
    TextMetrics m = {0};
    if (!font || !text || len == 0) return m;

    m.ascent  = font_ascent(font);
    m.descent = font_descent(font);
    m.height  = font_line_height(font);

    const char *p = text;
    const char *end = text + len;
    float x = 0;

    while (p < end) {
        uint32_t cp = utf8_decode(&p, end);
        if (cp == 0) break;
        x += font_char_width(font, cp);
    }

    m.width = x;
    return m;
}

/* ── Glyph rendering ──────────────────────────────────────────────── */

const GlyphBitmap *font_render_glyph(PaneFont *font, uint32_t codepoint)
{
    if (!font) return NULL;

    FT_UInt glyph_idx = FT_Get_Char_Index(font->face, codepoint);
    if (!glyph_idx) glyph_idx = FT_Get_Char_Index(font->face, '?');

    FT_Error err = FT_Load_Glyph(font->face, glyph_idx, FT_LOAD_RENDER);
    if (err) return NULL;

    FT_GlyphSlot slot = font->face->glyph;
    FT_Bitmap *bm = &slot->bitmap;

    /* Copy bitmap data into cached glyph. */
    size_t buf_size = (size_t)bm->width * bm->rows;
    if (buf_size > 0) {
        free(font->cached_glyph.buffer);
        font->cached_glyph.buffer = malloc(buf_size);

        /* FT bitmaps may have pitch != width; copy row by row. */
        for (unsigned int row = 0; row < bm->rows; row++) {
            memcpy(font->cached_glyph.buffer + row * bm->width,
                   bm->buffer + row * bm->pitch,
                   bm->width);
        }
    } else {
        free(font->cached_glyph.buffer);
        font->cached_glyph.buffer = NULL;
    }

    font->cached_glyph.width     = (int)bm->width;
    font->cached_glyph.height    = (int)bm->rows;
    font->cached_glyph.bearing_x = slot->bitmap_left;
    font->cached_glyph.bearing_y = slot->bitmap_top;
    font->cached_glyph.advance_x = (int)(slot->advance.x >> 6);

    return &font->cached_glyph;
}
