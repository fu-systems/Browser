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
#define MAX_FORM_VALUES 64
#define IDC_FORM_EDIT  2001

/* ── Form value storage ────────────────────────────────────────────── */

typedef struct {
    const DomNode *node;
    char          *value;
} FormFieldValue;

/* ── Text selection state ──────────────────────────────────────────── */

typedef struct {
    int            active;     /* currently dragging */
    int            has_sel;    /* selection exists */
    float          start_x;   /* page coordinates */
    float          start_y;
    float          end_x;
    float          end_y;
    /* Collected selected text (built after mouse-up). */
    char          *text;
    size_t         text_len;
} TextSelection;

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

    /* Form input overlay. */
    HWND              form_edit;       /* active EDIT control (or NULL) */
    const DomNode    *form_edit_node;  /* DOM node being edited */
    WNDPROC           form_edit_orig_proc;  /* original EDIT wndproc */

    /* Form value storage. */
    FormFieldValue    form_values[MAX_FORM_VALUES];
    int               form_value_count;

    /* Text selection. */
    TextSelection     sel;
} g;

/* ── Forward declarations ──────────────────────────────────────────── */

static void navigate_impl(const char *url, int push_history);
static void navigate(const char *url);
static void update_nav_buttons(void);
static void set_status(const char *text);
static void handle_form_submit(const DomNode *form_node);
static void dismiss_form_edit(void);
static void spawn_form_edit(const DomNode *node, const LayoutBox *box);
static void clear_selection(void);
static void collect_selected_text(void);
static void copy_selection_to_clipboard(void);

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

/* ── Form value management ─────────────────────────────────────────── */

static void clear_form_values(void)
{
    for (int i = 0; i < g.form_value_count; i++)
        free(g.form_values[i].value);
    g.form_value_count = 0;
}

static const char *get_form_value(const DomNode *node)
{
    for (int i = 0; i < g.form_value_count; i++) {
        if (g.form_values[i].node == node)
            return g.form_values[i].value;
    }
    return NULL;
}

static void set_form_value(const DomNode *node, const char *value)
{
    for (int i = 0; i < g.form_value_count; i++) {
        if (g.form_values[i].node == node) {
            free(g.form_values[i].value);
            g.form_values[i].value = _strdup(value);
            return;
        }
    }
    if (g.form_value_count < MAX_FORM_VALUES) {
        g.form_values[g.form_value_count].node = node;
        g.form_values[g.form_value_count].value = _strdup(value);
        g.form_value_count++;
    }
}

/* ── Form edit overlay ─────────────────────────────────────────────── */

static LRESULT CALLBACK FormEditProc(HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
    if (msg == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            /* Dismiss without saving. */
            HWND edit = g.form_edit;
            g.form_edit = NULL;
            g.form_edit_node = NULL;
            DestroyWindow(edit);
            SetFocus(g.hwnd);
            return 0;
        }
        if (wParam == VK_RETURN) {
            /* Save value, then try to submit parent form. */
            if (g.form_edit && g.form_edit_node) {
                char buf[4096];
                GetWindowTextA(g.form_edit, buf, sizeof(buf));
                set_form_value(g.form_edit_node, buf);

                const DomNode *form = find_form_parent(g.form_edit_node);
                dismiss_form_edit();
                if (form)
                    handle_form_submit(form);
            } else {
                dismiss_form_edit();
            }
            return 0;
        }
    }
    if (msg == WM_KILLFOCUS) {
        /* Save and dismiss when focus leaves. */
        if (g.form_edit && g.form_edit_node) {
            char buf[4096];
            GetWindowTextA(g.form_edit, buf, sizeof(buf));
            set_form_value(g.form_edit_node, buf);
        }
        /* Post a message to dismiss asynchronously (avoid destroying
         * the window while processing its own message). */
        PostMessage(g.hwnd, WM_APP + 1, 0, 0);
    }
    return CallWindowProcW(g.form_edit_orig_proc, hwnd, msg, wParam, lParam);
}

static void dismiss_form_edit(void)
{
    if (!g.form_edit) return;

    /* Save current value before destroying. */
    if (g.form_edit_node) {
        char buf[4096];
        GetWindowTextA(g.form_edit, buf, sizeof(buf));
        set_form_value(g.form_edit_node, buf);
    }

    HWND edit = g.form_edit;
    g.form_edit = NULL;
    g.form_edit_node = NULL;
    g.form_edit_orig_proc = NULL;
    DestroyWindow(edit);
    SetFocus(g.hwnd);
}

static void spawn_form_edit(const DomNode *node, const LayoutBox *box)
{
    dismiss_form_edit();

    /* Compute absolute position of the box. */
    float abs_x, abs_y;
    layout_box_abs_position(box, &abs_x, &abs_y);

    /* Convert to screen coordinates (subtract scroll, add toolbar). */
    int sx = (int)abs_x;
    int sy = (int)(abs_y - g.scroll_y) + TOOLBAR_HEIGHT;
    int sw = (int)(box->rect.width + box->padding.left + box->padding.right);
    int sh = (int)(box->rect.height + box->padding.top + box->padding.bottom);

    if (sw < 40) sw = 40;
    if (sh < 20) sh = 20;

    /* Get initial value. */
    const char *initial = get_form_value(node);
    if (!initial) {
        initial = elem_get_attr(node, "value");
        if (!initial && node->elem.tag == TAG_TEXTAREA) {
            DomNode *tc = node->first_child;
            if (tc && tc->type == PANE_NODE_TEXT && tc->text.data)
                initial = tc->text.data;
        }
    }
    if (!initial) initial = "";

    /* Determine if password. */
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL;
    const char *type = elem_get_attr(node, "type");
    if (type && strcmp(type, "password") == 0)
        style |= ES_PASSWORD;

    /* For textarea, use multiline. */
    if (node->elem.tag == TAG_TEXTAREA)
        style = (style & ~ES_AUTOHSCROLL) | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN;

    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(g.hwnd, GWLP_HINSTANCE);
    g.form_edit = CreateWindowA("EDIT", initial, style,
                                 sx, sy, sw, sh,
                                 g.hwnd, (HMENU)IDC_FORM_EDIT, hInst, NULL);

    /* Set font. */
    HFONT ui_font = CreateFontA(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    SendMessage(g.form_edit, WM_SETFONT, (WPARAM)ui_font, TRUE);

    /* Subclass to intercept Enter/Escape. */
    g.form_edit_orig_proc = (WNDPROC)SetWindowLongPtrW(g.form_edit,
        GWLP_WNDPROC, (LONG_PTR)FormEditProc);

    g.form_edit_node = node;
    SetFocus(g.form_edit);

    /* Select all text. */
    SendMessage(g.form_edit, EM_SETSEL, 0, -1);
}

/* ── Text selection helpers ────────────────────────────────────────── */

static void clear_selection(void)
{
    g.sel.active = 0;
    g.sel.has_sel = 0;
    free(g.sel.text);
    g.sel.text = NULL;
    g.sel.text_len = 0;
}

/* Check if a text box overlaps the selection rectangle. */
static int box_in_selection(const LayoutBox *box, float abs_x, float abs_y,
                             float sel_top, float sel_bot,
                             float sel_left, float sel_right)
{
    float bx = abs_x;
    float by = abs_y;
    float bx2 = bx + box->rect.width;
    float by2 = by + box->rect.height;

    /* Box must overlap vertical range. */
    if (by2 < sel_top || by > sel_bot) return 0;

    /* For single-line selection (start_y ~ end_y), check horizontal too. */
    if (sel_bot - sel_top < 30) {
        if (bx2 < sel_left || bx > sel_right) return 0;
    }

    return 1;
}

/* Recursively collect text from boxes within the selection region. */
static void collect_text_recursive(const LayoutBox *box, float ox, float oy,
                                    float sel_top, float sel_bot,
                                    float sel_left, float sel_right,
                                    char **buf, size_t *len, size_t *cap)
{
    if (!box) return;
    if (box->style && box->style->display == DISPLAY_NONE) return;

    float x = ox + box->rect.x;
    float y = oy + box->rect.y;

    if (box->type == BOX_TEXT && box->text && box->text_len > 0) {
        if (box_in_selection(box, x, y, sel_top, sel_bot, sel_left, sel_right)) {
            /* Append text. */
            size_t needed = *len + box->text_len + 2;
            if (needed > *cap) {
                while (needed > *cap) *cap *= 2;
                *buf = realloc(*buf, *cap);
            }
            if (*len > 0 && (*buf)[*len - 1] != ' ' && (*buf)[*len - 1] != '\n')
                (*buf)[(*len)++] = ' ';
            memcpy(*buf + *len, box->text, box->text_len);
            *len += box->text_len;
            (*buf)[*len] = '\0';
        }
    }

    for (LayoutBox *child = box->first_child; child; child = child->next_sibling)
        collect_text_recursive(child, x, y, sel_top, sel_bot,
                                sel_left, sel_right, buf, len, cap);
}

static void collect_selected_text(void)
{
    free(g.sel.text);
    g.sel.text = NULL;
    g.sel.text_len = 0;

    if (!g.has_content || !g.result.layout_tree || !g.result.layout_tree->root)
        return;

    float top = g.sel.start_y < g.sel.end_y ? g.sel.start_y : g.sel.end_y;
    float bot = g.sel.start_y > g.sel.end_y ? g.sel.start_y : g.sel.end_y;
    float left = g.sel.start_x < g.sel.end_x ? g.sel.start_x : g.sel.end_x;
    float right = g.sel.start_x > g.sel.end_x ? g.sel.start_x : g.sel.end_x;

    /* Expand vertical range to cover line heights. */
    bot += 20;

    size_t cap = 1024;
    size_t len = 0;
    char *buf = malloc(cap);
    buf[0] = '\0';

    collect_text_recursive(g.result.layout_tree->root, 0, 0,
                            top, bot, left, right, &buf, &len, &cap);

    g.sel.text = buf;
    g.sel.text_len = len;
}

static void copy_selection_to_clipboard(void)
{
    if (!g.sel.text || g.sel.text_len == 0) return;

    /* Convert to wide string. */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, g.sel.text,
                                    (int)g.sel.text_len, NULL, 0);
    if (wlen <= 0) return;

    if (!OpenClipboard(g.hwnd)) return;
    EmptyClipboard();

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (wlen + 1) * sizeof(wchar_t));
    if (hMem) {
        wchar_t *dst = (wchar_t *)GlobalLock(hMem);
        MultiByteToWideChar(CP_UTF8, 0, g.sel.text, (int)g.sel.text_len,
                            dst, wlen);
        dst[wlen] = L'\0';
        GlobalUnlock(hMem);
        SetClipboardData(CF_UNICODETEXT, hMem);
    }
    CloseClipboard();
    set_status("Copied to clipboard");
}

/* Paint selection highlight over text boxes in the selection region. */
static void paint_selection(HDC hdc, const LayoutBox *box, float ox, float oy)
{
    if (!box || !g.sel.has_sel) return;
    if (box->style && box->style->display == DISPLAY_NONE) return;

    float x = ox + box->rect.x;
    float y = oy + box->rect.y;

    float top = g.sel.start_y < g.sel.end_y ? g.sel.start_y : g.sel.end_y;
    float bot = g.sel.start_y > g.sel.end_y ? g.sel.start_y : g.sel.end_y;
    float left = g.sel.start_x < g.sel.end_x ? g.sel.start_x : g.sel.end_x;
    float right = g.sel.start_x > g.sel.end_x ? g.sel.start_x : g.sel.end_x;
    bot += 20;

    if (box->type == BOX_TEXT && box->text && box->text_len > 0) {
        float screen_y = y - g.scroll_y;
        if (box_in_selection(box, x, y, top, bot, left, right)) {
            RECT hl;
            hl.left   = (int)x;
            hl.top    = (int)screen_y;
            hl.right  = (int)(x + box->rect.width);
            hl.bottom = (int)(screen_y + box->rect.height);

            HBRUSH sel_brush = CreateSolidBrush(RGB(51, 153, 255));
            FillRect(hdc, &hl, sel_brush);
            DeleteObject(sel_brush);
        }
    }

    for (LayoutBox *child = box->first_child; child; child = child->next_sibling)
        paint_selection(hdc, child, x, y);
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
    dismiss_form_edit();
    clear_selection();
    clear_form_values();

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

        /* Use stored form value if available, otherwise fall back to attr. */
        const char *val = get_form_value(n);
        if (!val) {
            val = elem_get_attr(n, "value");
            if (!val) val = "";
        }

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
    if (!hit) {
        dismiss_form_edit();
        clear_selection();
        return;
    }

    /* Check for form elements first (higher priority than links). */
    const DomNode *form_node = find_form_element(hit);
    if (form_node) {
        if (is_button_element(form_node)) {
            dismiss_form_edit();
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
            const LayoutBox *form_box = find_box_for_node(
                g.result.layout_tree->root, form_node);
            if (form_box) {
                spawn_form_edit(form_node, form_box);
            }
            return;
        }
    }

    /* Dismiss form edit when clicking elsewhere. */
    dismiss_form_edit();

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

            /* Paint selection highlight behind text. */
            if (g.sel.has_sel)
                paint_selection(hdc, g.result.layout_tree->root, 0, 0);

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

        /* Dismiss form edit on scroll (position would be stale). */
        dismiss_form_edit();

        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        /* Only handle clicks in content area (below toolbar, above status). */
        if (my >= TOOLBAR_HEIGHT) {
            /* Convert to page coordinates for selection. */
            float px = (float)mx;
            float py = (float)(my - TOOLBAR_HEIGHT) + g.scroll_y;

            /* Check if clicking on a form element or link. */
            int is_interactive = 0;
            if (g.has_content && g.result.layout_tree && g.result.layout_tree->root) {
                const LayoutBox *hit = hit_test_box(g.result.layout_tree->root,
                                                     px, py, 0, 0);
                if (hit) {
                    const DomNode *form_node = find_form_element(hit);
                    if (form_node && (is_text_input(form_node) || is_button_element(form_node)))
                        is_interactive = 1;
                    if (!is_interactive && find_link_ancestor(hit))
                        is_interactive = 1;
                }
            }

            if (is_interactive) {
                clear_selection();
                handle_content_click(mx, my);
            } else {
                /* Start text selection. */
                clear_selection();
                g.sel.active = 1;
                g.sel.start_x = px;
                g.sel.start_y = py;
                g.sel.end_x = px;
                g.sel.end_y = py;
                SetCapture(hwnd);
                dismiss_form_edit();
            }
        }
        return 0;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lParam);
        int my = GET_Y_LPARAM(lParam);

        /* Update text selection if dragging. */
        if (g.sel.active) {
            g.sel.end_x = (float)mx;
            g.sel.end_y = (float)(my - TOOLBAR_HEIGHT) + g.scroll_y;
            g.sel.has_sel = 1;
            InvalidateRect(hwnd, NULL, TRUE);
        }

        handle_content_mousemove(mx, my);
        SetCursor(g.current_cursor);
        return 0;
    }

    case WM_LBUTTONUP: {
        if (g.sel.active) {
            int mx = GET_X_LPARAM(lParam);
            int my = GET_Y_LPARAM(lParam);
            g.sel.end_x = (float)mx;
            g.sel.end_y = (float)(my - TOOLBAR_HEIGHT) + g.scroll_y;
            g.sel.active = 0;
            ReleaseCapture();

            /* Only keep selection if mouse moved enough. */
            float dx = g.sel.end_x - g.sel.start_x;
            float dy = g.sel.end_y - g.sel.start_y;
            if (dx * dx + dy * dy > 25) {
                g.sel.has_sel = 1;
                collect_selected_text();
                if (g.sel.text_len > 0) {
                    char status[128];
                    snprintf(status, sizeof(status),
                             "Selected %zu chars (Ctrl+C to copy)",
                             g.sel.text_len);
                    set_status(status);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            } else {
                clear_selection();
            }
        }
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

        /* Ctrl+C: Copy selection to clipboard. */
        if (ctrl && wParam == 'C' && g.sel.has_sel && g.sel.text_len > 0) {
            copy_selection_to_clipboard();
            return 0;
        }

        /* Ctrl+A: Select all text on the page. */
        if (ctrl && wParam == 'A' && GetFocus() != g.url_entry && !g.form_edit) {
            if (g.has_content && g.result.layout_tree && g.result.layout_tree->root) {
                g.sel.start_x = 0;
                g.sel.start_y = 0;
                g.sel.end_x = g.viewport_w;
                g.sel.end_y = g.content_height;
                g.sel.has_sel = 1;
                g.sel.active = 0;
                collect_selected_text();
                InvalidateRect(hwnd, NULL, TRUE);
                if (g.sel.text_len > 0) {
                    char status[128];
                    snprintf(status, sizeof(status),
                             "Selected all (%zu chars, Ctrl+C to copy)",
                             g.sel.text_len);
                    set_status(status);
                }
            }
            return 0;
        }

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

    /* Async dismiss of form edit (posted from WM_KILLFOCUS handler). */
    case WM_APP + 1: {
        if (g.form_edit) {
            HWND edit = g.form_edit;
            g.form_edit = NULL;
            g.form_edit_node = NULL;
            g.form_edit_orig_proc = NULL;
            DestroyWindow(edit);
        }
        return 0;
    }

    case WM_DESTROY:
        dismiss_form_edit();
        clear_selection();
        clear_form_values();
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
        /* Let IsDialogMessage handle Tab/Enter for child controls
         * (form edit, URL bar) so keyboard input works properly. */
        if (g.form_edit && IsWindow(g.form_edit) &&
            msg.hwnd == g.form_edit) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

#endif /* _WIN32 */
