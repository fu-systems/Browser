/*
 * Pane — Shared Browser Utilities (Cross-Platform)
 *
 * Platform-independent functions used by both the GTK3 (Linux) and
 * Win32 (Windows) browser UIs.
 */

#ifndef PANE_BROWSER_COMMON_H
#define PANE_BROWSER_COMMON_H

#include "../dom/dom.h"
#include "../layout/box.h"
#include <stddef.h>
#include <stdbool.h>

/* ── URL resolution ────────────────────────────────────────────────── */

/* Resolve a possibly-relative href against a base URL.
 * Handles absolute, protocol-relative, root-relative, and path-relative. */
void resolve_url(const char *base, const char *href, char *out, size_t out_sz);

/* ── UTF-8 sanitization ───────────────────────────────────────────── */

/* Sanitize a buffer in-place, replacing invalid UTF-8 bytes with '?'.
 * Returns the same pointer for convenience. */
char *sanitize_utf8(char *buf, size_t len);

/* ── Layout hit-testing ────────────────────────────────────────────── */

/* Find the deepest layout box containing the given point (px, py).
 * ox/oy are the accumulated parent offsets. */
const LayoutBox *hit_test_box(const LayoutBox *box,
                               float px, float py,
                               float ox, float oy);

/* ── DOM ancestor helpers ──────────────────────────────────────────── */

/* Walk up the DOM tree from a layout box to find an <a> ancestor.
 * Returns the DomNode for the link, or NULL. */
const DomNode *find_link_ancestor(const LayoutBox *box);

/* Walk up from a layout box to find a form element (input/textarea/button/select).
 * Returns the DomNode, or NULL. */
const DomNode *find_form_element(const LayoutBox *box);

/* Find the parent <form> element of a DOM node. Returns NULL if none. */
const DomNode *find_form_parent(const DomNode *node);

/* Check if an input element is text-editable (text, search, email, etc.). */
bool is_text_input(const DomNode *node);

/* Check if a node is a clickable button (button, input type=submit, etc.). */
bool is_button_element(const DomNode *node);

/* ── Layout box helpers ────────────────────────────────────────────── */

/* Compute absolute page-space position of a layout box. */
void layout_box_abs_position(const LayoutBox *box, float *out_x, float *out_y);

/* Find the layout box associated with a specific DOM node (recursive search). */
const LayoutBox *find_box_for_node(const LayoutBox *root, const DomNode *node);

/* ── External CSS fetching ─────────────────────────────────────────── */

/* Scan HTML for <link rel="stylesheet" href="..."> tags, fetch up to 5
 * external CSS files, and concatenate them. Returns a heap-allocated
 * CSS string (caller must free) or NULL. */
char *fetch_external_css(const char *html, size_t html_len,
                         const char *base_url, size_t *out_len);

#endif /* PANE_BROWSER_COMMON_H */
