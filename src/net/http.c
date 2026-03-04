/*
 * Pane — HTTP Client Implementation
 *
 * Uses POSIX sockets for HTTP and OpenSSL for HTTPS.
 * Supports: GET, redirects, chunked transfer encoding, content-length.
 */

#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

/* ── URL parsing ──────────────────────────────────────────────────── */

typedef struct {
    int   is_https;
    char  host[256];
    char  port[8];
    char  path[2048];
} ParsedUrl;

static int parse_url(const char *url, ParsedUrl *out)
{
    memset(out, 0, sizeof(*out));

    const char *p = url;

    if (strncmp(p, "https://", 8) == 0) {
        out->is_https = 1;
        strcpy(out->port, "443");
        p += 8;
    } else if (strncmp(p, "http://", 7) == 0) {
        out->is_https = 0;
        strcpy(out->port, "80");
        p += 7;
    } else {
        /* Assume https if no scheme. */
        out->is_https = 1;
        strcpy(out->port, "443");
    }

    /* Extract host. */
    const char *slash = strchr(p, '/');
    const char *colon = strchr(p, ':');

    size_t host_len;
    if (colon && (!slash || colon < slash)) {
        /* Host:port. */
        host_len = colon - p;
        if (host_len >= sizeof(out->host)) return -1;
        memcpy(out->host, p, host_len);
        out->host[host_len] = '\0';

        const char *port_start = colon + 1;
        const char *port_end = slash ? slash : port_start + strlen(port_start);
        size_t port_len = port_end - port_start;
        if (port_len >= sizeof(out->port)) return -1;
        memcpy(out->port, port_start, port_len);
        out->port[port_len] = '\0';
    } else {
        host_len = slash ? (size_t)(slash - p) : strlen(p);
        if (host_len >= sizeof(out->host)) return -1;
        memcpy(out->host, p, host_len);
        out->host[host_len] = '\0';
    }

    /* Path. */
    if (slash) {
        strncpy(out->path, slash, sizeof(out->path) - 1);
    } else {
        strcpy(out->path, "/");
    }

    return 0;
}

/* ── Socket I/O wrapper (plain or TLS) ────────────────────────────── */

typedef struct {
    int      fd;
    SSL     *ssl;
    SSL_CTX *ctx;
} Connection;

static int conn_write(Connection *c, const void *buf, size_t len)
{
    if (c->ssl)
        return SSL_write(c->ssl, buf, (int)len);
    return (int)send(c->fd, buf, len, 0);
}

static int conn_read(Connection *c, void *buf, size_t len)
{
    if (c->ssl)
        return SSL_read(c->ssl, buf, (int)len);
    return (int)recv(c->fd, buf, len, 0);
}

static void conn_close(Connection *c)
{
    if (c->ssl) {
        SSL_shutdown(c->ssl);
        SSL_free(c->ssl);
    }
    if (c->ctx) SSL_CTX_free(c->ctx);
    if (c->fd >= 0) close(c->fd);
    c->fd = -1;
    c->ssl = NULL;
    c->ctx = NULL;
}

/* ── Connect to host ──────────────────────────────────────────────── */

static int tcp_connect(const char *host, const char *port)
{
    struct addrinfo hints = {0}, *res, *rp;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port, &hints, &res);
    if (err != 0) return -1;

    int fd = -1;
    for (rp = res; rp; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) continue;

        /* Set a connect timeout via SO_SNDTIMEO. */
        struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0)
            break;

        close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

static int conn_open(Connection *c, const ParsedUrl *url)
{
    memset(c, 0, sizeof(*c));
    c->fd = -1;

    c->fd = tcp_connect(url->host, url->port);
    if (c->fd < 0) return -1;

    if (url->is_https) {
        c->ctx = SSL_CTX_new(TLS_client_method());
        if (!c->ctx) { close(c->fd); c->fd = -1; return -1; }

        SSL_CTX_set_default_verify_paths(c->ctx);

        c->ssl = SSL_new(c->ctx);
        SSL_set_fd(c->ssl, c->fd);
        SSL_set_tlsext_host_name(c->ssl, url->host);

        if (SSL_connect(c->ssl) <= 0) {
            conn_close(c);
            return -1;
        }
    }

    return 0;
}

/* ── Growable buffer ──────────────────────────────────────────────── */

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} Buffer;

static void buf_init(Buffer *b)
{
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buf_ensure(Buffer *b, size_t extra)
{
    if (b->len + extra > b->cap) {
        size_t new_cap = b->cap ? b->cap * 2 : 4096;
        while (new_cap < b->len + extra) new_cap *= 2;
        b->data = realloc(b->data, new_cap);
        b->cap = new_cap;
    }
}

static void buf_append(Buffer *b, const char *data, size_t len)
{
    buf_ensure(b, len);
    memcpy(b->data + b->len, data, len);
    b->len += len;
}

static void buf_free(Buffer *b)
{
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* ── HTTP response parsing ────────────────────────────────────────── */

static char *header_value(const char *headers, const char *name)
{
    /* Case-insensitive search for "Name: value\r\n". */
    size_t nlen = strlen(name);
    const char *p = headers;

    while (*p) {
        if (strncasecmp(p, name, nlen) == 0 && p[nlen] == ':') {
            const char *val = p + nlen + 1;
            while (*val == ' ' || *val == '\t') val++;
            const char *end = strstr(val, "\r\n");
            if (!end) end = val + strlen(val);
            size_t vlen = end - val;
            char *result = malloc(vlen + 1);
            memcpy(result, val, vlen);
            result[vlen] = '\0';
            return result;
        }
        const char *next = strstr(p, "\r\n");
        if (!next) break;
        p = next + 2;
    }
    return NULL;
}

/* Read chunked transfer encoding body. */
static int read_chunked(Connection *c, const char *leftover, size_t leftover_len,
                        Buffer *body)
{
    Buffer raw;
    buf_init(&raw);
    if (leftover_len > 0)
        buf_append(&raw, leftover, leftover_len);

    /* Read until we get the final "0\r\n\r\n". */
    char tmp[8192];
    for (;;) {
        /* Try to parse a chunk from raw. */
        while (raw.len > 0) {
            /* Find chunk size line. */
            char *crlf = memmem(raw.data, raw.len, "\r\n", 2);
            if (!crlf) break;

            size_t line_len = crlf - raw.data;
            char size_str[32];
            if (line_len >= sizeof(size_str)) { buf_free(&raw); return -1; }
            memcpy(size_str, raw.data, line_len);
            size_str[line_len] = '\0';

            unsigned long chunk_size = strtoul(size_str, NULL, 16);

            /* Need: line + \r\n + chunk_size + \r\n */
            size_t need = line_len + 2 + chunk_size + 2;
            if (raw.len < need) break;  /* need more data */

            if (chunk_size == 0) {
                /* Final chunk. */
                buf_free(&raw);
                return 0;
            }

            buf_append(body, raw.data + line_len + 2, chunk_size);

            /* Remove consumed data from raw. */
            size_t consumed = need;
            memmove(raw.data, raw.data + consumed, raw.len - consumed);
            raw.len -= consumed;
        }

        int n = conn_read(c, tmp, sizeof(tmp));
        if (n <= 0) break;
        buf_append(&raw, tmp, n);

        /* Safety limit: 16 MB. */
        if (raw.len > 16 * 1024 * 1024) break;
    }

    buf_free(&raw);
    return 0;
}

/* ── Main HTTP GET ────────────────────────────────────────────────── */

static HttpResponse *make_error(const char *msg)
{
    HttpResponse *r = calloc(1, sizeof(HttpResponse));
    r->error = strdup(msg);
    return r;
}

static HttpResponse *http_get_one(const ParsedUrl *url)
{
    Connection conn;
    if (conn_open(&conn, url) != 0) {
        return make_error("Connection failed");
    }

    /* Send HTTP request. */
    char request[4096];
    int rlen = snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "User-Agent: Pane/0.1\r\n"
        "Accept: text/html,application/xhtml+xml,*/*\r\n"
        "Accept-Encoding: identity\r\n"
        "Connection: close\r\n"
        "\r\n",
        url->path, url->host);

    if (conn_write(&conn, request, rlen) <= 0) {
        conn_close(&conn);
        return make_error("Failed to send request");
    }

    /* Read response headers. */
    Buffer hdr;
    buf_init(&hdr);

    char tmp[8192];
    char *header_end = NULL;

    while (!header_end) {
        int n = conn_read(&conn, tmp, sizeof(tmp));
        if (n <= 0) break;
        buf_append(&hdr, tmp, n);
        buf_ensure(&hdr, 1);
        hdr.data[hdr.len] = '\0';
        header_end = strstr(hdr.data, "\r\n\r\n");

        if (hdr.len > 64 * 1024) break;  /* Header too large. */
    }

    if (!header_end) {
        buf_free(&hdr);
        conn_close(&conn);
        return make_error("Invalid HTTP response");
    }

    *header_end = '\0';
    char *headers = hdr.data;
    char *body_start = header_end + 4;
    size_t body_avail = hdr.len - (body_start - hdr.data);

    /* Parse status line. */
    HttpResponse *resp = calloc(1, sizeof(HttpResponse));
    if (sscanf(headers, "HTTP/%*d.%*d %d", &resp->status_code) != 1) {
        buf_free(&hdr);
        conn_close(&conn);
        free(resp);
        return make_error("Malformed HTTP response");
    }

    /* Extract headers. */
    resp->content_type = header_value(headers, "Content-Type");
    resp->location = header_value(headers, "Location");

    char *transfer_enc = header_value(headers, "Transfer-Encoding");
    char *content_len_str = header_value(headers, "Content-Length");

    /* Read body. */
    Buffer body;
    buf_init(&body);

    int is_chunked = transfer_enc && strcasestr(transfer_enc, "chunked");

    if (is_chunked) {
        read_chunked(&conn, body_start, body_avail, &body);
    } else {
        long content_len = content_len_str ? atol(content_len_str) : -1;

        /* Append what we already have. */
        if (body_avail > 0)
            buf_append(&body, body_start, body_avail);

        /* Read the rest. */
        if (content_len > 0) {
            while ((long)body.len < content_len) {
                int n = conn_read(&conn, tmp, sizeof(tmp));
                if (n <= 0) break;
                buf_append(&body, tmp, n);
            }
        } else {
            /* Read until connection close. */
            for (;;) {
                int n = conn_read(&conn, tmp, sizeof(tmp));
                if (n <= 0) break;
                buf_append(&body, tmp, n);
                if (body.len > 16 * 1024 * 1024) break;  /* 16 MB limit. */
            }
        }
    }

    free(transfer_enc);
    free(content_len_str);

    /* Null-terminate body. */
    buf_ensure(&body, 1);
    body.data[body.len] = '\0';

    resp->body = body.data;
    resp->body_len = body.len;

    buf_free(&hdr);
    conn_close(&conn);
    return resp;
}

/* ── Public API ───────────────────────────────────────────────────── */

HttpResponse *http_get(const char *url, int max_redirects)
{
    char current_url[4096];
    strncpy(current_url, url, sizeof(current_url) - 1);
    current_url[sizeof(current_url) - 1] = '\0';

    for (int redirects = 0; redirects <= max_redirects; redirects++) {
        ParsedUrl parsed;
        if (parse_url(current_url, &parsed) != 0) {
            return make_error("Invalid URL");
        }

        HttpResponse *resp = http_get_one(&parsed);
        if (resp->error) return resp;

        /* Handle redirects. */
        if (resp->status_code >= 300 && resp->status_code < 400 && resp->location) {
            /* Build redirect URL. */
            if (resp->location[0] == '/') {
                /* Relative redirect. */
                snprintf(current_url, sizeof(current_url), "%s://%s%s",
                    parsed.is_https ? "https" : "http",
                    parsed.host, resp->location);
            } else if (strncmp(resp->location, "http", 4) == 0) {
                strncpy(current_url, resp->location, sizeof(current_url) - 1);
            } else {
                /* Relative to current path. */
                snprintf(current_url, sizeof(current_url), "%s://%s/%s",
                    parsed.is_https ? "https" : "http",
                    parsed.host, resp->location);
            }
            http_response_free(resp);
            continue;
        }

        return resp;
    }

    return make_error("Too many redirects");
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
