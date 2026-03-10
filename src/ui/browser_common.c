/*
 * Pane — Shared Browser Utilities (Cross-Platform)
 *
 * Platform-independent functions used by both the GTK3 (Linux) and
 * Win32 (Windows) browser UIs: URL resolution, UTF-8 sanitization,
 * hit-testing, link/form detection, external CSS fetching.
 */

#include "browser_common.h"
#include "../net/http.h"
#include "../util/compat.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

/* ── URL resolution ────────────────────────────────────────────────── */

void resolve_url(const char *base, const char *href, char *out, size_t out_sz)
{
    if (!href || !href[0]) { out[0] = '\0'; return; }

    /* Absolute URL */
    if (strncmp(href, "http://", 7) == 0 || strncmp(href, "https://", 8) == 0) {
        snprintf(out, out_sz, "%s", href);
        return;
    }

    /* Protocol-relative URL */
    if (href[0] == '/' && href[1] == '/') {
        /* Use same scheme as base. */
        if (strncmp(base, "https://", 8) == 0)
            snprintf(out, out_sz, "https:%s", href);
        else
            snprintf(out, out_sz, "http:%s", href);
        return;
    }

    /* Extract scheme + host from base. */
    const char *scheme_end = strstr(base, "://");
    if (!scheme_end) { out[0] = '\0'; return; }
    const char *host_start = scheme_end + 3;
    const char *path_start = strchr(host_start, '/');

    if (href[0] == '/') {
        /* Root-relative */
        size_t prefix_len = path_start ? (size_t)(path_start - base) : strlen(base);
        snprintf(out, out_sz, "%.*s%s", (int)prefix_len, base, href);
    } else {
        /* Relative to current path */
        if (path_start) {
            const char *last_slash = strrchr(path_start, '/');
            if (last_slash) {
                size_t prefix_len = (size_t)(last_slash - base) + 1;
                snprintf(out, out_sz, "%.*s%s", (int)prefix_len, base, href);
            } else {
                size_t prefix_len = strlen(base);
                snprintf(out, out_sz, "%.*s/%s", (int)prefix_len, base, href);
            }
        } else {
            snprintf(out, out_sz, "%s/%s", base, href);
        }
    }
}

/* ── UTF-8 sanitization ───────────────────────────────────────────── */

char *sanitize_utf8(char *buf, size_t len)
{
    if (!buf) return buf;
    unsigned char *s = (unsigned char *)buf;
    unsigned char *end = s + len;

    while (s < end) {
        if (*s < 0x80) {
            s++;
        } else if ((*s & 0xE0) == 0xC0) {
            if (s + 1 >= end || (s[1] & 0xC0) != 0x80) { *s++ = '?'; continue; }
            if (*s < 0xC2) { *s = '?'; s[1] = '?'; s += 2; continue; } /* overlong */
            s += 2;
        } else if ((*s & 0xF0) == 0xE0) {
            if (s + 2 >= end || (s[1] & 0xC0) != 0x80 || (s[2] & 0xC0) != 0x80) {
                *s++ = '?'; continue;
            }
            /* Reject overlong and surrogates. */
            uint32_t cp = ((*s & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                *s = '?'; s[1] = '?'; s[2] = '?'; s += 3; continue;
            }
            s += 3;
        } else if ((*s & 0xF8) == 0xF0) {
            if (s + 3 >= end || (s[1] & 0xC0) != 0x80 ||
                (s[2] & 0xC0) != 0x80 || (s[3] & 0xC0) != 0x80) {
                *s++ = '?'; continue;
            }
            uint32_t cp = ((*s & 0x07) << 18) | ((s[1] & 0x3F) << 12) |
                          ((s[2] & 0x3F) << 6) | (s[3] & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                *s = '?'; s[1] = '?'; s[2] = '?'; s[3] = '?'; s += 4; continue;
            }
            s += 4;
        } else {
            *s++ = '?';
        }
    }
    return buf;
}

/* ── Layout hit-testing ────────────────────────────────────────────── */

const LayoutBox *hit_test_box(const LayoutBox *box,
                               float px, float py,
                               float ox, float oy)
{
    if (!box) return NULL;
    if (box->style && box->style->display == DISPLAY_NONE) return NULL;

    float x = ox + box->rect.x;
    float y = oy + box->rect.y;

    float bx = x - (box->padding.left + box->border.left);
    float by = y - (box->padding.top + box->border.top);
    float bw = box->border.left + box->padding.left + box->rect.width +
               box->padding.right + box->border.right;
    float bh = box->border.top + box->padding.top + box->rect.height +
               box->padding.bottom + box->border.bottom;

    /* Check children first (front-to-back, last child is on top). */
    const LayoutBox *hit = NULL;
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        const LayoutBox *h = hit_test_box(child, px, py, x, y);
        if (h) hit = h;
    }
    if (hit) return hit;

    /* Check if point is inside this box. */
    if (px >= bx && px < bx + bw && py >= by && py < by + bh)
        return box;

    return NULL;
}

/* ── DOM ancestor helpers ──────────────────────────────────────────── */

const DomNode *find_link_ancestor(const LayoutBox *box)
{
    while (box) {
        if (box->node && box->node->type == PANE_NODE_ELEMENT &&
            box->node->elem.tag == TAG_A) {
            return box->node;
        }
        box = box->parent;
    }
    return NULL;
}

const DomNode *find_form_element(const LayoutBox *box)
{
    while (box) {
        if (box->node && box->node->type == PANE_NODE_ELEMENT) {
            HtmlTag tag = box->node->elem.tag;
            if (tag == TAG_INPUT || tag == TAG_TEXTAREA ||
                tag == TAG_BUTTON || tag == TAG_SELECT)
                return box->node;
        }
        box = box->parent;
    }
    return NULL;
}

const DomNode *find_form_parent(const DomNode *node)
{
    for (const DomNode *n = node->parent; n; n = n->parent) {
        if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_FORM)
            return n;
    }
    return NULL;
}

bool is_text_input(const DomNode *node)
{
    if (node->elem.tag == TAG_TEXTAREA) return true;
    if (node->elem.tag == TAG_INPUT) {
        const char *type = elem_get_attr(node, "type");
        if (!type) return true; /* default is text */
        return (strcmp(type, "text") == 0 || strcmp(type, "search") == 0 ||
                strcmp(type, "email") == 0 || strcmp(type, "password") == 0 ||
                strcmp(type, "url") == 0 || strcmp(type, "tel") == 0 ||
                strcmp(type, "number") == 0);
    }
    return false;
}

bool is_button_element(const DomNode *node)
{
    if (node->elem.tag == TAG_BUTTON) return true;
    if (node->elem.tag == TAG_INPUT) {
        const char *type = elem_get_attr(node, "type");
        if (type && (strcmp(type, "submit") == 0 || strcmp(type, "button") == 0 ||
                     strcmp(type, "reset") == 0))
            return true;
    }
    return false;
}

/* ── Layout box helpers ────────────────────────────────────────────── */

void layout_box_abs_position(const LayoutBox *box, float *out_x, float *out_y)
{
    float x = 0, y = 0;
    for (const LayoutBox *b = box; b; b = b->parent) {
        x += b->rect.x;
        y += b->rect.y;
    }
    *out_x = x;
    *out_y = y;
}

const LayoutBox *find_box_for_node(const LayoutBox *root, const DomNode *node)
{
    if (!root) return NULL;
    if (root->node == node) return root;
    for (LayoutBox *child = root->first_child; child; child = child->next_sibling) {
        const LayoutBox *found = find_box_for_node(child, node);
        if (found) return found;
    }
    return NULL;
}

/* ── External CSS fetching ─────────────────────────────────────────── */

char *fetch_external_css(const char *html, size_t html_len,
                         const char *base_url, size_t *out_len)
{
    /* Quick scan for <link ... rel="stylesheet" ... href="..."> in HTML.
     * This is a rough heuristic parser — good enough for real sites. */
    size_t css_cap = 4096;
    size_t css_len = 0;
    char *css_buf = malloc(css_cap);
    if (!css_buf) { *out_len = 0; return NULL; }
    css_buf[0] = '\0';

    const char *p = html;
    const char *end = html + html_len;
    int fetch_count = 0;
    const int MAX_CSS_FETCHES = 5;
    const size_t MAX_CSS_TOTAL = 512 * 1024;

    while (p < end) {
        /* Find <link */
        const char *link = p;
        while (link < end - 5) {
            if (link[0] == '<' &&
                (link[1] == 'l' || link[1] == 'L') &&
                (link[2] == 'i' || link[2] == 'I') &&
                (link[3] == 'n' || link[3] == 'N') &&
                (link[4] == 'k' || link[4] == 'K') &&
                (link[5] == ' ' || link[5] == '\t' || link[5] == '\n' || link[5] == '\r')) {
                break;
            }
            link++;
        }
        if (link >= end - 5) break;
        p = link + 1;

        /* Find end of tag. */
        const char *tag_end = memchr(link, '>', end - link);
        if (!tag_end) break;

        /* Check if rel="stylesheet" is present. */
        bool has_stylesheet = false;
        const char *rel = link;
        while (rel < tag_end - 3) {
            if ((rel[0] == 'r' || rel[0] == 'R') &&
                (rel[1] == 'e' || rel[1] == 'E') &&
                (rel[2] == 'l' || rel[2] == 'L') &&
                (rel[3] == '=' || rel[3] == ' ' || rel[3] == '\t')) {
                /* Find value. */
                const char *v = rel + 3;
                while (v < tag_end && (*v == ' ' || *v == '=' || *v == '\t')) v++;
                if (v < tag_end && (*v == '"' || *v == '\'')) {
                    char q = *v++;
                    const char *ve = memchr(v, q, tag_end - v);
                    if (ve) {
                        size_t vlen = ve - v;
                        if (vlen == 10 && strncasecmp(v, "stylesheet", 10) == 0)
                            has_stylesheet = true;
                    }
                }
                break;
            }
            rel++;
        }

        if (!has_stylesheet) { p = tag_end + 1; continue; }

        /* Extract href. */
        const char *href_start = NULL;
        size_t href_len = 0;
        const char *h = link;
        while (h < tag_end - 4) {
            if ((h[0] == 'h' || h[0] == 'H') &&
                (h[1] == 'r' || h[1] == 'R') &&
                (h[2] == 'e' || h[2] == 'E') &&
                (h[3] == 'f' || h[3] == 'F') &&
                (h[4] == '=' || h[4] == ' ' || h[4] == '\t')) {
                const char *v = h + 4;
                while (v < tag_end && (*v == ' ' || *v == '=' || *v == '\t')) v++;
                if (v < tag_end && (*v == '"' || *v == '\'')) {
                    char q = *v++;
                    const char *ve = memchr(v, q, tag_end - v);
                    if (ve) {
                        href_start = v;
                        href_len = ve - v;
                    }
                }
                break;
            }
            h++;
        }

        if (!href_start || href_len == 0) { p = tag_end + 1; continue; }

        /* Resolve URL. */
        char href_buf[512];
        if (href_len >= sizeof(href_buf)) { p = tag_end + 1; continue; }
        memcpy(href_buf, href_start, href_len);
        href_buf[href_len] = '\0';

        char resolved[2048];
        resolve_url(base_url, href_buf, resolved, sizeof(resolved));
        if (!resolved[0]) { p = tag_end + 1; continue; }

        /* Limit CSS fetches. */
        if (fetch_count >= MAX_CSS_FETCHES || css_len >= MAX_CSS_TOTAL) {
            p = tag_end + 1;
            continue;
        }

        /* Fetch CSS. */
        HttpResponse *css_resp = http_get(resolved, 3);
        fetch_count++;
        if (css_resp && !css_resp->error && css_resp->body && css_resp->body_len > 0) {
            size_t needed = css_len + css_resp->body_len + 2;
            while (needed > css_cap) {
                css_cap *= 2;
                char *new_buf = realloc(css_buf, css_cap);
                if (!new_buf) { http_response_free(css_resp); break; }
                css_buf = new_buf;
            }
            css_buf[css_len++] = '\n';
            memcpy(css_buf + css_len, css_resp->body, css_resp->body_len);
            css_len += css_resp->body_len;
            css_buf[css_len] = '\0';
        }
        if (css_resp) http_response_free(css_resp);

        p = tag_end + 1;
    }

    *out_len = css_len;
    if (css_len == 0) { free(css_buf); return NULL; }
    return css_buf;
}
