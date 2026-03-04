/*
 * Pane — Rendering Engine Entry Point
 *
 * Demonstrates the full pipeline:
 *   HTML parse → DOM → CSS cascade → computed style →
 *   pairwise context transitions → layout → paint
 */

#include "pane.h"
#include "html/tree_builder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ── pane_render implementation ────────────────────────────────────── */

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
            if (n->type == NODE_ELEMENT && n->elem.tag == TAG_STYLE) {
                DomNode *text = n->first_child;
                if (text && text->type == NODE_TEXT && text->text.data) {
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

/* ── Display list dumper ───────────────────────────────────────────── */

static void dump_display_list(const DisplayList *dl)
{
    printf("Display List: %d commands\n", dl->count);
    printf("─────────────────────────────────────\n");

    for (int i = 0; i < dl->count; i++) {
        const PaintCmd *cmd = &dl->cmds[i];
        printf("[%3d] ", i);

        switch (cmd->type) {
        case PAINT_RECT:
            printf("RECT  (%.0f,%.0f %.0fx%.0f) color=#%02x%02x%02x%02x\n",
                   cmd->rect.x, cmd->rect.y,
                   cmd->rect.width, cmd->rect.height,
                   cmd->fill_rect.color.r, cmd->fill_rect.color.g,
                   cmd->fill_rect.color.b, cmd->fill_rect.color.a);
            break;
        case PAINT_BORDER:
            printf("BORDER (%.0f,%.0f %.0fx%.0f) widths=(%.0f,%.0f,%.0f,%.0f)\n",
                   cmd->rect.x, cmd->rect.y,
                   cmd->rect.width, cmd->rect.height,
                   cmd->border.widths.top, cmd->border.widths.right,
                   cmd->border.widths.bottom, cmd->border.widths.left);
            break;
        case PAINT_TEXT:
            printf("TEXT  (%.0f,%.0f) size=%.0f \"",
                   cmd->rect.x, cmd->rect.y, cmd->text.font_size);
            /* Print first 60 chars. */
            for (size_t j = 0; j < cmd->text.len && j < 60; j++)
                putchar(cmd->text.text[j] >= 32 ? cmd->text.text[j] : '?');
            if (cmd->text.len > 60) printf("...");
            printf("\"\n");
            break;
        }
    }
}

/* ── Layout tree dumper ────────────────────────────────────────────── */

static const char *box_type_name(LayoutBoxType type)
{
    switch (type) {
    case BOX_BLOCK:           return "BLOCK";
    case BOX_INLINE:          return "INLINE";
    case BOX_INLINE_BLOCK:    return "INLINE-BLOCK";
    case BOX_FLEX:            return "FLEX";
    case BOX_GRID:            return "GRID";
    case BOX_TABLE:           return "TABLE";
    case BOX_TABLE_ROW:       return "TABLE-ROW";
    case BOX_TABLE_CELL:      return "TABLE-CELL";
    case BOX_TEXT:            return "TEXT";
    case BOX_ANONYMOUS_BLOCK: return "ANON-BLOCK";
    }
    return "?";
}

static void dump_layout_tree(const LayoutBox *box, int depth)
{
    if (!box) return;

    for (int i = 0; i < depth; i++) printf("  ");
    printf("%-12s (%.0f,%.0f) %5.0f×%-5.0f",
           box_type_name(box->type),
           box->rect.x, box->rect.y,
           box->rect.width, box->rect.height);

    if (box->node && box->node->type == NODE_ELEMENT) {
        printf("  <%s", html_tag_to_name(box->node->elem.tag));
        if (box->node->elem.id)
            printf(" id=\"%s\"", box->node->elem.id);
        if (box->node->elem.class_str)
            printf(" class=\"%s\"", box->node->elem.class_str);
        printf(">");
    } else if (box->type == BOX_TEXT && box->text) {
        printf("  \"");
        for (size_t j = 0; j < box->text_len && j < 40; j++)
            putchar(box->text[j] >= 32 ? box->text[j] : '?');
        if (box->text_len > 40) printf("...");
        printf("\"");
    }
    printf("\n");

    for (LayoutBox *child = box->first_child; child; child = child->next_sibling)
        dump_layout_tree(child, depth + 1);
}

/* ── Main ──────────────────────────────────────────────────────────── */

static const char *demo_html =
    "<!DOCTYPE html>\n"
    "<html>\n"
    "<head>\n"
    "  <title>Pane Rendering Engine Demo</title>\n"
    "  <style>\n"
    "    body { margin: 8px; font-size: 16px; color: #333; }\n"
    "    h1 { font-size: 32px; color: #1a1a2e; margin-bottom: 16px; }\n"
    "    .container { width: 600px; margin: 0 auto; padding: 20px;\n"
    "                 background-color: #f8f8f8; border: 1px solid #ddd; }\n"
    "    p { margin: 10px 0; line-height: 1.5; }\n"
    "    .highlight { background-color: #ffffcc; padding: 10px;\n"
    "                 border-left: 4px solid #e6b800; }\n"
    "    .flex-row { display: flex; gap: 10px; }\n"
    "    .flex-item { flex: 1; padding: 15px; background-color: #e8f4f8;\n"
    "                 border: 1px solid #b8d4e3; }\n"
    "  </style>\n"
    "</head>\n"
    "<body>\n"
    "  <div class=\"container\">\n"
    "    <h1>Pane Engine</h1>\n"
    "    <p>This is a demonstration of the Pane rendering engine, built in C17.</p>\n"
    "    <p>The engine uses a <strong>pairwise context transition model</strong> where\n"
    "       each parent-child relationship produces a new rendering context.</p>\n"
    "    <div class=\"highlight\">\n"
    "      <p>Context fields include: formatting context, containing block,\n"
    "         stacking context, writing mode, and 17 other inherited properties.</p>\n"
    "    </div>\n"
    "    <div class=\"flex-row\">\n"
    "      <div class=\"flex-item\">HTML Parser</div>\n"
    "      <div class=\"flex-item\">CSS Cascade</div>\n"
    "      <div class=\"flex-item\">Layout Engine</div>\n"
    "    </div>\n"
    "  </div>\n"
    "</body>\n"
    "</html>\n";

int main(int argc, char **argv)
{
    const char *html;
    size_t html_len;
    char *file_buf = NULL;

    if (argc > 1) {
        /* Read HTML from file. */
        FILE *f = fopen(argv[1], "rb");
        if (!f) {
            fprintf(stderr, "Error: cannot open %s\n", argv[1]);
            return 1;
        }
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        file_buf = malloc(sz + 1);
        fread(file_buf, 1, sz, f);
        file_buf[sz] = '\0';
        fclose(f);
        html = file_buf;
        html_len = sz;
    } else {
        html = demo_html;
        html_len = strlen(demo_html);
    }

    float vw = 800, vh = 600;
    printf("Pane Rendering Engine v0.1.0\n");
    printf("Viewport: %.0f × %.0f\n", vw, vh);
    printf("Input: %zu bytes of HTML\n", html_len);
    printf("═══════════════════════════════════════\n\n");

    PaneResult result = pane_render(html, html_len, NULL, 0, vw, vh);

    printf("DOM: %s\n",
           result.document->html ? "parsed successfully" : "no <html> element");
    if (result.document->head) printf("  <head> found\n");
    if (result.document->body) printf("  <body> found\n");
    printf("\n");

    printf("Layout Tree:\n");
    printf("─────────────────────────────────────\n");
    dump_layout_tree(result.layout_tree->root, 0);
    printf("\n");

    dump_display_list(result.display_list);

    pane_result_free(&result);
    free(file_buf);

    printf("\nDone.\n");
    return 0;
}
