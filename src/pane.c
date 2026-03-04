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

    /* Also extract <style> elements from DOM. */
    Stylesheet *inline_ss = NULL;
    if (result.document->head) {
        for (DomNode *n = result.document->head->first_child; n; n = n->next_sibling) {
            if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_STYLE) {
                DomNode *text = n->first_child;
                if (text && text->type == PANE_NODE_TEXT && text->text.data) {
                    inline_ss = css_parse_stylesheet(text->text.data, text->text.len);
                }
            }
        }
    }

    /* 3. Build layout tree with style + context transitions + layout. */
    int ss_count = 0;
    Stylesheet *ss_array[2];
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
