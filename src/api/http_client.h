#ifndef HAMCLOCK_HTTP_CLIENT_H
#define HAMCLOCK_HTTP_CLIENT_H

#include <time.h>

typedef struct {
    unsigned char *data;
    int size;
    int http_status;
    char *etag;
    time_t last_modified;
} http_response_t;

// HTTP request with caching support
int http_get(const char *url, http_response_t *response, time_t cache_ttl);

// HTTP request with If-None-Match (ETag)
int http_get_conditional(const char *url, const char *etag,
                         time_t last_modified, http_response_t *response);

// Free response data
void http_response_free(http_response_t *response);

// Set timeout (in seconds)
void http_set_timeout(int seconds);

// Initialize HTTP client
int http_init(void);
void http_deinit(void);

#endif // HAMCLOCK_HTTP_CLIENT_H
