/*
 * Pane — HTTP Client (Win32 WinHTTP Implementation)
 *
 * Uses the native WinHTTP API for HTTP/HTTPS requests.
 * No OpenSSL dependency needed on Windows.
 */

#ifdef _WIN32

#include "http.h"

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "winhttp.lib")

/* ── URL parsing ──────────────────────────────────────────────────── */

typedef struct {
    int     is_https;
    wchar_t host[256];
    WORD    port;
    wchar_t path[2048];
} ParsedUrlW;

static int parse_url_w(const char *url, ParsedUrlW *out)
{
    /* Convert to wide string. */
    wchar_t wurl[4096];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 4096);

    URL_COMPONENTS uc = {0};
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = out->host;
    uc.dwHostNameLength = 256;
    uc.lpszUrlPath = out->path;
    uc.dwUrlPathLength = 2048;

    if (!WinHttpCrackUrl(wurl, 0, 0, &uc))
        return -1;

    out->is_https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
    out->port = uc.nPort;
    if (out->path[0] == L'\0') wcscpy(out->path, L"/");

    return 0;
}

/* ── Growable buffer ──────────────────────────────────────────────── */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} Buffer;

static void buf_init(Buffer *b) { b->data = NULL; b->len = 0; b->cap = 0; }

static void buf_append(Buffer *b, const char *data, size_t len)
{
    if (b->len + len > b->cap) {
        size_t new_cap = b->cap ? b->cap * 2 : 4096;
        while (new_cap < b->len + len) new_cap *= 2;
        b->data = (char *)realloc(b->data, new_cap);
        b->cap = new_cap;
    }
    memcpy(b->data + b->len, data, len);
    b->len += len;
}

/* ── Public API ───────────────────────────────────────────────────── */

static HttpResponse *make_error(const char *msg)
{
    HttpResponse *r = (HttpResponse *)calloc(1, sizeof(HttpResponse));
    r->error = _strdup(msg);
    return r;
}

HttpResponse *http_get(const char *url, int max_redirects)
{
    ParsedUrlW parsed;
    if (parse_url_w(url, &parsed) != 0)
        return make_error("Invalid URL");

    HINTERNET hSession = WinHttpOpen(L"Pane/0.1",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession)
        return make_error("WinHTTP session failed");

    HINTERNET hConnect = WinHttpConnect(hSession, parsed.host, parsed.port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return make_error("Connection failed");
    }

    DWORD flags = parsed.is_https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", parsed.path,
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return make_error("Request creation failed");
    }

    /* Set redirect policy. */
    DWORD opt = max_redirects > 0
        ? WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS
        : WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &opt, sizeof(opt));

    /* Set timeouts. */
    DWORD timeout = 15000;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return make_error("Send request failed");
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return make_error("No response received");
    }

    /* Get status code. */
    DWORD status_code = 0;
    DWORD sc_size = sizeof(status_code);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status_code, &sc_size,
                        WINHTTP_NO_HEADER_INDEX);

    /* Get content type. */
    wchar_t ct_buf[256] = {0};
    DWORD ct_size = sizeof(ct_buf);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_TYPE,
                        WINHTTP_HEADER_NAME_BY_INDEX, ct_buf, &ct_size,
                        WINHTTP_NO_HEADER_INDEX);

    /* Read body. */
    Buffer body;
    buf_init(&body);

    DWORD bytes_available;
    while (WinHttpQueryDataAvailable(hRequest, &bytes_available) && bytes_available > 0) {
        char *chunk = (char *)malloc(bytes_available);
        DWORD bytes_read = 0;
        WinHttpReadData(hRequest, chunk, bytes_available, &bytes_read);
        if (bytes_read > 0)
            buf_append(&body, chunk, bytes_read);
        free(chunk);

        if (body.len > 16 * 1024 * 1024) break;  /* 16 MB limit. */
    }

    /* Build response. */
    HttpResponse *resp = (HttpResponse *)calloc(1, sizeof(HttpResponse));
    resp->status_code = (int)status_code;
    resp->body = body.data;
    resp->body_len = body.len;

    /* Null-terminate body. */
    if (resp->body) {
        resp->body = (char *)realloc(resp->body, body.len + 1);
        resp->body[body.len] = '\0';
    }

    /* Convert content type to UTF-8. */
    if (ct_buf[0]) {
        int ct_len = WideCharToMultiByte(CP_UTF8, 0, ct_buf, -1, NULL, 0, NULL, NULL);
        resp->content_type = (char *)malloc(ct_len);
        WideCharToMultiByte(CP_UTF8, 0, ct_buf, -1, resp->content_type, ct_len, NULL, NULL);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return resp;
}

void http_response_free(HttpResponse *resp)
{
    if (!resp) return;
    free(resp->body);
    free(resp->content_type);
    free(resp->location);
    free(resp->error);
    free(resp);
}

#endif /* _WIN32 */
