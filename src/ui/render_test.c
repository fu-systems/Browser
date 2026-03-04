/*
 * Pane — Offscreen Render Test
 *
 * Renders the home page to a PNG file without needing a display.
 * Validates that the full pipeline (parse → style → layout → paint) works.
 */

#include "../pane.h"
#include "../paint/cairo_backend.h"
#include "../font/font.h"
#include <cairo/cairo.h>
#include <stdio.h>
#include <string.h>

static const char *TEST_HTML =
    "<!DOCTYPE html>\n"
    "<html><head><title>Pane Render Test</title>\n"
    "<style>\n"
    "  body { font-family: sans-serif; margin: 20px; color: #222;\n"
    "         background-color: #fafafa; }\n"
    "  h1 { font-size: 28px; color: #1a1a2e; margin-bottom: 12px; }\n"
    "  .box { background-color: #e8f4f8; border: 2px solid #3498db;\n"
    "         padding: 16px; margin-bottom: 12px; }\n"
    "  .box h2 { font-size: 18px; color: #2c3e50; margin-bottom: 6px; }\n"
    "  .box p { font-size: 14px; color: #555; }\n"
    "  .highlight { background-color: #ffffcc; border-left: 4px solid #f39c12;\n"
    "               padding: 12px; }\n"
    "  .footer { margin-top: 20px; font-size: 11px; color: #999; }\n"
    "</style>\n"
    "</head><body>\n"
    "  <h1>Pane Browser Render Test</h1>\n"
    "  <div class=\"box\">\n"
    "    <h2>HTML Parsing</h2>\n"
    "    <p>The HTML5 tokenizer processes input through 36 states,\n"
    "       feeding tokens to a tree builder with 19 insertion modes.</p>\n"
    "  </div>\n"
    "  <div class=\"box\">\n"
    "    <h2>CSS Cascade</h2>\n"
    "    <p>Selectors are matched against DOM elements. Declarations are\n"
    "       sorted by specificity and source order to determine winners.</p>\n"
    "  </div>\n"
    "  <div class=\"highlight\">\n"
    "    <p>The pairwise context engine propagates 21 fields down the\n"
    "       DOM tree: formatting context, containing block, stacking\n"
    "       context, writing mode, font size, and more.</p>\n"
    "  </div>\n"
    "  <div class=\"footer\">Rendered by Pane v0.1.0 — Cairo + FreeType</div>\n"
    "</body></html>\n";

int main(int argc, char **argv)
{
    const char *output = "render_test.png";
    if (argc > 1) output = argv[1];

    int width = 800, height = 600;

    printf("Pane Offscreen Render Test\n");
    printf("Output: %s (%dx%d)\n", output, width, height);

    /* Initialize font system. */
    if (!font_system_init()) {
        fprintf(stderr, "Error: font system init failed\n");
        return 1;
    }

    /* Render the HTML. */
    PaneResult result = pane_render(TEST_HTML, strlen(TEST_HTML),
                                     NULL, 0, width, height);

    printf("DOM: %s\n",
           result.document->html ? "OK" : "no <html>");
    printf("Layout boxes: %s\n",
           result.layout_tree->root ? "OK" : "none");
    printf("Display list: %d commands\n",
           result.display_list->count);

    /* Create Cairo surface and render. */
    cairo_surface_t *surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, width, height);
    cairo_t *cr = cairo_create(surface);

    CairoRenderer renderer;
    cairo_renderer_init(&renderer, cr, 16.0f);

    /* Clear background. */
    cairo_render_clear(&renderer, 1.0, 1.0, 1.0);

    /* Render the layout tree. */
    cairo_render_layout(&renderer, result.layout_tree->root);

    cairo_renderer_destroy(&renderer);

    /* Save to PNG. */
    cairo_status_t status = cairo_surface_write_to_png(surface, output);
    if (status == CAIRO_STATUS_SUCCESS) {
        printf("PNG saved: %s\n", output);
    } else {
        fprintf(stderr, "Error saving PNG: %s\n",
                cairo_status_to_string(status));
    }

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    pane_result_free(&result);
    font_system_shutdown();

    printf("Done.\n");
    return 0;
}
