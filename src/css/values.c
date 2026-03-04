/*
 * Pane — CSS Value Implementation
 */

#include "values.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

const CssValue css_value_auto    = { .type = VAL_AUTO };
const CssValue css_value_none    = { .type = VAL_NONE };
const CssValue css_value_initial = { .type = VAL_INITIAL };
const CssValue css_value_inherit = { .type = VAL_INHERIT };
const CssValue css_value_zero    = { .type = VAL_LENGTH, .length = { 0.0f, UNIT_PX } };

float css_length_to_px(CssValue val, float font_size, float root_font_size,
                       float vw, float vh, float cb_size)
{
    if (val.type == VAL_NUMBER) return val.number;
    if (val.type == VAL_PERCENTAGE) return val.percentage * cb_size / 100.0f;
    if (val.type != VAL_LENGTH) return 0.0f;

    float m = val.length.magnitude;
    switch (val.length.unit) {
    case UNIT_PX:      return m;
    case UNIT_EM:      return m * font_size;
    case UNIT_REM:     return m * root_font_size;
    case UNIT_PERCENT: return m * cb_size / 100.0f;
    case UNIT_VW:      return m * vw / 100.0f;
    case UNIT_VH:      return m * vh / 100.0f;
    case UNIT_VMIN:    return m * fminf(vw, vh) / 100.0f;
    case UNIT_VMAX:    return m * fmaxf(vw, vh) / 100.0f;
    case UNIT_CM:      return m * 96.0f / 2.54f;
    case UNIT_MM:      return m * 96.0f / 25.4f;
    case UNIT_IN:      return m * 96.0f;
    case UNIT_PT:      return m * 96.0f / 72.0f;
    case UNIT_PC:      return m * 96.0f / 6.0f;
    case UNIT_CH:      return m * font_size * 0.5f;  /* approximation */
    case UNIT_EX:      return m * font_size * 0.5f;  /* approximation */
    case UNIT_LH:      return m * font_size * 1.2f;  /* approximation */
    default:           return m;
    }
}

/* ── Named Colors ───────────────────────────────────────────────────── */

typedef struct { const char *name; uint8_t r, g, b; } NamedColor;

static const NamedColor named_colors[] = {
    {"aliceblue",240,248,255}, {"antiquewhite",250,235,215},
    {"aqua",0,255,255}, {"aquamarine",127,255,212},
    {"azure",240,255,255}, {"beige",245,245,220},
    {"bisque",255,228,196}, {"black",0,0,0},
    {"blanchedalmond",255,235,205}, {"blue",0,0,255},
    {"blueviolet",138,43,226}, {"brown",165,42,42},
    {"burlywood",222,184,135}, {"cadetblue",95,158,160},
    {"chartreuse",127,255,0}, {"chocolate",210,105,30},
    {"coral",255,127,80}, {"cornflowerblue",100,149,237},
    {"cornsilk",255,248,220}, {"crimson",220,20,60},
    {"cyan",0,255,255}, {"darkblue",0,0,139},
    {"darkcyan",0,139,139}, {"darkgoldenrod",184,134,11},
    {"darkgray",169,169,169}, {"darkgreen",0,100,0},
    {"darkgrey",169,169,169}, {"darkkhaki",189,183,107},
    {"darkmagenta",139,0,139}, {"darkolivegreen",85,107,47},
    {"darkorange",255,140,0}, {"darkorchid",153,50,204},
    {"darkred",139,0,0}, {"darksalmon",233,150,122},
    {"darkseagreen",143,188,143}, {"darkslateblue",72,61,139},
    {"darkslategray",47,79,79}, {"darkslategrey",47,79,79},
    {"darkturquoise",0,206,209}, {"darkviolet",148,0,211},
    {"deeppink",255,20,147}, {"deepskyblue",0,191,255},
    {"dimgray",105,105,105}, {"dimgrey",105,105,105},
    {"dodgerblue",30,144,255}, {"firebrick",178,34,34},
    {"floralwhite",255,250,240}, {"forestgreen",34,139,34},
    {"fuchsia",255,0,255}, {"gainsboro",220,220,220},
    {"ghostwhite",248,248,255}, {"gold",255,215,0},
    {"goldenrod",218,165,32}, {"gray",128,128,128},
    {"green",0,128,0}, {"greenyellow",173,255,47},
    {"grey",128,128,128}, {"honeydew",240,255,240},
    {"hotpink",255,105,180}, {"indianred",205,92,92},
    {"indigo",75,0,130}, {"ivory",255,255,240},
    {"khaki",240,230,140}, {"lavender",230,230,250},
    {"lavenderblush",255,240,245}, {"lawngreen",124,252,0},
    {"lemonchiffon",255,250,205}, {"lightblue",173,216,230},
    {"lightcoral",240,128,128}, {"lightcyan",224,255,255},
    {"lightgoldenrodyellow",250,250,210}, {"lightgray",211,211,211},
    {"lightgreen",144,238,144}, {"lightgrey",211,211,211},
    {"lightpink",255,182,193}, {"lightsalmon",255,160,122},
    {"lightseagreen",32,178,170}, {"lightskyblue",135,206,250},
    {"lightslategray",119,136,153}, {"lightslategrey",119,136,153},
    {"lightsteelblue",176,196,222}, {"lightyellow",255,255,224},
    {"lime",0,255,0}, {"limegreen",50,205,50},
    {"linen",250,240,230}, {"magenta",255,0,255},
    {"maroon",128,0,0}, {"mediumaquamarine",102,205,170},
    {"mediumblue",0,0,205}, {"mediumorchid",186,85,211},
    {"mediumpurple",147,112,219}, {"mediumseagreen",60,179,113},
    {"mediumslateblue",123,104,238}, {"mediumspringgreen",0,250,154},
    {"mediumturquoise",72,209,204}, {"mediumvioletred",199,21,133},
    {"midnightblue",25,25,112}, {"mintcream",245,255,250},
    {"mistyrose",255,228,225}, {"moccasin",255,228,181},
    {"navajowhite",255,222,173}, {"navy",0,0,128},
    {"oldlace",253,245,230}, {"olive",128,128,0},
    {"olivedrab",107,142,35}, {"orange",255,165,0},
    {"orangered",255,69,0}, {"orchid",218,112,214},
    {"palegoldenrod",238,232,170}, {"palegreen",152,251,152},
    {"paleturquoise",175,238,238}, {"palevioletred",219,112,147},
    {"papayawhip",255,239,213}, {"peachpuff",255,218,185},
    {"peru",205,133,63}, {"pink",255,192,203},
    {"plum",221,160,221}, {"powderblue",176,224,230},
    {"purple",128,0,128}, {"rebeccapurple",102,51,153},
    {"red",255,0,0}, {"rosybrown",188,143,143},
    {"royalblue",65,105,225}, {"saddlebrown",139,69,19},
    {"salmon",250,128,114}, {"sandybrown",244,164,96},
    {"seagreen",46,139,87}, {"seashell",255,245,238},
    {"sienna",160,82,45}, {"silver",192,192,192},
    {"skyblue",135,206,235}, {"slateblue",106,90,205},
    {"slategray",112,128,144}, {"slategrey",112,128,144},
    {"snow",255,250,250}, {"springgreen",0,255,127},
    {"steelblue",70,130,180}, {"tan",210,180,140},
    {"teal",0,128,128}, {"thistle",216,191,216},
    {"tomato",255,99,71}, {"turquoise",64,224,208},
    {"violet",238,130,238}, {"wheat",245,222,179},
    {"white",255,255,255}, {"whitesmoke",245,245,245},
    {"yellow",255,255,0}, {"yellowgreen",154,205,50},
};

#define NUM_NAMED_COLORS (sizeof(named_colors) / sizeof(named_colors[0]))

bool css_color_from_name(const char *name, size_t len, CssColor *out)
{
    /* Special keywords. */
    if (len == 11 && strncmp(name, "transparent", 11) == 0) {
        *out = CSS_COLOR_TRANSPARENT;
        return true;
    }
    if (len == 12 && strncmp(name, "currentcolor", 12) == 0) {
        /* Sentinel — resolved during style resolution. */
        *out = (CssColor){0, 0, 0, 1};
        return true;
    }

    char lower[32];
    if (len >= sizeof(lower)) return false;
    for (size_t i = 0; i < len; i++)
        lower[i] = (char)tolower((unsigned char)name[i]);
    lower[len] = '\0';

    for (size_t i = 0; i < NUM_NAMED_COLORS; i++) {
        if (strcmp(lower, named_colors[i].name) == 0) {
            *out = CSS_COLOR(named_colors[i].r, named_colors[i].g,
                             named_colors[i].b);
            return true;
        }
    }
    return false;
}

static int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'A';
    return -1;
}

bool css_color_from_hex(const char *hex, size_t len, CssColor *out)
{
    if (len == 0) return false;
    if (hex[0] == '#') { hex++; len--; }

    if (len == 3 || len == 4) {
        int r = hex_digit(hex[0]);
        int g = hex_digit(hex[1]);
        int b = hex_digit(hex[2]);
        int a = (len == 4) ? hex_digit(hex[3]) : 15;
        if (r < 0 || g < 0 || b < 0 || a < 0) return false;
        *out = (CssColor){ r*17, g*17, b*17, a*17 };
        return true;
    }

    if (len == 6 || len == 8) {
        int r = hex_digit(hex[0]) * 16 + hex_digit(hex[1]);
        int g = hex_digit(hex[2]) * 16 + hex_digit(hex[3]);
        int b = hex_digit(hex[4]) * 16 + hex_digit(hex[5]);
        int a = 255;
        if (len == 8)
            a = hex_digit(hex[6]) * 16 + hex_digit(hex[7]);
        if (r < 0 || g < 0 || b < 0 || a < 0) return false;
        *out = (CssColor){ r, g, b, a };
        return true;
    }

    return false;
}

CssColor css_parse_color_string(const char *str)
{
    CssColor c = {0, 0, 0, 0};
    if (!str || !str[0]) return c;

    size_t len = strlen(str);

    /* Try hex first. */
    if (str[0] == '#') {
        if (css_color_from_hex(str, len, &c)) return c;
        return (CssColor){0, 0, 0, 0};
    }

    /* Try named color. */
    if (css_color_from_name(str, len, &c)) return c;

    /* Try bare hex (HTML allows color="FF0000" without #). */
    if (len == 6 || len == 3) {
        if (css_color_from_hex(str, len, &c)) return c;
    }

    return (CssColor){0, 0, 0, 0};
}
