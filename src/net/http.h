/*
 * Pane — HTTP Client
 *
 * Simple HTTP/HTTPS client using POSIX sockets + OpenSSL.
 * Supports GET requests with redirects and chunked transfer encoding.
 */

#ifndef PANE_NET_HTTP_H
#define PANE_NET_HTTP_H

#include <stddef.h>

/* HTTP response. Caller must free body with http_response_free(). */
typedef struct {
    int     status_code;
    char   *body;
    size_t  body_len;
    char   *content_type;   /* e.g. "text/html; charset=utf-8" */
    char   *location;       /* redirect target (3xx) */
    char   *error;          /* error message on failure */
} HttpResponse;

/* Fetch a URL via HTTP GET. Follows up to max_redirects redirects.
 * Returns a heap-allocated HttpResponse. */
HttpResponse *http_get(const char *url, int max_redirects);

/* Free an HttpResponse. */
void http_response_free(HttpResponse *resp);

#endif /* PANE_NET_HTTP_H */
