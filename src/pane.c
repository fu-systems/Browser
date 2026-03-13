/*
 * Pane — Public API Implementation
 *
 * Full rendering pipeline:
 *   HTML parse → DOM → CSS cascade → computed style →
 *   pairwise context transitions → layout → paint
 */

#include "pane.h"
#include "html/tree_builder.h"
#include <string.h>
#include <stdlib.h>

/* ── Built-in User-Agent Stylesheet ───────────────────────────────── */

static const char UA_CSS[] =
    /* Box model */
    "html { display: block; }"
    "body { display: block; margin: 8px; }"
    "head, title, meta, link, style, script, noscript, template { display: none; }"

    /* Block elements */
    "article, aside, details, figcaption, figure, footer, header, hgroup, "
    "main, nav, section, summary { display: block; }"
    "address, blockquote, center, div, fieldset, form, hr, legend, listing, "
    "menu, pre, search, dialog, dir { display: block; }"

    /* Headings */
    "h1 { display: block; font-size: 2em; font-weight: bold; margin: 0.67em 0; }"
    "h2 { display: block; font-size: 1.5em; font-weight: bold; margin: 0.83em 0; }"
    "h3 { display: block; font-size: 1.17em; font-weight: bold; margin: 1em 0; }"
    "h4 { display: block; font-weight: bold; margin: 1.33em 0; }"
    "h5 { display: block; font-size: 0.83em; font-weight: bold; margin: 1.67em 0; }"
    "h6 { display: block; font-size: 0.67em; font-weight: bold; margin: 2.33em 0; }"

    /* Paragraphs & lists */
    "p { display: block; margin: 1em 0; }"
    "ul, ol { display: block; margin: 1em 0; padding-left: 40px; }"
    "li { display: list-item; }"
    "dl { display: block; margin: 1em 0; }"
    "dt { display: block; font-weight: bold; }"
    "dd { display: block; margin-left: 40px; }"
    "blockquote { display: block; margin: 1em 40px; }"

    /* Inline formatting */
    "b, strong { font-weight: bold; }"
    "i, em, cite, var, dfn { font-style: italic; }"
    "u, ins { text-decoration: underline; }"
    "s, strike, del { text-decoration: line-through; }"
    "small { font-size: 0.83em; }"
    "big { font-size: 1.17em; }"
    "sub { font-size: 0.83em; }"
    "sup { font-size: 0.83em; }"
    "code, kbd, samp, tt { font-family: monospace; }"
    "pre { display: block; font-family: monospace; white-space: pre; margin: 1em 0; }"

    /* Links */
    "a { color: #0000ee; text-decoration: underline; }"

    /* Horizontal rule */
    "hr { display: block; margin: 0.5em 0; border-top: 1px solid #888; }"

    /* Table defaults */
    "table { display: table; border-collapse: separate; border-spacing: 2px; }"
    "thead { display: table-header-group; }"
    "tbody { display: table-row-group; }"
    "tfoot { display: table-footer-group; }"
    "tr { display: table-row; }"
    "td, th { display: table-cell; padding: 1px; }"
    "th { font-weight: bold; text-align: center; }"
    "caption { display: table-caption; text-align: center; }"
    "colgroup { display: table-column-group; }"
    "col { display: table-column; }"

    /* Forms */
    "fieldset { display: block; border: 2px solid #888; padding: 8px; margin: 0 2px; }"
    "legend { display: block; padding: 0 4px; }"

    /* Figure */
    "figure { display: block; margin: 1em 40px; }"
    "figcaption { display: block; }"
;

static Stylesheet *ua_stylesheet = NULL;

static Stylesheet *get_ua_stylesheet(void)
{
    if (!ua_stylesheet) {
        ua_stylesheet = css_parse_stylesheet(UA_CSS, sizeof(UA_CSS) - 1);
    }
    return ua_stylesheet;
}

PaneResult pane_render(const char *html, size_t html_len,
                       const char *css, size_t css_len,
                       float viewport_width, float viewport_height)
{
    PaneResult result = {0};

    /* 1. Parse HTML → DOM. */
    result.document = html_parse(html, html_len);

    /* 2. Parse CSS → Stylesheet. */
    result.stylesheet = NULL;
    if (css && css_len > 0) {
        result.stylesheet = css_parse_stylesheet(css, css_len);
    }

    /* Extract <style> elements from <head> and <body> — concatenate all into one. */
    Stylesheet *inline_ss = NULL;
    {
        size_t style_cap = 0, style_len = 0;
        char *style_buf = NULL;

        /* Helper: recursively collect <style> content from a subtree. */
        /* Use an iterative approach with a stack to avoid deep recursion. */
        DomNode *roots[2] = { result.document->head, result.document->body };
        for (int ri = 0; ri < 2; ri++) {
            if (!roots[ri]) continue;
            for (DomNode *n = roots[ri]->first_child; n; n = n->next_sibling) {
                if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_STYLE) {
                    DomNode *text = n->first_child;
                    if (text && text->type == PANE_NODE_TEXT && text->text.data) {
                        size_t need = style_len + text->text.len + 2;
                        if (need > style_cap) {
                            style_cap = need > 4096 ? need : 4096;
                            char *nb = realloc(style_buf, style_cap);
                            if (!nb) break;
                            style_buf = nb;
                        }
                        if (style_len > 0) style_buf[style_len++] = '\n';
                        memcpy(style_buf + style_len, text->text.data, text->text.len);
                        style_len += text->text.len;
                    }
                }
            }
        }
        if (style_buf && style_len > 0) {
            style_buf[style_len] = '\0';
            inline_ss = css_parse_stylesheet(style_buf, style_len);
            free(style_buf);
        }
    }

    /* 3. Build layout tree with style + context transitions + layout. */
    int ss_count = 0;
    Stylesheet *ss_array[4];
    /* UA stylesheet first (lowest priority). */
    Stylesheet *ua = get_ua_stylesheet();
    if (ua) ss_array[ss_count++] = ua;
    if (result.stylesheet) ss_array[ss_count++] = result.stylesheet;
    if (inline_ss)         ss_array[ss_count++] = inline_ss;

    Stylesheet **ss_ptr = ss_count > 0 ? ss_array : NULL;
    result.layout_tree = layout_build(result.document, ss_ptr, ss_count,
                                       viewport_width, viewport_height);

    /* 4. Generate display list. */
    result.display_list = paint_generate(result.layout_tree->root);
    paint_sort(result.display_list);

    /* Clean up inline stylesheet. */
    if (inline_ss) css_stylesheet_free(inline_ss);

    return result;
}

void pane_result_free(PaneResult *result)
{
    if (result->display_list) paint_free(result->display_list);
    if (result->layout_tree)  layout_tree_free(result->layout_tree);
    if (result->stylesheet)   css_stylesheet_free(result->stylesheet);
    if (result->document)     doc_destroy(result->document);
    memset(result, 0, sizeof(*result));
}
