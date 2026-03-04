/*
 * Pane — Browser UI Implementation (GTK3 + Cairo)
 *
 * Complete browser chrome: tabs, toolbar, content rendering, scrolling.
 */

#include "browser.h"
#include "../html/tree_builder.h"
#include "../net/http.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

/* ── Forward declarations ──────────────────────────────────────────── */

static void redraw_content(BrowserWindow *bw);
static void update_title(BrowserWindow *bw);
static void update_nav_buttons(BrowserWindow *bw);
static void update_scroll(BrowserWindow *bw);
static BrowserTab *active(BrowserWindow *bw);
static void update_plugin_button_style(GtkWidget *btn, bool enabled);

/* ── Default pages ─────────────────────────────────────────────────── */

static const char *HOME_PAGE =
    "<!DOCTYPE html>\n"
    "<html><head><title>Pane Browser</title>\n"
    "<style>\n"
    "  body { font-family: sans-serif; margin: 40px; color: #333;\n"
    "         background-color: #f5f5f5; }\n"
    "  h1 { font-size: 36px; color: #1a1a2e; margin-bottom: 8px; }\n"
    "  .subtitle { font-size: 14px; color: #888; margin-bottom: 32px; }\n"
    "  .card { background-color: #fff; border: 1px solid #ddd;\n"
    "          padding: 20px; margin-bottom: 16px; }\n"
    "  .card h2 { font-size: 18px; color: #16213e; margin-bottom: 8px; }\n"
    "  .card p { font-size: 14px; color: #555; line-height: 1.6; }\n"
    "  .stats { display: flex; margin-top: 24px; }\n"
    "  .stat { padding: 16px 24px; background-color: #e8f4f8;\n"
    "          border: 1px solid #b8d4e3; margin-right: 12px; }\n"
    "  .stat-num { font-size: 28px; font-weight: bold; color: #1a1a2e; }\n"
    "  .stat-label { font-size: 12px; color: #666; }\n"
    "  .footer { margin-top: 32px; font-size: 12px; color: #aaa; }\n"
    "</style>\n"
    "</head><body>\n"
    "  <h1>Pane</h1>\n"
    "  <div class=\"subtitle\">A web browser built from scratch in C17</div>\n"
    "  <div class=\"card\">\n"
    "    <h2>Welcome</h2>\n"
    "    <p>This is the Pane web browser. Type HTML in the address bar or\n"
    "       load a local file to render it. The engine uses a pairwise\n"
    "       context transition model for style and layout resolution.</p>\n"
    "  </div>\n"
    "  <div class=\"card\">\n"
    "    <h2>Architecture</h2>\n"
    "    <p>HTML Parser (tokenizer + tree builder with 62 rules) feeds into\n"
    "       a DOM tree. CSS cascade resolves styles. The pairwise context\n"
    "       engine propagates 21 context fields down the tree. Block and\n"
    "       inline layout produce positioned boxes. Cairo + FreeType render\n"
    "       the final pixels.</p>\n"
    "  </div>\n"
    "  <div class=\"stats\">\n"
    "    <div class=\"stat\">\n"
    "      <div class=\"stat-num\">412</div>\n"
    "      <div class=\"stat-label\">CSS Properties</div>\n"
    "    </div>\n"
    "    <div class=\"stat\">\n"
    "      <div class=\"stat-num\">130</div>\n"
    "      <div class=\"stat-label\">HTML Tags</div>\n"
    "    </div>\n"
    "    <div class=\"stat\">\n"
    "      <div class=\"stat-num\">21</div>\n"
    "      <div class=\"stat-label\">Context Fields</div>\n"
    "    </div>\n"
    "    <div class=\"stat\">\n"
    "      <div class=\"stat-num\">93</div>\n"
    "      <div class=\"stat-label\">Compressed Rules</div>\n"
    "    </div>\n"
    "  </div>\n"
    "  <div class=\"footer\">Pane v0.1.0 — Phase 2 C Rendering Engine</div>\n"
    "</body></html>\n";

static const char *ERROR_PAGE_FMT =
    "<!DOCTYPE html>\n"
    "<html><head><title>Error</title>\n"
    "<style>\n"
    "  body { font-family: sans-serif; margin: 60px; color: #333;\n"
    "         background-color: #fff5f5; }\n"
    "  h1 { font-size: 24px; color: #c0392b; }\n"
    "  p { font-size: 14px; color: #666; }\n"
    "  .url { font-family: monospace; background: #f0f0f0; padding: 4px 8px; }\n"
    "</style>\n"
    "</head><body>\n"
    "  <h1>Page Load Error</h1>\n"
    "  <p>Could not load: <span class=\"url\">%s</span></p>\n"
    "  <p>%s</p>\n"
    "</body></html>\n";

/* ── Utility ───────────────────────────────────────────────────────── */

static BrowserTab *active(BrowserWindow *bw)
{
    if (bw->active_tab >= 0 && bw->active_tab < bw->tab_count)
        return &bw->tabs[bw->active_tab];
    return NULL;
}

static void tab_free_content(BrowserTab *tab)
{
    if (tab->has_content) {
        pane_result_free(&tab->result);
        tab->has_content = false;
    }
}

static void tab_free_history(BrowserTab *tab)
{
    for (int i = 0; i < tab->history_count; i++)
        free(tab->history[i]);
    tab->history_count = 0;
    tab->history_pos = -1;
}

static void tab_push_history(BrowserTab *tab, const char *url)
{
    /* Truncate forward history. */
    for (int i = tab->history_pos + 1; i < tab->history_count; i++) {
        free(tab->history[i]);
        tab->history[i] = NULL;
    }
    tab->history_count = tab->history_pos + 1;

    if (tab->history_count >= MAX_HISTORY) {
        /* Shift out oldest. */
        free(tab->history[0]);
        memmove(tab->history, tab->history + 1,
                (MAX_HISTORY - 1) * sizeof(char *));
        tab->history_count = MAX_HISTORY - 1;
    }

    tab->history[tab->history_count] = strdup(url);
    tab->history_pos = tab->history_count;
    tab->history_count++;
}

/* ── Rendering ─────────────────────────────────────────────────────── */

static void render_page(BrowserWindow *bw, const char *html, size_t len,
                        const char *title)
{
    BrowserTab *tab = active(bw);
    if (!tab) return;

    tab_free_content(tab);

    tab->result = pane_render(html, len, NULL, 0,
                              bw->viewport_width, bw->viewport_height);
    tab->has_content = true;

    /* Run plugin DOM-ready hook (lets plugins mutate the DOM). */
    if (bw->plugin_pipeline && tab->result.document)
        pipeline_run_dom_ready(bw->plugin_pipeline, tab->result.document);
    tab->scroll_x = 0;
    tab->scroll_y = 0;

    /* Estimate content height from layout tree. */
    if (tab->result.layout_tree && tab->result.layout_tree->root) {
        LayoutBox *root = tab->result.layout_tree->root;
        tab->content_height = root->rect.y + root->rect.height +
                              root->padding.bottom + root->border.bottom +
                              root->margin.bottom;
        if (tab->content_height < bw->viewport_height)
            tab->content_height = bw->viewport_height;
    }

    if (title) {
        strncpy(tab->title, title, sizeof(tab->title) - 1);
    } else {
        /* Try to extract <title> from DOM. */
        if (tab->result.document && tab->result.document->head) {
            DomNode *head = tab->result.document->head;
            for (DomNode *n = head->first_child; n; n = n->next_sibling) {
                if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_TITLE) {
                    DomNode *tc = n->first_child;
                    if (tc && tc->type == PANE_NODE_TEXT && tc->text.data) {
                        size_t tlen = tc->text.len < 255 ? tc->text.len : 255;
                        memcpy(tab->title, tc->text.data, tlen);
                        tab->title[tlen] = '\0';
                    }
                    break;
                }
            }
        }
        if (!tab->title[0])
            strcpy(tab->title, "Untitled");
    }

    update_title(bw);
    update_scroll(bw);
    redraw_content(bw);
}

/* ── Page loading ──────────────────────────────────────────────────── */

void browser_load_html(BrowserWindow *bw, const char *html, const char *title)
{
    BrowserTab *tab = active(bw);
    if (!tab) return;

    strncpy(tab->url, "about:html", sizeof(tab->url));
    render_page(bw, html, strlen(html), title);
}

void browser_navigate(BrowserWindow *bw, const char *url)
{
    BrowserTab *tab = active(bw);
    if (!tab) return;

    /* Run plugin navigation hook (may modify or cancel URL). */
    char nav_url[4096];
    strncpy(nav_url, url, sizeof(nav_url) - 1);
    nav_url[sizeof(nav_url) - 1] = '\0';
    if (bw->plugin_pipeline) {
        if (!pipeline_run_navigate(bw->plugin_pipeline,
                                   nav_url, sizeof(nav_url))) {
            /* Plugin cancelled navigation. */
            gtk_label_set_text(GTK_LABEL(bw->status_bar),
                               "Navigation blocked by plugin");
            return;
        }
        url = nav_url;
    }

    /* Sync cookie manager with per-tab cookie toggle. */
    if (bw->plugin_registry) {
        PanePlugin *cm = plugin_registry_find(bw->plugin_registry,
                                              "cookie_manager");
        if (cm) {
            CookieManagerData *cmd = cookie_manager_get_data(cm);
            if (cmd) cmd->cookies_enabled = tab->cookies_enabled;
        }
    }

    strncpy(tab->url, url, sizeof(tab->url) - 1);

    /* Home page. */
    if (strcmp(url, "about:home") == 0 || strcmp(url, "") == 0) {
        tab_push_history(tab, url);
        render_page(bw, HOME_PAGE, strlen(HOME_PAGE), NULL);
        gtk_entry_set_text(GTK_ENTRY(bw->url_entry), url);
        update_nav_buttons(bw);
        return;
    }

    /* Inline HTML (starts with < or <!). */
    if (url[0] == '<') {
        tab_push_history(tab, "about:html");
        render_page(bw, url, strlen(url), NULL);
        gtk_entry_set_text(GTK_ENTRY(bw->url_entry), "about:html");
        update_nav_buttons(bw);
        return;
    }

    /* Local file. */
    if (strncmp(url, "file://", 7) == 0 || url[0] == '/') {
        const char *path = (strncmp(url, "file://", 7) == 0) ? url + 7 : url;

        FILE *f = fopen(path, "rb");
        if (!f) {
            char err[4096];
            snprintf(err, sizeof(err), ERROR_PAGE_FMT, url, "File not found.");
            render_page(bw, err, strlen(err), "Error");
            update_nav_buttons(bw);
            return;
        }

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        char *buf = malloc(sz + 1);
        fread(buf, 1, sz, f);
        buf[sz] = '\0';
        fclose(f);

        tab_push_history(tab, url);
        render_page(bw, buf, sz, NULL);
        free(buf);

        gtk_entry_set_text(GTK_ENTRY(bw->url_entry), url);
        update_nav_buttons(bw);
        return;
    }

    /* HTTP / HTTPS URLs. */
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        /* Update status bar. */
        gtk_label_set_text(GTK_LABEL(bw->status_bar), "Loading...");

        /* Process pending GTK events to show "Loading..." immediately. */
        while (gtk_events_pending()) gtk_main_iteration();

        /* Run plugin before-request hook. */
        FetchRequest *freq = fetch_request_create(url);
        if (bw->plugin_pipeline && freq) {
            if (!pipeline_run_before_request(bw->plugin_pipeline, freq)) {
                /* Plugin cancelled the request. */
                fetch_request_free(freq);
                gtk_label_set_text(GTK_LABEL(bw->status_bar),
                                   "Request blocked by plugin");
                update_nav_buttons(bw);
                return;
            }
            /* Use possibly-modified URL from plugin. */
            url = freq->url;
        }

        HttpResponse *resp = http_get(url, 5);

        /* Run plugin after-response hook. */
        if (bw->plugin_pipeline && resp && !resp->error &&
            resp->body && resp->body_len > 0) {
            FetchResponse *fresp = fetch_response_create(
                resp->status_code, resp->body, resp->body_len, url);
            if (fresp) {
                pipeline_run_after_response(bw->plugin_pipeline, fresp);
                /* Copy back modified body if changed. */
                if (fresp->body && fresp->body_len > 0 &&
                    fresp->body_len != resp->body_len) {
                    free(resp->body);
                    resp->body = malloc(fresp->body_len + 1);
                    memcpy(resp->body, fresp->body, fresp->body_len);
                    resp->body[fresp->body_len] = '\0';
                    resp->body_len = fresp->body_len;
                }
                fetch_response_free(fresp);
            }
        }

        fetch_request_free(freq);

        if (resp->error) {
            char err[4096];
            snprintf(err, sizeof(err), ERROR_PAGE_FMT, url, resp->error);
            tab_push_history(tab, url);
            render_page(bw, err, strlen(err), "Error");
            gtk_label_set_text(GTK_LABEL(bw->status_bar), "Error");
        } else if (resp->body && resp->body_len > 0) {
            tab_push_history(tab, url);
            render_page(bw, resp->body, resp->body_len, NULL);

            char status_msg[256];
            snprintf(status_msg, sizeof(status_msg),
                     "HTTP %d — %zu bytes", resp->status_code, resp->body_len);
            gtk_label_set_text(GTK_LABEL(bw->status_bar), status_msg);
        } else {
            char err[4096];
            snprintf(err, sizeof(err), ERROR_PAGE_FMT, url, "Empty response.");
            tab_push_history(tab, url);
            render_page(bw, err, strlen(err), "Error");
            gtk_label_set_text(GTK_LABEL(bw->status_bar), "Empty response");
        }

        http_response_free(resp);
        gtk_entry_set_text(GTK_ENTRY(bw->url_entry), url);
        update_nav_buttons(bw);
        return;
    }

    /* Bare domain name — try as https:// */
    if (strchr(url, '.') && url[0] != '<' && url[0] != '/') {
        char full_url[2048];
        snprintf(full_url, sizeof(full_url), "https://%s", url);
        browser_navigate(bw, full_url);
        return;
    }

    /* Unknown URL scheme — show error. */
    char err[4096];
    snprintf(err, sizeof(err), ERROR_PAGE_FMT, url,
             "Unsupported URL scheme.");
    tab_push_history(tab, url);
    render_page(bw, err, strlen(err), "Error");
    gtk_entry_set_text(GTK_ENTRY(bw->url_entry), url);
    update_nav_buttons(bw);
}

/* ── Tab management ────────────────────────────────────────────────── */

static void on_tab_clicked(GtkWidget *widget, gpointer data);
static void on_tab_close(GtkWidget *widget, gpointer data);

int browser_add_tab(BrowserWindow *bw)
{
    if (bw->tab_count >= MAX_TABS) return -1;

    int idx = bw->tab_count++;
    BrowserTab *tab = &bw->tabs[idx];
    memset(tab, 0, sizeof(BrowserTab));
    strcpy(tab->title, "New Tab");
    strcpy(tab->url, "about:home");
    tab->history_pos = -1;

    /* Create tab bar widgets. */
    tab->tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    tab->tab_label = gtk_label_new("New Tab");
    gtk_label_set_max_width_chars(GTK_LABEL(tab->tab_label), 20);
    gtk_label_set_ellipsize(GTK_LABEL(tab->tab_label), PANGO_ELLIPSIZE_END);

    tab->tab_button = gtk_button_new_with_label("\xC3\x97"); /* × */
    gtk_widget_set_size_request(tab->tab_button, 20, 20);

    gtk_box_pack_start(GTK_BOX(tab->tab_box), tab->tab_label, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(tab->tab_box), tab->tab_button, FALSE, FALSE, 0);

    /* Wrap in an event box for clicking. */
    GtkWidget *event_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event_box), tab->tab_box);

    g_signal_connect(event_box, "button-press-event",
                     G_CALLBACK(on_tab_clicked), bw);
    g_signal_connect(tab->tab_button, "clicked",
                     G_CALLBACK(on_tab_close), bw);

    /* Store the tab index as widget data. */
    g_object_set_data(G_OBJECT(event_box), "tab-index",
                      GINT_TO_POINTER(idx));
    g_object_set_data(G_OBJECT(tab->tab_button), "tab-index",
                      GINT_TO_POINTER(idx));

    gtk_box_pack_start(GTK_BOX(bw->tab_bar), event_box, FALSE, FALSE, 0);
    gtk_widget_show_all(event_box);

    return idx;
}

void browser_close_tab(BrowserWindow *bw, int tab_idx)
{
    if (tab_idx < 0 || tab_idx >= bw->tab_count) return;
    if (bw->tab_count <= 1) {
        /* Last tab — quit. */
        gtk_main_quit();
        return;
    }

    BrowserTab *tab = &bw->tabs[tab_idx];
    tab_free_content(tab);
    tab_free_history(tab);

    /* Remove tab widgets. */
    GtkWidget *parent = gtk_widget_get_parent(tab->tab_box);
    if (parent) gtk_widget_destroy(parent);

    /* Shift remaining tabs. */
    for (int i = tab_idx; i < bw->tab_count - 1; i++) {
        bw->tabs[i] = bw->tabs[i + 1];
    }
    bw->tab_count--;

    if (bw->active_tab >= bw->tab_count)
        bw->active_tab = bw->tab_count - 1;

    /* Update tab indices in widgets. */
    GList *children = gtk_container_get_children(GTK_CONTAINER(bw->tab_bar));
    int i = 0;
    for (GList *l = children; l; l = l->next, i++) {
        g_object_set_data(G_OBJECT(l->data), "tab-index", GINT_TO_POINTER(i));
        /* Update close button index. */
        BrowserTab *t = &bw->tabs[i];
        g_object_set_data(G_OBJECT(t->tab_button), "tab-index",
                          GINT_TO_POINTER(i));
    }
    g_list_free(children);

    browser_switch_tab(bw, bw->active_tab);
}

void browser_switch_tab(BrowserWindow *bw, int tab_idx)
{
    if (tab_idx < 0 || tab_idx >= bw->tab_count) return;
    bw->active_tab = tab_idx;

    BrowserTab *tab = &bw->tabs[tab_idx];
    gtk_entry_set_text(GTK_ENTRY(bw->url_entry), tab->url);

    /* Highlight active tab. */
    GList *children = gtk_container_get_children(GTK_CONTAINER(bw->tab_bar));
    int i = 0;
    for (GList *l = children; l; l = l->next, i++) {
        GtkStyleContext *ctx = gtk_widget_get_style_context(GTK_WIDGET(l->data));
        if (i == tab_idx)
            gtk_style_context_add_class(ctx, "active-tab");
        else
            gtk_style_context_remove_class(ctx, "active-tab");
    }
    g_list_free(children);

    update_title(bw);
    update_nav_buttons(bw);
    update_scroll(bw);
    redraw_content(bw);

    /* Sync cookie button state with tab. */
    if (bw->cookie_btn)
        update_plugin_button_style(bw->cookie_btn, tab->cookies_enabled);
}

/* ── Tab callbacks ─────────────────────────────────────────────────── */

static void on_tab_clicked(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "tab-index"));
    browser_switch_tab(bw, idx);
}

static void on_tab_close(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "tab-index"));
    browser_close_tab(bw, idx);
}

/* ── UI Updates ────────────────────────────────────────────────────── */

static void update_title(BrowserWindow *bw)
{
    BrowserTab *tab = active(bw);
    if (!tab) return;

    char title[512];
    snprintf(title, sizeof(title), "%s — Pane", tab->title);
    gtk_window_set_title(GTK_WINDOW(bw->window), title);

    if (tab->tab_label)
        gtk_label_set_text(GTK_LABEL(tab->tab_label), tab->title);
}

static void update_nav_buttons(BrowserWindow *bw)
{
    BrowserTab *tab = active(bw);
    if (!tab) return;

    gtk_widget_set_sensitive(bw->back_btn, tab->history_pos > 0);
    gtk_widget_set_sensitive(bw->forward_btn,
        tab->history_pos < tab->history_count - 1);
}

static void update_scroll(BrowserWindow *bw)
{
    BrowserTab *tab = active(bw);
    if (!tab || !bw->scroll_adj) return;

    float page = bw->viewport_height;
    float upper = tab->content_height > page ? tab->content_height : page;

    gtk_adjustment_configure(bw->scroll_adj,
        tab->scroll_y,   /* value */
        0,               /* lower */
        upper,           /* upper */
        20,              /* step */
        page * 0.8f,     /* page increment */
        page);           /* page size */
}

static void redraw_content(BrowserWindow *bw)
{
    if (bw->content_area)
        gtk_widget_queue_draw(bw->content_area);
}

/* ── Drawing callback ──────────────────────────────────────────────── */

static gboolean on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);

    /* Get widget dimensions. */
    int w = gtk_widget_get_allocated_width(widget);
    int h = gtk_widget_get_allocated_height(widget);

    bw->viewport_width = (float)w;
    bw->viewport_height = (float)h;

    /* White background. */
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if (!tab || !tab->has_content) {
        /* Draw placeholder. */
        cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
        cairo_select_font_face(cr, "sans-serif",
            CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 14);
        cairo_move_to(cr, 20, 30);
        cairo_show_text(cr, "No content loaded. Type a URL or press Home.");
        return TRUE;
    }

    /* Render the page. */
    if (tab->result.layout_tree && tab->result.layout_tree->root) {
        CairoRenderer renderer;
        cairo_renderer_init(&renderer, cr, 16.0f);
        renderer.scroll_x = tab->scroll_x;
        renderer.scroll_y = tab->scroll_y;

        cairo_render_layout(&renderer, tab->result.layout_tree->root);
        cairo_renderer_destroy(&renderer);
    }

    return TRUE;
}

/* ── Scrollbar callback ────────────────────────────────────────────── */

static void on_scroll_changed(GtkAdjustment *adj, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (!tab) return;

    tab->scroll_y = (float)gtk_adjustment_get_value(adj);
    redraw_content(bw);
}

/* ── Mouse scroll callback ─────────────────────────────────────────── */

static gboolean on_scroll_event(GtkWidget *widget, GdkEventScroll *event,
                                 gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (!tab) return FALSE;

    float delta = 60.0f;

    switch (event->direction) {
    case GDK_SCROLL_UP:
        tab->scroll_y -= delta;
        break;
    case GDK_SCROLL_DOWN:
        tab->scroll_y += delta;
        break;
    case GDK_SCROLL_SMOOTH:
        tab->scroll_y += (float)(event->delta_y * delta);
        break;
    default:
        return FALSE;
    }

    /* Clamp. */
    if (tab->scroll_y < 0) tab->scroll_y = 0;
    float max_scroll = tab->content_height - bw->viewport_height;
    if (max_scroll < 0) max_scroll = 0;
    if (tab->scroll_y > max_scroll) tab->scroll_y = max_scroll;

    /* Sync scrollbar. */
    gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
    redraw_content(bw);
    return TRUE;
}

/* ── Keyboard callback ─────────────────────────────────────────────── */

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event,
                              gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);

    guint key = event->keyval;
    guint mods = event->state & gtk_accelerator_get_default_mod_mask();

    /* Ctrl+T: New tab. */
    if (mods == GDK_CONTROL_MASK && key == GDK_KEY_t) {
        int idx = browser_add_tab(bw);
        if (idx >= 0) {
            browser_switch_tab(bw, idx);
            browser_navigate(bw, "about:home");
        }
        return TRUE;
    }

    /* Ctrl+W: Close tab. */
    if (mods == GDK_CONTROL_MASK && key == GDK_KEY_w) {
        browser_close_tab(bw, bw->active_tab);
        return TRUE;
    }

    /* Ctrl+L: Focus URL bar. */
    if (mods == GDK_CONTROL_MASK && key == GDK_KEY_l) {
        gtk_widget_grab_focus(bw->url_entry);
        gtk_editable_select_region(GTK_EDITABLE(bw->url_entry), 0, -1);
        return TRUE;
    }

    /* Ctrl+R / F5: Reload. */
    if ((mods == GDK_CONTROL_MASK && key == GDK_KEY_r) ||
        key == GDK_KEY_F5) {
        if (tab) browser_navigate(bw, tab->url);
        return TRUE;
    }

    /* Alt+Left: Back. */
    if (mods == GDK_MOD1_MASK && key == GDK_KEY_Left) {
        if (tab && tab->history_pos > 0) {
            tab->history_pos--;
            browser_navigate(bw, tab->history[tab->history_pos]);
        }
        return TRUE;
    }

    /* Alt+Right: Forward. */
    if (mods == GDK_MOD1_MASK && key == GDK_KEY_Right) {
        if (tab && tab->history_pos < tab->history_count - 1) {
            tab->history_pos++;
            browser_navigate(bw, tab->history[tab->history_pos]);
        }
        return TRUE;
    }

    /* Ctrl+Tab / Ctrl+Page_Down: Next tab. */
    if (mods == GDK_CONTROL_MASK &&
        (key == GDK_KEY_Tab || key == GDK_KEY_Page_Down)) {
        int next = (bw->active_tab + 1) % bw->tab_count;
        browser_switch_tab(bw, next);
        return TRUE;
    }

    /* Ctrl+Shift+Tab / Ctrl+Page_Up: Prev tab. */
    if ((mods == (GDK_CONTROL_MASK | GDK_SHIFT_MASK) && key == GDK_KEY_Tab) ||
        (mods == GDK_CONTROL_MASK && key == GDK_KEY_Page_Up)) {
        int prev = (bw->active_tab - 1 + bw->tab_count) % bw->tab_count;
        browser_switch_tab(bw, prev);
        return TRUE;
    }

    /* Page Up/Down for scrolling when content area is focused. */
    if (!gtk_widget_has_focus(bw->url_entry)) {
        float page = bw->viewport_height * 0.8f;
        if (key == GDK_KEY_Page_Up || key == GDK_KEY_KP_Page_Up) {
            if (tab) {
                tab->scroll_y -= page;
                if (tab->scroll_y < 0) tab->scroll_y = 0;
                gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
                redraw_content(bw);
            }
            return TRUE;
        }
        if (key == GDK_KEY_Page_Down || key == GDK_KEY_KP_Page_Down) {
            if (tab) {
                tab->scroll_y += page;
                float max_s = tab->content_height - bw->viewport_height;
                if (max_s < 0) max_s = 0;
                if (tab->scroll_y > max_s) tab->scroll_y = max_s;
                gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
                redraw_content(bw);
            }
            return TRUE;
        }

        /* Arrow keys. */
        if (key == GDK_KEY_Up) {
            if (tab) {
                tab->scroll_y -= 40;
                if (tab->scroll_y < 0) tab->scroll_y = 0;
                gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
                redraw_content(bw);
            }
            return TRUE;
        }
        if (key == GDK_KEY_Down) {
            if (tab) {
                tab->scroll_y += 40;
                float max_s = tab->content_height - bw->viewport_height;
                if (max_s < 0) max_s = 0;
                if (tab->scroll_y > max_s) tab->scroll_y = max_s;
                gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
                redraw_content(bw);
            }
            return TRUE;
        }

        /* Home/End. */
        if (key == GDK_KEY_Home) {
            if (tab) { tab->scroll_y = 0; gtk_adjustment_set_value(bw->scroll_adj, 0); redraw_content(bw); }
            return TRUE;
        }
        if (key == GDK_KEY_End) {
            if (tab) {
                float max_s = tab->content_height - bw->viewport_height;
                if (max_s < 0) max_s = 0;
                tab->scroll_y = max_s;
                gtk_adjustment_set_value(bw->scroll_adj, max_s);
                redraw_content(bw);
            }
            return TRUE;
        }

        /* Space: page down. */
        if (key == GDK_KEY_space) {
            if (tab) {
                tab->scroll_y += bw->viewport_height * 0.8f;
                float max_s = tab->content_height - bw->viewport_height;
                if (max_s < 0) max_s = 0;
                if (tab->scroll_y > max_s) tab->scroll_y = max_s;
                gtk_adjustment_set_value(bw->scroll_adj, tab->scroll_y);
                redraw_content(bw);
            }
            return TRUE;
        }
    }

    return FALSE;
}

/* ── Toolbar callbacks ─────────────────────────────────────────────── */

static void on_back(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (!tab || tab->history_pos <= 0) return;

    tab->history_pos--;
    const char *url = tab->history[tab->history_pos];
    strncpy(tab->url, url, sizeof(tab->url) - 1);

    if (strcmp(url, "about:home") == 0) {
        render_page(bw, HOME_PAGE, strlen(HOME_PAGE), NULL);
    } else {
        browser_navigate(bw, url);
    }
    gtk_entry_set_text(GTK_ENTRY(bw->url_entry), tab->url);
    update_nav_buttons(bw);
}

static void on_forward(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (!tab || tab->history_pos >= tab->history_count - 1) return;

    tab->history_pos++;
    const char *url = tab->history[tab->history_pos];
    strncpy(tab->url, url, sizeof(tab->url) - 1);
    browser_navigate(bw, url);
    gtk_entry_set_text(GTK_ENTRY(bw->url_entry), tab->url);
    update_nav_buttons(bw);
}

static void on_reload(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (tab) browser_navigate(bw, tab->url);
}

static void on_home(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    browser_navigate(bw, "about:home");
}

static void on_url_activate(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    const char *text = gtk_entry_get_text(GTK_ENTRY(bw->url_entry));
    browser_navigate(bw, text);
    /* Move focus to content area. */
    gtk_widget_grab_focus(bw->content_area);
}

static void on_new_tab_btn(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    int idx = browser_add_tab(bw);
    if (idx >= 0) {
        browser_switch_tab(bw, idx);
        browser_navigate(bw, "about:home");
    }
}

/* ── Plugin toggle callbacks ───────────────────────────────────────── */

static void update_plugin_button_style(GtkWidget *btn, bool enabled)
{
    GtkStyleContext *ctx = gtk_widget_get_style_context(btn);
    if (enabled)
        gtk_style_context_add_class(ctx, "plugin-active");
    else
        gtk_style_context_remove_class(ctx, "plugin-active");
}

static void on_privacy_toggle(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    if (!bw->plugin_registry) return;

    bool enabled = plugin_registry_is_enabled(bw->plugin_registry,
                                              "privacy_shield");
    if (enabled)
        plugin_registry_disable(bw->plugin_registry, "privacy_shield");
    else
        plugin_registry_enable(bw->plugin_registry, "privacy_shield");

    update_plugin_button_style(bw->privacy_btn, !enabled);

    char msg[128];
    snprintf(msg, sizeof(msg), "Privacy Shield: %s", !enabled ? "ON" : "OFF");
    gtk_label_set_text(GTK_LABEL(bw->status_bar), msg);
}

static void on_darkmode_toggle(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    if (!bw->plugin_registry) return;

    bool enabled = plugin_registry_is_enabled(bw->plugin_registry,
                                              "dark_mode");
    if (enabled)
        plugin_registry_disable(bw->plugin_registry, "dark_mode");
    else
        plugin_registry_enable(bw->plugin_registry, "dark_mode");

    update_plugin_button_style(bw->darkmode_btn, !enabled);

    char msg[128];
    snprintf(msg, sizeof(msg), "Dark Mode: %s", !enabled ? "ON" : "OFF");
    gtk_label_set_text(GTK_LABEL(bw->status_bar), msg);

    /* Reload current page to apply dark mode. */
    BrowserTab *tab = active(bw);
    if (tab && tab->has_content && tab->url[0])
        browser_navigate(bw, tab->url);
}

static void on_cookie_toggle(GtkWidget *widget, gpointer data)
{
    BrowserWindow *bw = data;
    BrowserTab *tab = active(bw);
    if (!tab) return;

    tab->cookies_enabled = !tab->cookies_enabled;
    update_plugin_button_style(bw->cookie_btn, tab->cookies_enabled);

    char msg[128];
    snprintf(msg, sizeof(msg), "Cookies: %s (this tab)",
             tab->cookies_enabled ? "ON" : "OFF");
    gtk_label_set_text(GTK_LABEL(bw->status_bar), msg);
}

/* ── Resize callback ───────────────────────────────────────────────── */

static void on_content_resize(GtkWidget *widget, GdkRectangle *alloc,
                               gpointer data)
{
    BrowserWindow *bw = data;
    bw->viewport_width = (float)alloc->width;
    bw->viewport_height = (float)alloc->height;

    /* Re-layout current page. */
    BrowserTab *tab = active(bw);
    if (tab && tab->has_content) {
        /* For now just redraw — full re-layout would re-run the pipeline. */
        update_scroll(bw);
    }
}

/* ── Window destroy ────────────────────────────────────────────────── */

static void on_window_destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

/* ── CSS for the browser chrome ────────────────────────────────────── */

static const char *BROWSER_CSS =
    "window { background-color: #2b2b2b; }\n"
    "#tab-bar { background-color: #1e1e1e; padding: 2px 4px 0; }\n"
    "#tab-bar > * { background-color: #3c3c3c; border-radius: 6px 6px 0 0;\n"
    "               padding: 4px 6px; margin-right: 2px;\n"
    "               border: 1px solid #555; border-bottom: none; }\n"
    "#tab-bar > *.active-tab { background-color: #505050; }\n"
    "#tab-bar label { color: #ddd; font-size: 12px; }\n"
    "#tab-bar button { background: none; border: none; color: #aaa;\n"
    "                  padding: 0 2px; min-width: 16px; min-height: 16px; }\n"
    "#tab-bar button:hover { color: #fff; background-color: #c0392b;\n"
    "                        border-radius: 4px; }\n"
    "#new-tab-btn { background: none; border: none; color: #888;\n"
    "               padding: 4px 10px; font-size: 16px; }\n"
    "#new-tab-btn:hover { color: #fff; }\n"
    "#toolbar { background-color: #3c3c3c; padding: 4px 6px; }\n"
    "#toolbar button { background-color: #505050; border: 1px solid #666;\n"
    "                  color: #ddd; padding: 4px 8px; border-radius: 4px;\n"
    "                  min-width: 30px; }\n"
    "#toolbar button:hover { background-color: #606060; }\n"
    "#toolbar button:disabled { color: #666; }\n"
    "#url-entry { background-color: #1e1e1e; color: #eee; border: 1px solid #555;\n"
    "             border-radius: 4px; padding: 4px 8px; font-size: 13px; }\n"
    "#url-entry:focus { border-color: #4a9eff; }\n"
    "#status-bar { background-color: #1e1e1e; color: #888;\n"
    "              padding: 2px 8px; font-size: 11px; }\n"
    "#content-area { background-color: #ffffff; }\n"
    ".plugin-btn { background-color: #505050; border: 1px solid #666;\n"
    "              color: #888; padding: 4px 8px; border-radius: 4px;\n"
    "              min-width: 30px; font-size: 11px; }\n"
    ".plugin-btn:hover { background-color: #606060; color: #ddd; }\n"
    ".plugin-btn.plugin-active { background-color: #2e7d32;\n"
    "                            border-color: #4caf50; color: #fff; }\n";

/* ── Window creation ───────────────────────────────────────────────── */

BrowserWindow *browser_window_new(void)
{
    BrowserWindow *bw = calloc(1, sizeof(BrowserWindow));
    bw->viewport_width = 1024;
    bw->viewport_height = 700;

    /* Initialize font system. */
    if (!font_system_init()) {
        fprintf(stderr, "pane: Warning: font system init failed\n");
    }
    bw->fonts_ready = true;

    /* ── Apply CSS ─────────────────────────────────────────────── */

    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, BROWSER_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* ── Window ────────────────────────────────────────────────── */

    bw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(bw->window), "Pane Browser");
    gtk_window_set_default_size(GTK_WINDOW(bw->window), 1100, 780);
    g_signal_connect(bw->window, "destroy", G_CALLBACK(on_window_destroy), bw);
    g_signal_connect(bw->window, "key-press-event", G_CALLBACK(on_key_press), bw);

    /* ── Main layout ───────────────────────────────────────────── */

    bw->main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(bw->window), bw->main_vbox);

    /* ── Tab bar ───────────────────────────────────────────────── */

    GtkWidget *tab_bar_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(tab_bar_hbox, "tab-bar");

    bw->tab_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(tab_bar_hbox), bw->tab_bar, TRUE, TRUE, 0);

    GtkWidget *new_tab_btn = gtk_button_new_with_label("+");
    gtk_widget_set_name(new_tab_btn, "new-tab-btn");
    g_signal_connect(new_tab_btn, "clicked", G_CALLBACK(on_new_tab_btn), bw);
    gtk_box_pack_end(GTK_BOX(tab_bar_hbox), new_tab_btn, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(bw->main_vbox), tab_bar_hbox, FALSE, FALSE, 0);

    /* ── Toolbar ───────────────────────────────────────────────── */

    bw->toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_widget_set_name(bw->toolbar, "toolbar");

    bw->back_btn = gtk_button_new_with_label("\xe2\x97\x80"); /* ◀ */
    bw->forward_btn = gtk_button_new_with_label("\xe2\x96\xb6"); /* ▶ */
    bw->reload_btn = gtk_button_new_with_label("\xe2\x9f\xb3"); /* ⟳ */
    bw->home_btn = gtk_button_new_with_label("\xe2\x8c\x82"); /* ⌂ */

    g_signal_connect(bw->back_btn, "clicked", G_CALLBACK(on_back), bw);
    g_signal_connect(bw->forward_btn, "clicked", G_CALLBACK(on_forward), bw);
    g_signal_connect(bw->reload_btn, "clicked", G_CALLBACK(on_reload), bw);
    g_signal_connect(bw->home_btn, "clicked", G_CALLBACK(on_home), bw);

    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->back_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->forward_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->reload_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->home_btn, FALSE, FALSE, 0);

    bw->url_entry = gtk_entry_new();
    gtk_widget_set_name(bw->url_entry, "url-entry");
    gtk_entry_set_placeholder_text(GTK_ENTRY(bw->url_entry),
        "Enter URL, file path, or HTML...");
    g_signal_connect(bw->url_entry, "activate", G_CALLBACK(on_url_activate), bw);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->url_entry, TRUE, TRUE, 4);

    /* Plugin toggle buttons. */
    bw->privacy_btn = gtk_button_new_with_label("Privacy");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(bw->privacy_btn), "plugin-btn");
    g_signal_connect(bw->privacy_btn, "clicked",
                     G_CALLBACK(on_privacy_toggle), bw);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->privacy_btn, FALSE, FALSE, 0);

    bw->darkmode_btn = gtk_button_new_with_label("Dark");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(bw->darkmode_btn), "plugin-btn");
    g_signal_connect(bw->darkmode_btn, "clicked",
                     G_CALLBACK(on_darkmode_toggle), bw);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->darkmode_btn, FALSE, FALSE, 0);

    bw->cookie_btn = gtk_button_new_with_label("Cookies");
    gtk_style_context_add_class(
        gtk_widget_get_style_context(bw->cookie_btn), "plugin-btn");
    g_signal_connect(bw->cookie_btn, "clicked",
                     G_CALLBACK(on_cookie_toggle), bw);
    gtk_box_pack_start(GTK_BOX(bw->toolbar), bw->cookie_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(bw->main_vbox), bw->toolbar, FALSE, FALSE, 0);

    /* ── Content area + scrollbar ──────────────────────────────── */

    bw->content_scroll_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    bw->content_area = gtk_drawing_area_new();
    gtk_widget_set_name(bw->content_area, "content-area");
    gtk_widget_set_can_focus(bw->content_area, TRUE);
    gtk_widget_add_events(bw->content_area,
        GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK |
        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
        GDK_POINTER_MOTION_MASK);

    g_signal_connect(bw->content_area, "draw", G_CALLBACK(on_draw), bw);
    g_signal_connect(bw->content_area, "scroll-event",
                     G_CALLBACK(on_scroll_event), bw);
    g_signal_connect(bw->content_area, "size-allocate",
                     G_CALLBACK(on_content_resize), bw);

    gtk_box_pack_start(GTK_BOX(bw->content_scroll_box),
                       bw->content_area, TRUE, TRUE, 0);

    /* Scrollbar. */
    bw->scroll_adj = gtk_adjustment_new(0, 0, 1000, 20, 200, 200);
    bw->scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, bw->scroll_adj);
    g_signal_connect(bw->scroll_adj, "value-changed",
                     G_CALLBACK(on_scroll_changed), bw);
    gtk_box_pack_end(GTK_BOX(bw->content_scroll_box),
                     bw->scrollbar, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(bw->main_vbox),
                       bw->content_scroll_box, TRUE, TRUE, 0);

    /* ── Status bar ────────────────────────────────────────────── */

    bw->status_bar = gtk_label_new("Ready");
    gtk_widget_set_name(bw->status_bar, "status-bar");
    gtk_label_set_xalign(GTK_LABEL(bw->status_bar), 0.0f);
    gtk_box_pack_end(GTK_BOX(bw->main_vbox), bw->status_bar, FALSE, FALSE, 0);

    /* ── Initialize plugin system ──────────────────────────────── */

    bw->plugin_registry = plugin_registry_create();
    bw->plugin_context = plugin_context_create();
    bw->plugin_pipeline = plugin_pipeline_create(bw->plugin_registry,
                                                  bw->plugin_context);

    /* Register built-in plugins (all disabled by default). */
    plugin_registry_register(bw->plugin_registry, privacy_shield_create());
    plugin_registry_register(bw->plugin_registry, dark_mode_create());
    plugin_registry_register(bw->plugin_registry, cookie_manager_create());

    /* Enable cookie manager (always registered, gated by per-tab toggle). */
    plugin_registry_enable(bw->plugin_registry, "cookie_manager");

    /* ── Create first tab ──────────────────────────────────────── */

    int first = browser_add_tab(bw);
    browser_switch_tab(bw, first);

    /* Show everything. */
    gtk_widget_show_all(bw->window);

    /* Load home page. */
    browser_navigate(bw, "about:home");

    return bw;
}

void browser_window_free(BrowserWindow *bw)
{
    if (!bw) return;

    for (int i = 0; i < bw->tab_count; i++) {
        tab_free_content(&bw->tabs[i]);
        tab_free_history(&bw->tabs[i]);
    }

    /* Clean up plugin system. */
    if (bw->plugin_pipeline) plugin_pipeline_free(bw->plugin_pipeline);
    if (bw->plugin_registry) plugin_registry_free(bw->plugin_registry);
    if (bw->plugin_context)  plugin_context_free(bw->plugin_context);

    font_system_shutdown();
    free(bw);
}

void browser_run(void)
{
    gtk_main();
}
