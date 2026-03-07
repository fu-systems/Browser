/*
 * Pane — Browser UI (GTK3)
 *
 * Full browser window with:
 *   - Tab bar (multiple tabs)
 *   - Navigation toolbar (back, forward, reload, address bar)
 *   - Content area (Cairo-rendered page via the Pane engine)
 *   - Status bar
 *   - Keyboard and mouse navigation
 *   - Scrolling
 */

#ifndef PANE_BROWSER_H
#define PANE_BROWSER_H

#include "../pane.h"
#include "../paint/cairo_backend.h"
#include "../plugin/plugin.h"
#include "../plugin/plugin_registry.h"
#include "../plugin/plugin_pipeline.h"
#include "../plugin/builtin/privacy_shield.h"
#include "../plugin/builtin/dark_mode.h"
#include "../plugin/builtin/cookie_manager.h"
#include <gtk/gtk.h>

/* ── Tab ───────────────────────────────────────────────────────────── */

#define MAX_TABS 32
#define MAX_HISTORY 64
#define MAX_FORM_VALUES 128

/* Stores a form field value (kept per-tab so values survive re-render). */
typedef struct {
    const DomNode  *node;      /* DOM node this value belongs to */
    char           *value;     /* heap-allocated current value */
} FormFieldValue;

typedef struct {
    char         title[256];
    char         url[2048];

    /* Rendering result for this tab. */
    PaneResult   result;
    bool         has_content;

    /* Scroll position. */
    float        scroll_x;
    float        scroll_y;
    float        content_height;

    /* History. */
    char        *history[MAX_HISTORY];
    int          history_count;
    int          history_pos;   /* -1 = no history */

    /* Per-tab plugin settings. */
    bool         cookies_enabled;  /* per-tab cookie toggle */

    /* Form interaction state. */
    FormFieldValue  form_values[MAX_FORM_VALUES];
    int             form_value_count;

    /* Tab widget (label in tab bar). */
    GtkWidget   *tab_label;
    GtkWidget   *tab_button;   /* close button */
    GtkWidget   *tab_box;      /* hbox containing label + button */
} BrowserTab;

/* ── Browser Window ────────────────────────────────────────────────── */

typedef struct {
    /* GTK widgets. */
    GtkWidget   *window;
    GtkWidget   *main_vbox;
    GtkWidget   *tab_bar;        /* horizontal box for tabs */
    GtkWidget   *toolbar;
    GtkWidget   *back_btn;
    GtkWidget   *forward_btn;
    GtkWidget   *reload_btn;
    GtkWidget   *home_btn;
    GtkWidget   *url_entry;
    GtkWidget   *content_area;   /* GtkDrawingArea */
    GtkWidget   *status_bar;
    GtkWidget   *scrollbar;      /* vertical scrollbar */
    GtkWidget   *content_scroll_box; /* hbox: content_area + scrollbar */
    GtkAdjustment *scroll_adj;

    /* Tab state. */
    BrowserTab   tabs[MAX_TABS];
    int          tab_count;
    int          active_tab;

    /* Renderer. */
    bool         fonts_ready;

    /* Viewport. */
    float        viewport_width;
    float        viewport_height;

    /* Plugin system. */
    PluginRegistry  *plugin_registry;
    PluginContext   *plugin_context;
    PluginPipeline  *plugin_pipeline;

    /* Plugin toolbar widgets. */
    GtkWidget       *privacy_btn;
    GtkWidget       *darkmode_btn;
    GtkWidget       *cookie_btn;

    /* Form overlay: native GTK widgets positioned over form elements. */
    GtkWidget       *content_overlay;   /* GtkOverlay wrapping content_area */
    GtkWidget       *form_fixed;        /* GtkFixed for positioned form widgets */
    GtkWidget       *active_form_widget; /* Currently focused form widget */
    const DomNode   *active_form_node;   /* DOM node of focused form element */
} BrowserWindow;

/* ── API ────────────────────────────────────────────────────────────── */

/* Create and show a browser window. Returns the window struct. */
BrowserWindow *browser_window_new(void);

/* Free browser resources. */
void browser_window_free(BrowserWindow *bw);

/* Navigate the active tab to a URL or HTML content. */
void browser_navigate(BrowserWindow *bw, const char *url);

/* Navigate to raw HTML string. */
void browser_load_html(BrowserWindow *bw, const char *html, const char *title);

/* Tab management. */
int  browser_add_tab(BrowserWindow *bw);
void browser_close_tab(BrowserWindow *bw, int tab_idx);
void browser_switch_tab(BrowserWindow *bw, int tab_idx);

/* Run the GTK main loop. Call after browser_window_new(). */
void browser_run(void);

#endif /* PANE_BROWSER_H */
