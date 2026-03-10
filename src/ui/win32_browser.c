/*
 * Pane — Win32 Browser UI Implementation
 *
 * Native Windows browser chrome using Win32 API.
 * Uses GDI for 2D rendering with FreeType for text.
 *
 * Features synced with GTK3/Linux build:
 *   - External CSS fetching for <link> stylesheets
 *   - Link clicking with hit-testing
 *   - URL resolution (relative/absolute)
 *   - History stack with back/forward
 *   - Mouse hover cursor changes
 *   - Form element detection and submission
 *   - Keyboard shortcuts (Ctrl+L, Ctrl+R, Alt+arrows, etc.)
 *   - Status bar
 *   - UTF-8 sanitization
 */

#ifdef _WIN32

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "win32_browser.h"
#include "browser_common.h"
#include "../pane.h"
#include "../font/font.h"
#include "../net/http.h"
#include "../style/computed.h"
#include "../layout/box.h"
#include "../layout/inline.h"
#include "../html/tree_builder.h"
#include "../dom/dom.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

/* ── Constants ─────────────────────────────────────────────────────── */

#define IDC_URL_ENTRY  1001
#define IDC_BACK_BTN   1002
#define IDC_FWD_BTN    1003
#define IDC_RELOAD_BTN 1004
#define IDC_HOME_BTN   1005
#define IDC_GO_BTN     1006

#define TOOLBAR_HEIGHT 36
#define STATUSBAR_HEIGHT 22
#define MAX_URL 2048
#define MAX_HISTORY 64

/* ── State ─────────────────────────────────────────────────────────── */

static struct {
    HWND     hwnd;
    HWND     url_entry;
    HWND     back_btn;
    HWND     fwd_btn;
    HWND     reload_btn;
    HWND     home_btn;
    HWND     go_btn;
    HWND     status_label;

    PaneResult result;
    int        has_content;

    float    scroll_y;
    float    content_height;
    float    viewport_w;
    float    viewport_h;

    char     current_url[MAX_URL];
    PaneFont *font_regular;

    /* History stack. */
    char    *history[MAX_HISTORY];
    int      history_count;
    int      history_pos;   /* -1 = no history */

    /* Cursor tracking. */
    HCURSOR  cursor_arrow;
    HCURSOR  cursor_hand;
    HCURSOR  cursor_ibeam;
    HCURSOR  current_cursor;
} g;

/* ── Forward declarations ──────────────────────────────────────────── */

static void navigate_impl(const char *url, int push_history);
static void navigate(const char *url);
static void update_nav_buttons(void);
static void set_status(const char *text);

/* ── GDI text measurement callback ─────────────────────────────────── */

static HFONT create_gdi_font(float font_size, bool bold, const char *family)
{
    int font_height = -(int)(font_size * 96.0f / 72.0f);
    int weight = bold ? FW_BOLD : FW_NORMAL;

    return CreateFontA(
        font_height, 0, 0, 0, weight,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
        family ? family : "Segoe UI");
}

static float win32_measure_text_cb(const char *text, size_t len,
                                    float font_size, bool bold,
                                    bool monospace)
{
    if (!text || len == 0) return 0;

    const char *family = monospace ? "Courier New" : "Segoe UI";
    HFONT hfont = create_gdi_font(font_size, bold, family);

    HDC hdc = CreateCompatibleDC(NULL);
    HFONT old_font = (HFONT)SelectObject(hdc, hfont);

    /* Convert UTF-8 to wide string. */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, (int)len, NULL, 0);
    if (wlen <= 0) {
        SelectObject(hdc, old_font);
        DeleteObject(hfont);
        DeleteDC(hdc);
        return (float)len * font_size * 0.6f;
    }

    wchar_t *wtext = (wchar_t *)malloc((wlen + 1) * sizeof(wchar_t));
    if (!wtext) {
        SelectObject(hdc, old_font);
        DeleteObject(hfont);
        DeleteDC(hdc);
        return (float)len * font_size * 0.6f;
    }
    MultiByteToWideChar(CP_UTF8, 0, text, (int)len, wtext, wlen);

    SIZE sz;
    GetTextExtentPoint32W(hdc, wtext, wlen, &sz);
    float width = (float)sz.cx;

    free(wtext);
    SelectObject(hdc, old_font);
    DeleteObject(hfont);
    DeleteDC(hdc);

    return width;
}

/* ── Home page ─────────────────────────────────────────────────────── */

static const char *HOME_HTML =
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

/* ── History management ────────────────────────────────────────────── */

static void push_history(const char *url)
{
    /* Truncate forward history. */
    for (int i = g.history_pos + 1; i < g.history_count; i++) {
        free(g.history[i]);
        g.history[i] = NULL;
    }
    g.history_count = g.history_pos + 1;

    if (g.history_count >= MAX_HISTORY) {
        /* Shift out oldest. */
        free(g.history[0]);
        memmove(g.history, g.history + 1,
                (MAX_HISTORY - 1) * sizeof(char *));
        g.history_count = MAX_HISTORY - 1;
    }

    g.history[g.history_count] = _strdup(url);
    g.history_pos = g.history_count;
    g.history_count++;
}

static void free_history(void)
{
    for (int i = 0; i < g.history_count; i++) {
        free(g.history[i]);
        g.history[i] = NULL;
    }
    g.history_count = 0;
    g.history_pos = -1;
}

/* ── Status bar helper ─────────────────────────────────────────────── */

static void set_status(const char *text)
{
    if (g.status_label)
        SetWindowTextA(g.status_label, text ? text : "");
}

/* ── Navigation button state ───────────────────────────────────────── */

static void update_nav_buttons(void)
{
    EnableWindow(g.back_btn, g.history_pos > 0);
    EnableWindow(g.fwd_btn, g.history_pos < g.history_count - 1);
}

/* ── Load page ─────────────────────────────────────────────────────── */

static void load_page(const char *html, size_t len)
{
    if (g.has_content) {
        pane_result_free(&g.result);
        g.has_content = 0;
    }

    /* Fetch external CSS from <link> tags (matches Linux behavior). */
    size_t ext_css_len = 0;
    char *ext_css = NULL;
    if (g.current_url[0] && strncmp(g.current_url, "http", 4) == 0) {
        ext_css = fetch_external_css(html, len, g.current_url, &ext_css_len);
    }

    g.result = pane_render(html, len, ext_css, ext_css_len,
                            g.viewport_w, g.viewport_h);
    free(ext_css);
    g.has_content = 1;
    g.scroll_y = 0;

    if (g.result.layout_tree && g.result.layout_tree->root) {
        LayoutBox *r = g.result.layout_tree->root;
        g.content_height = r->rect.y + r->rect.height + r->margin.bottom + 40;
    }

    InvalidateRect(g.hwnd, NULL, TRUE);

    /* Update title. */
    if (g.result.document && g.result.document->head) {
        DomNode *head = g.result.document->head;
        for (DomNode *n = head->first_child; n; n = n->next_sibling) {
            if (n->type == PANE_NODE_ELEMENT && n->elem.tag == TAG_TITLE) {
                DomNode *tc = n->first_child;
                if (tc && tc->type == PANE_NODE_TEXT && tc->text.data) {
                    char title[512];
                    int tlen = tc->text.len < 255 ? (int)tc->text.len : 255;
                    snprintf(title, sizeof(title), "%.*s - Pane", tlen, tc->text.data);
                    wchar_t wtitle[512];
                    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 512);
                    SetWindowTextW(g.hwnd, wtitle);
                }
                break;
            }
        }
    }
}

static void show_error(const char *url, const char *detail)
{
    char err[4096];
    snprintf(err, sizeof(err),
        "<html><head><title>Error</title>"
        "<style>body{font-family:sans-serif;margin:60px;background:#fff5f5;color:#333}"
        "h1{color:#c0392b;font-size:24px}"
        "p{font-size:14px;color:#666}"
        ".url{font-family:monospace;background:#f0f0f0;padding:4px 8px}</style></head>"
        "<body><h1>Page Load Error</h1>"
        "<p>Could not load: <span class=\"url\">%s</span></p>"
        "<p>%s</p></body></html>",
        url, detail);
    load_page(err, strlen(err));
}

/* ── Navigation ────────────────────────────────────────────────────── */

static void navigate_impl(const char *url, int push_hist)
{
    strncpy(g.current_url, url, MAX_URL - 1);
    g.current_url[MAX_URL - 1] = '\0';

    if (strcmp(url, "about:home") == 0 || url[0] == '\0') {
        if (push_hist) push_history(url);
        load_page(HOME_HTML, strlen(HOME_HTML));
        SetWindowTextA(g.url_entry, url);
        update_nav_buttons();
        set_status("Ready");
        return;
    }

    /* Inline HTML. */
    if (url[0] == '<') {
        if (push_hist) push_history("about:html");
        load_page(url, strlen(url));
        SetWindowTextA(g.url_entry, "about:html");
        update_nav_buttons();
        return;
    }

    /* File path. */
    if (strncmp(url, "file://", 7) == 0 || url[0] == '/' ||
        (url[0] != '\0' && url[1] == ':')) {
        const char *path = url;
        if (strncmp(url, "file:///", 8) == 0) path = url + 8;
        else if (strncmp(url, "file://", 7) == 0) path = url + 7;

        FILE *f = fopen(path, "rb");
        if (!f) {
            show_error(url, "File not found.");
            update_nav_buttons();
            return;
        }

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *buf = (char *)malloc(sz + 1);
        if (buf) {
            fread(buf, 1, sz, f);
            buf[sz] = '\0';
            if (push_hist) push_history(url);
            load_page(buf, sz);
            free(buf);
        }
        fclose(f);
        SetWindowTextA(g.url_entry, url);
        update_nav_buttons();
        return;
    }

    /* HTTP / HTTPS URLs. */
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        set_status("Loading...");

        HttpResponse *resp = http_get(url, 5);

        if (resp->error) {
            show_error(url, resp->error);
            if (push_hist) push_history(url);
            set_status("Error");
        } else if (resp->body && resp->body_len > 0) {
            /* Sanitize UTF-8 before rendering (matches Linux behavior). */
            sanitize_utf8(resp->body, resp->body_len);
            if (push_hist) push_history(url);
            load_page(resp->body, resp->body_len);

            char status_msg[256];
            snprintf(status_msg, sizeof(status_msg),
                     "HTTP %d — %zu bytes", resp->status_code, resp->body_len);
            set_status(status_msg);
        } else {
            show_error(url, "Empty response.");
            if (push_hist) push_history(url);
            set_status("Empty response");
        }

        http_response_free(resp);
        SetWindowTextA(g.url_entry, g.current_url);
        update_nav_buttons();
        return;
    }

    /* Bare domain name — try as https:// */
    if (strchr(url, '.') && url[0] != '<' && url[0] != '/') {
        char full_url[2048];
        snprintf(full_url, sizeof(full_url), "https://%s", url);
        navigate_impl(full_url, push_hist);
        return;
    }

    show_error(url, "Unsupported URL scheme.");
    SetWindowTextA(g.url_entry, url);
    update_nav_buttons();
}

static void navigate(const char *url)
{
    navigate_impl(url, 1);
}

/* ── Form submission helper ────────────────────────────────────────── */

static void handle_form_submit(const DomNode *form_node)
{
    if (!form_node) return;

    const char *action = elem_get_attr(form_node, "action");
    const char *method = elem_get_attr(form_node, "method");
    int is_get = !method || _stricmp(method, "get") == 0;

    /* Build query string from form fields. */
    char query[4096] = {0};
    size_t qlen = 0;

    for (DomNode *n = dom_next_in_tree(form_node, form_node); n;
         n = dom_next_in_tree(n, form_node)) {
        if (n->type != PANE_NODE_ELEMENT) continue;
        if (n->elem.tag != TAG_INPUT && n->elem.tag != TAG_TEXTAREA)
            continue;

        const char *name = elem_get_attr(n, "name");
        if (!name || !name[0]) continue;

        /* Skip non-submittable input types. */
        if (n->elem.tag == TAG_INPUT) {
            const char *itype = elem_get_attr(n, "type");
            if (itype && (strcmp(itype, "submit") == 0 ||
                          strcmp(itype, "button") == 0 ||
                          strcmp(itype, "reset") == 0))
                continue;
        }

        const char *val = elem_get_attr(n, "value");
        if (!val) val = "";

        if (qlen > 0 && qlen < sizeof(query) - 1)
            query[qlen++] = '&';

        int written = snprintf(query + qlen, sizeof(query) - qlen,
                               "%s=%s", name, val);
        if (written > 0) qlen += (size_t)written;
    }

    /* Build final URL. */
    char final_url[4096];
    if (action && action[0]) {
        char resolved_action[2048];
        resolve_url(g.current_url, action, resolved_action,
                    sizeof(resolved_action));
        if (is_get && qlen > 0)
            snprintf(final_url, sizeof(final_url), "%s?%s",
                     resolved_action, query);
        else
            snprintf(final_url, sizeof(final_url), "%s", resolved_action);
    } else {
        if (is_get && qlen > 0)
            snprintf(final_url, sizeof(final_url), "%s?%s",
                     g.current_url, query);
        else
            snprintf(final_url, sizeof(final_url), "%s", g.current_url);
    }

    navigate(final_url);
}

/* ── Render layout box via GDI ─────────────────────────────────────── */

static void paint_box_gdi(HDC hdc, const LayoutBox *box, float ox, float oy)
{
    if (!box) return;
    if (box->style && box->style->display == DISPLAY_NONE) return;

    ComputedStyle *s = box->style;
    float x = ox + box->rect.x;
    float y = oy + box->rect.y - g.scroll_y;

    /* Background. */
    if (s && s->background_color.a > 0) {
        float bx = x - box->padding.left - box->border.left;
        float by = y - box->padding.top - box->border.top;
        float bw = box->border.left + box->padding.left + box->rect.width +
                   box->padding.right + box->border.right;
        float bh = box->border.top + box->padding.top + box->rect.height +
                   box->padding.bottom + box->border.bottom;

        HBRUSH brush = CreateSolidBrush(
            RGB(s->background_color.r, s->background_color.g, s->background_color.b));
        RECT rc = { (int)bx, (int)by, (int)(bx + bw), (int)(by + bh) };
        FillRect(hdc, &rc, brush);
        DeleteObject(brush);
    }

    /* Borders. */
    if (s && (box->border.top > 0 || box->border.left > 0)) {
        float bx = x - box->padding.left - box->border.left;
        float by = y - box->padding.top - box->border.top;
        float bw = box->border.left + box->padding.left + box->rect.width +
                   box->padding.right + box->border.right;
        float bh = box->border.top + box->padding.top + box->rect.height +
                   box->padding.bottom + box->border.bottom;

        HPEN pen = CreatePen(PS_SOLID, (int)box->border.top,
            RGB(s->border_top_color.r, s->border_top_color.g, s->border_top_color.b));
        HPEN old_pen = (HPEN)SelectObject(hdc, pen);
        HBRUSH old_brush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        Rectangle(hdc, (int)bx, (int)by, (int)(bx + bw), (int)(by + bh));

        SelectObject(hdc, old_brush);
        SelectObject(hdc, old_pen);
        DeleteObject(pen);
    }

    /* Text. */
    if (box->type == BOX_TEXT && box->text && box->text_len > 0 && s) {
        bool bold = s->font_weight >= 600;
        HFONT hfont = create_gdi_font(s->font_size, bold, s->font_family);

        HFONT old_font = (HFONT)SelectObject(hdc, hfont);
        SetTextColor(hdc, RGB(s->color.r, s->color.g, s->color.b));
        SetBkMode(hdc, TRANSPARENT);

        /* Convert UTF-8 to wide string. */
        int wlen = MultiByteToWideChar(CP_UTF8, 0, box->text, (int)box->text_len, NULL, 0);
        wchar_t *wtext = (wchar_t *)malloc((wlen + 1) * sizeof(wchar_t));
        if (wtext) {
            MultiByteToWideChar(CP_UTF8, 0, box->text, (int)box->text_len, wtext, wlen);

            /* Use DrawTextW with word wrapping within the box content width. */
            RECT text_rc;
            text_rc.left   = (int)x;
            text_rc.top    = (int)y;
            text_rc.right  = (int)(x + box->rect.width);
            text_rc.bottom = (int)(y + box->rect.height);
            DrawTextW(hdc, wtext, wlen, &text_rc, DT_LEFT | DT_TOP | DT_WORDBREAK | DT_NOPREFIX);

            free(wtext);
        }

        SelectObject(hdc, old_font);
        DeleteObject(hfont);
    }

    /* Children. */
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        paint_box_gdi(hdc, child, x, oy + box->rect.y);
    }
}

/* ── Content area click handler ────────────────────────────────────── */

static void handle_content_click(int mx, int my)
{
    if (!g.has_content || !g.result.layout_tree || !g.result.layout_tree->root)
        return;

    /* Convert window coordinates to page coordinates. */
    float px = (float)mx;
    float py = (float)(my - TOOLBAR_HEIGHT) + g.scroll_y;

    const LayoutBox *hit = hit_test_box(g.result.layout_tree->root,
                                         px, py, 0, 0);
    if (!hit) return;

    /* Check for form elements first (higher priority than links). */
    const DomNode *form_node = find_form_element(hit);
    if (form_node) {
        if (is_button_element(form_node)) {
            /* Handle button/submit click. */
            const char *type = NULL;
            if (form_node->elem.tag == TAG_INPUT)
                type = elem_get_attr(form_node, "type");
            else if (form_node->elem.tag == TAG_BUTTON) {
                type = elem_get_attr(form_node, "type");
                if (!type) type = "submit";
            }

            if (type && strcmp(type, "submit") == 0) {
                const DomNode *form = find_form_parent(form_node);
                if (form) {
                    handle_form_submit(form);
                } else {
                    set_status("Submit button: no parent form found");
                }
            } else {
                set_status("Button clicked");
            }
            return;
        }
        if (is_text_input(form_node)) {
            set_status("Text input clicked (editing not yet supported)");
            return;
        }
    }

    /* Check for link. */
    const DomNode *link_node = find_link_ancestor(hit);
    if (link_node) {
        const char *href = elem_get_attr(link_node, "href");
        if (href && href[0]) {
            char resolved[2048];
            resolve_url(g.current_url, href, resolved, sizeof(resolved));
            if (resolved[0]) {
                SetWindowTextA(g.url_entry, resolved);
                navigate(resolved);
            }
        }
    }
}

/* ── Mouse hover handler ───────────────────────────────────────────── */

static void handle_content_mousemove(int mx, int my)
{
    if (my < TOOLBAR_HEIGHT) {
        g.current_cursor = g.cursor_arrow;
        return;
    }

    if (!g.has_content || !g.result.layout_tree || !g.result.layout_tree->root) {
        g.current_cursor = g.cursor_arrow;
        return;
    }

    float px = (float)mx;
    float py = (float)(my - TOOLBAR_HEIGHT) + g.scroll_y;

    const LayoutBox *hit = hit_test_box(g.result.layout_tree->root,
                                         px, py, 0, 0);
    if (hit) {
        /* Check form elements. */
        const DomNode *form_node = find_form_element(hit);
        if (form_node && is_text_input(form_node)) {
            g.current_cursor = g.cursor_ibeam;
            set_status("");
            return;
        }
        if (form_node && is_button_element(form_node)) {
            g.current_cursor = g.cursor_hand;
            set_status("");
            return;
        }

        /* Check links. */
        const DomNode *link_node = find_link_ancestor(hit);
        if (link_node) {
            g.current_cursor = g.cursor_hand;
            const char *href = elem_get_attr(link_node, "href");
            if (href) {
                char resolved[2048];
                resolve_url(g.current_url, href, resolved, sizeof(resolved));
                set_status(resolved);
            }
            return;
        }
    }

    g.current_cursor = g.cursor_arrow;
    set_status("");
}

/* ── Window procedure ──────────────────────────────────────────────── */

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hInst = ((CREATESTRUCT *)lParam)->hInstance;

        /* Create toolbar controls. */
        g.back_btn = CreateWindowA("BUTTON", "<", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            4, 4, 30, 28, hwnd, (HMENU)IDC_BACK_BTN, hInst, NULL);
        g.fwd_btn = CreateWindowA("BUTTON", ">", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            38, 4, 30, 28, hwnd, (HMENU)IDC_FWD_BTN, hInst, NULL);
        g.reload_btn = CreateWindowA("BUTTON", "R", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            72, 4, 30, 28, hwnd, (HMENU)IDC_RELOAD_BTN, hInst, NULL);
        g.home_btn = CreateWindowA("BUTTON", "H", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            106, 4, 30, 28, hwnd, (HMENU)IDC_HOME_BTN, hInst, NULL);

        RECT rc;
        GetClientRect(hwnd, &rc);
        int url_w = rc.right - 210;

        g.url_entry = CreateWindowA("EDIT", "about:home",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            144, 6, url_w > 100 ? url_w : 100, 24,
            hwnd, (HMENU)IDC_URL_ENTRY, hInst, NULL);

        g.go_btn = CreateWindowA("BUTTON", "Go", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            rc.right - 60, 4, 52, 28, hwnd, (HMENU)IDC_GO_BTN, hInst, NULL);

        /* Status bar at the bottom. */
        g.status_label = CreateWindowA("STATIC", "Ready",
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
            4, rc.bottom - STATUSBAR_HEIGHT + 2, rc.right - 8, STATUSBAR_HEIGHT - 4,
            hwnd, NULL, hInst, NULL);

        /* Set font for controls. */
        HFONT ui_font = CreateFontA(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        SendMessage(g.url_entry, WM_SETFONT, (WPARAM)ui_font, TRUE);
        SendMessage(g.back_btn, WM_SETFONT, (WPARAM)ui_font, TRUE);
        SendMessage(g.fwd_btn, WM_SETFONT, (WPARAM)ui_font, TRUE);
        SendMessage(g.reload_btn, WM_SETFONT, (WPARAM)ui_font, TRUE);
        SendMessage(g.home_btn, WM_SETFONT, (WPARAM)ui_font, TRUE);
        SendMessage(g.go_btn, WM_SETFONT, (WPARAM)ui_font, TRUE);

        HFONT status_font = CreateFontA(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
        SendMessage(g.status_label, WM_SETFONT, (WPARAM)status_font, TRUE);

        /* Initialize button states. */
        update_nav_buttons();

        /* Load home page. */
        navigate("about:home");
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        float new_w = (float)w;
        float new_h = (float)(h - TOOLBAR_HEIGHT - STATUSBAR_HEIGHT);

        int size_changed = (new_w != g.viewport_w || new_h != g.viewport_h);
        g.viewport_w = new_w;
        g.viewport_h = new_h;

        /* Resize URL entry and go button. */
        int url_w = w - 210;
        MoveWindow(g.url_entry, 144, 6, url_w > 100 ? url_w : 100, 24, TRUE);
        MoveWindow(g.go_btn, w - 60, 4, 52, 28, TRUE);

        /* Resize status bar. */
        MoveWindow(g.status_label, 4, h - STATUSBAR_HEIGHT + 2,
                   w - 8, STATUSBAR_HEIGHT - 4, TRUE);

        /* Re-layout page at new viewport dimensions. */
        if (size_changed && g.has_content) {
            navigate_impl(g.current_url, 0);
        } else {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        /* Toolbar background. */
        HBRUSH toolbar_brush = CreateSolidBrush(RGB(240, 240, 240));
        RECT toolbar_rc = { 0, 0, rc.right, TOOLBAR_HEIGHT };
        FillRect(hdc, &toolbar_rc, toolbar_brush);
        DeleteObject(toolbar_brush);

        /* Status bar background. */
        HBRUSH status_brush = CreateSolidBrush(RGB(240, 240, 240));
        RECT status_rc = { 0, rc.bottom - STATUSBAR_HEIGHT, rc.right, rc.bottom };
        FillRect(hdc, &status_rc, status_brush);
        DeleteObject(status_brush);

        /* Content area. */
        RECT content_rc = { 0, TOOLBAR_HEIGHT, rc.right, rc.bottom - STATUSBAR_HEIGHT };
        HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &content_rc, white);
        DeleteObject(white);

        /* Render page. */
        if (g.has_content && g.result.layout_tree && g.result.layout_tree->root) {
            /* Clip to content area. */
            HRGN clip = CreateRectRgn(0, TOOLBAR_HEIGHT, rc.right,
                                       rc.bottom - STATUSBAR_HEIGHT);
            SelectClipRgn(hdc, clip);

            /* Offset for toolbar. */
            SetViewportOrgEx(hdc, 0, TOOLBAR_HEIGHT, NULL);
            paint_box_gdi(hdc, g.result.layout_tree->root, 0, 0);
            SetViewportOrgEx(hdc, 0, 0, NULL);

            SelectClipRgn(hdc, NULL);
            DeleteObject(clip);
        }

        /* Toolbar separator line. */
        HPEN sep_pen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN old_pen = (HPEN)SelectObject(hdc, sep_pen);
        MoveToEx(hdc, 0, TOOLBAR_HEIGHT, NULL);
        LineTo(hdc, rc.right, TOOLBAR_HEIGHT);
        /* Status bar separator. */
        MoveToEx(hdc, 0, rc.bottom - STATUSBAR_HEIGHT, NULL);
        LineTo(hdc, rc.right, rc.bottom - STATUSBAR_HEIGHT);
        SelectObject(hdc, old_pen);
        DeleteObject(sep_pen);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        g.scroll_y -= (float)delta * 0.5f;
        if (g.scroll_y < 0) g.scroll_y = 0;
        float max_scroll = g.content_height - g.viewport_h;
        if (max_scroll < 0) max_scroll = 0;
        if (g.scroll_y > max_scroll) g.scroll_y = max_scroll;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        /* Only handle clicks in content area (below toolbar, above status). */
        if (my >= TOOLBAR_HEIGHT) {
            handle_content_click(mx, my);
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);
        handle_content_mousemove(mx, my);
        SetCursor(g.current_cursor);
        return 0;
    }

    case WM_SETCURSOR: {
        if (LOWORD(lParam) == HTCLIENT) {
            SetCursor(g.current_cursor ? g.current_cursor : g.cursor_arrow);
            return TRUE;
        }
        break;
    }

    case WM_KEYDOWN: {
        int ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        int alt  = (GetKeyState(VK_MENU)    & 0x8000) != 0;

        /* Ctrl+L: Focus URL bar. */
        if (ctrl && wParam == 'L') {
            SetFocus(g.url_entry);
            SendMessage(g.url_entry, EM_SETSEL, 0, -1);
            return 0;
        }

        /* Ctrl+R / F5: Reload. */
        if ((ctrl && wParam == 'R') || wParam == VK_F5) {
            navigate_impl(g.current_url, 0);
            return 0;
        }

        /* Alt+Left: Back. */
        if (alt && wParam == VK_LEFT) {
            if (g.history_pos > 0) {
                g.history_pos--;
                navigate_impl(g.history[g.history_pos], 0);
                SetWindowTextA(g.url_entry, g.current_url);
                update_nav_buttons();
            }
            return 0;
        }

        /* Alt+Right: Forward. */
        if (alt && wParam == VK_RIGHT) {
            if (g.history_pos < g.history_count - 1) {
                g.history_pos++;
                navigate_impl(g.history[g.history_pos], 0);
                SetWindowTextA(g.url_entry, g.current_url);
                update_nav_buttons();
            }
            return 0;
        }

        /* Backspace: Back (when not in URL bar). */
        if (wParam == VK_BACK && GetFocus() != g.url_entry) {
            if (g.history_pos > 0) {
                g.history_pos--;
                navigate_impl(g.history[g.history_pos], 0);
                SetWindowTextA(g.url_entry, g.current_url);
                update_nav_buttons();
            }
            return 0;
        }

        if (wParam == VK_RETURN && GetFocus() == g.url_entry) {
            char url[MAX_URL];
            GetWindowTextA(g.url_entry, url, MAX_URL);
            navigate(url);
            SetFocus(hwnd);
            return 0;
        }

        /* Scroll keys (when not in URL bar). */
        if (GetFocus() != g.url_entry) {
            float page = g.viewport_h * 0.8f;
            float max_s = g.content_height - g.viewport_h;
            if (max_s < 0) max_s = 0;

            if (wParam == VK_DOWN)   g.scroll_y += 40;
            if (wParam == VK_UP)     g.scroll_y -= 40;
            if (wParam == VK_NEXT)   g.scroll_y += page;
            if (wParam == VK_PRIOR)  g.scroll_y -= page;
            if (wParam == VK_HOME)   g.scroll_y = 0;
            if (wParam == VK_END)    g.scroll_y = max_s;
            if (wParam == VK_SPACE)  g.scroll_y += page;

            if (g.scroll_y < 0) g.scroll_y = 0;
            if (g.scroll_y > max_s) g.scroll_y = max_s;
            InvalidateRect(hwnd, NULL, TRUE);
        }
        return 0;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);

        if (id == IDC_GO_BTN) {
            char url[MAX_URL];
            GetWindowTextA(g.url_entry, url, MAX_URL);
            navigate(url);
            SetFocus(hwnd);
        }
        if (id == IDC_HOME_BTN) {
            SetWindowTextA(g.url_entry, "about:home");
            navigate("about:home");
        }
        if (id == IDC_RELOAD_BTN) {
            navigate_impl(g.current_url, 0);
        }
        if (id == IDC_BACK_BTN) {
            if (g.history_pos > 0) {
                g.history_pos--;
                navigate_impl(g.history[g.history_pos], 0);
                SetWindowTextA(g.url_entry, g.current_url);
                update_nav_buttons();
            }
        }
        if (id == IDC_FWD_BTN) {
            if (g.history_pos < g.history_count - 1) {
                g.history_pos++;
                navigate_impl(g.history[g.history_pos], 0);
                SetWindowTextA(g.url_entry, g.current_url);
                update_nav_buttons();
            }
        }
        return 0;
    }

    case WM_DESTROY:
        if (g.has_content) pane_result_free(&g.result);
        free_history();
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

/* ── Entry point ───────────────────────────────────────────────────── */

int win32_browser_run(int argc, char **argv)
{
    HINSTANCE hInst = GetModuleHandle(NULL);

    memset(&g, 0, sizeof(g));
    g.viewport_w = 1024;
    g.viewport_h = 700;
    g.history_pos = -1;

    /* Load cursors. */
    g.cursor_arrow = LoadCursor(NULL, IDC_ARROW);
    g.cursor_hand  = LoadCursor(NULL, IDC_HAND);
    g.cursor_ibeam = LoadCursor(NULL, IDC_IBEAM);
    g.current_cursor = g.cursor_arrow;

    /* Register GDI text measurement so layout gets accurate widths. */
    layout_set_measure_fn(win32_measure_text_cb);

    /* Register window class. */
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = NULL;  /* We handle cursor in WM_SETCURSOR */
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"PaneBrowser";
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassExW(&wc);

    g.hwnd = CreateWindowExW(
        0, L"PaneBrowser", L"Pane Browser",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1100, 780,
        NULL, NULL, hInst, NULL);

    ShowWindow(g.hwnd, SW_SHOW);
    UpdateWindow(g.hwnd);

    /* Navigate to argument or home. */
    if (argc > 1) {
        SetWindowTextA(g.url_entry, argv[1]);
        navigate(argv[1]);
    }

    /* Message loop. */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        /* Handle Enter in URL bar. */
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN &&
            msg.hwnd == g.url_entry) {
            char url[MAX_URL];
            GetWindowTextA(g.url_entry, url, MAX_URL);
            navigate(url);
            SetFocus(g.hwnd);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

#endif /* _WIN32 */
