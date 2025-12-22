#ifndef HAMCLOCK_CACHE_H
#define HAMCLOCK_CACHE_H

#include <time.h>

// Cache entry for HTTP responses
typedef struct {
    char *url;
    char *etag;
    time_t last_modified;
    unsigned char *data;
    int data_size;
    time_t fetched_at;
    time_t expires_at;
} cache_entry_t;

// Get cached response
int cache_get(const char *url, unsigned char **data, int *size,
              char **etag, time_t *last_modified);

// Store cached response
int cache_set(const char *url, const unsigned char *data, int size,
              const char *etag, time_t last_modified, time_t expires_at);

// Check if cache is still valid
int cache_is_valid(const char *url, time_t max_age);

// Clear old cache entries
int cache_cleanup(time_t older_than);

// Invalidate specific cache entry
int cache_invalidate(const char *url);

#endif // HAMCLOCK_CACHE_H
