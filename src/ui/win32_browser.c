/*
 * Pane — Win32 Browser UI Implementation
 *
 * Native Windows browser chrome using Win32 API.
 * Uses GDI for 2D rendering with FreeType for text.
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
#include "../pane.h"
#include "../font/font.h"
#include "../net/http.h"
#include "../style/computed.h"
#include "../layout/box.h"
#include "../html/tree_builder.h"

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

/* ── State ─────────────────────────────────────────────────────────── */

static struct {
    HWND     hwnd;
    HWND     url_entry;
    HWND     back_btn;
    HWND     fwd_btn;
    HWND     reload_btn;
    HWND     home_btn;
    HWND     go_btn;
    HWND     statusbar;

    PaneResult result;
    int        has_content;

    float    scroll_y;
    float    content_height;
    float    viewport_w;
    float    viewport_h;

    char     current_url[MAX_URL];
    PaneFont *font_regular;
} g;

/* ── Home page ─────────────────────────────────────────────────────── */

static const char *HOME_HTML =
    "<!DOCTYPE html>\n"
    "<html><head><title>Pane Browser</title>\n"
    "<style>\n"
    "  body { font-family: sans-serif; margin: 40px; color: #333;\n"
    "         background-color: #f5f5f5; }\n"
    "  h1 { font-size: 36px; color: #1a1a2e; margin-bottom: 8px; }\n"
    "  .sub { font-size: 14px; color: #888; margin-bottom: 32px; }\n"
    "  .card { background-color: #fff; border: 1px solid #ddd;\n"
    "          padding: 20px; margin-bottom: 16px; }\n"
    "  .card h2 { font-size: 18px; color: #16213e; margin-bottom: 8px; }\n"
    "  .card p { font-size: 14px; color: #555; }\n"
    "  .footer { margin-top: 32px; font-size: 12px; color: #aaa; }\n"
    "</style></head><body>\n"
    "  <h1>Pane</h1>\n"
    "  <div class=\"sub\">A web browser built from scratch in C17</div>\n"
    "  <div class=\"card\">\n"
    "    <h2>Welcome</h2>\n"
    "    <p>Type a file path or HTML in the address bar. The engine uses\n"
    "       a pairwise context transition model for rendering.</p>\n"
    "  </div>\n"
    "  <div class=\"card\">\n"
    "    <h2>Engine Stats</h2>\n"
    "    <p>412 CSS properties, 130 HTML tags, 21 context fields,\n"
    "       93 compressed rules, FreeType font rendering.</p>\n"
    "  </div>\n"
    "  <div class=\"footer\">Pane v0.1.0 — Win32 Native Build</div>\n"
    "</body></html>\n";

/* ── Load page ─────────────────────────────────────────────────────── */

static void load_page(const char *html, size_t len)
{
    if (g.has_content) {
        pane_result_free(&g.result);
        g.has_content = 0;
    }

    g.result = pane_render(html, len, NULL, 0, g.viewport_w, g.viewport_h);
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

static void navigate(const char *url)
{
    strncpy(g.current_url, url, MAX_URL - 1);

    if (strcmp(url, "about:home") == 0 || url[0] == '\0') {
        load_page(HOME_HTML, strlen(HOME_HTML));
        return;
    }

    /* Inline HTML. */
    if (url[0] == '<') {
        load_page(url, strlen(url));
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
            return;
        }

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        char *buf = (char *)malloc(sz + 1);
        if (buf) {
            fread(buf, 1, sz, f);
            buf[sz] = '\0';
            load_page(buf, sz);
            free(buf);
        }
        fclose(f);
        return;
    }

    /* HTTP / HTTPS URLs. */
    if (strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0) {
        HttpResponse *resp = http_get(url, 5);

        if (resp->error) {
            show_error(url, resp->error);
        } else if (resp->body && resp->body_len > 0) {
            load_page(resp->body, resp->body_len);
        } else {
            show_error(url, "Empty response.");
        }

        http_response_free(resp);
        return;
    }

    /* Bare domain name — try as https:// */
    if (strchr(url, '.') && url[0] != '<' && url[0] != '/') {
        char full_url[2048];
        snprintf(full_url, sizeof(full_url), "https://%s", url);
        navigate(full_url);
        return;
    }

    show_error(url, "Unsupported URL scheme.");
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
        int font_height = -(int)(s->font_size * 96.0f / 72.0f);
        int weight = s->font_weight >= 600 ? FW_BOLD : FW_NORMAL;

        HFONT hfont = CreateFontA(
            font_height, 0, 0, 0, weight,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS,
            s->font_family ? s->font_family : "Segoe UI");

        HFONT old_font = (HFONT)SelectObject(hdc, hfont);
        SetTextColor(hdc, RGB(s->color.r, s->color.g, s->color.b));
        SetBkMode(hdc, TRANSPARENT);

        RECT text_rc = { (int)x, (int)y,
                         (int)(x + box->rect.width + 200),
                         (int)(y + box->rect.height + 200) };

        /* Convert UTF-8 to wide string. */
        int wlen = MultiByteToWideChar(CP_UTF8, 0, box->text, (int)box->text_len, NULL, 0);
        wchar_t *wtext = (wchar_t *)malloc((wlen + 1) * sizeof(wchar_t));
        if (wtext) {
            MultiByteToWideChar(CP_UTF8, 0, box->text, (int)box->text_len, wtext, wlen);
            wtext[wlen] = L'\0';
            DrawTextW(hdc, wtext, wlen, &text_rc, DT_LEFT | DT_TOP | DT_WORDBREAK);
            free(wtext);
        }

        SelectObject(hdc, old_font);
        DeleteObject(hfont);
    }

    /* Children. */
    for (LayoutBox *child = box->first_child; child; child = child->next_sibling) {
        paint_box_gdi(hdc, child, x, oy);
    }
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

        /* Load home page. */
        navigate("about:home");
        return 0;
    }

    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        g.viewport_w = (float)w;
        g.viewport_h = (float)(h - TOOLBAR_HEIGHT);

        /* Resize URL entry and go button. */
        int url_w = w - 210;
        MoveWindow(g.url_entry, 144, 6, url_w > 100 ? url_w : 100, 24, TRUE);
        MoveWindow(g.go_btn, w - 60, 4, 52, 28, TRUE);

        InvalidateRect(hwnd, NULL, TRUE);
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

        /* Content area. */
        RECT content_rc = { 0, TOOLBAR_HEIGHT, rc.right, rc.bottom };
        HBRUSH white = CreateSolidBrush(RGB(255, 255, 255));
        FillRect(hdc, &content_rc, white);
        DeleteObject(white);

        /* Render page. */
        if (g.has_content && g.result.layout_tree && g.result.layout_tree->root) {
            /* Clip to content area. */
            HRGN clip = CreateRectRgn(0, TOOLBAR_HEIGHT, rc.right, rc.bottom);
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

    case WM_KEYDOWN: {
        if (wParam == VK_F5) {
            navigate(g.current_url);
            return 0;
        }
        if (wParam == VK_RETURN && GetFocus() == g.url_entry) {
            char url[MAX_URL];
            GetWindowTextA(g.url_entry, url, MAX_URL);
            navigate(url);
            SetFocus(hwnd);
            return 0;
        }
        /* Scroll keys. */
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
        int code = HIWORD(wParam);

        if (id == IDC_GO_BTN || (id == IDC_URL_ENTRY && code == EN_KILLFOCUS + 1)) {
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
            navigate(g.current_url);
        }
        return 0;
    }

    case WM_DESTROY:
        if (g.has_content) pane_result_free(&g.result);
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

    /* Register window class. */
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
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
