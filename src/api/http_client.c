#include "http_client.h"
#include "../core/log.h"
#include "../data/cache.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

static CURL *curl_handle = NULL;
static int http_timeout = 30;  // seconds

// Memory buffer for response
typedef struct {
    unsigned char *data;
    int size;
    int capacity;
} membuf_t;

// Callback for writing response data
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    membuf_t *mem = (membuf_t *)userp;

    unsigned char *ptr = realloc(mem->data, mem->size + realsize + 1);
    if (!ptr) {
        log_error("Not enough memory for HTTP response");
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->data[mem->size] = 0;

    return realsize;
}

int http_init(void) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_handle = curl_easy_init();

    if (!curl_handle) {
        log_error("Failed to initialize curl");
        return -1;
    }

    log_info("HTTP client initialized");
    return 0;
}

void http_deinit(void) {
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
        curl_handle = NULL;
    }
    curl_global_cleanup();
}

void http_set_timeout(int seconds) {
    if (seconds > 0) {
        http_timeout = seconds;
    }
}

int http_get(const char *url, http_response_t *response, time_t cache_ttl) {
    if (!url || !response) return -1;

    // Initialize response
    memset(response, 0, sizeof(*response));

    // Check cache first
    unsigned char *cached_data = NULL;
    int cached_size = 0;
    char *etag = NULL;
    time_t last_modified = 0;

    if (cache_ttl > 0 && cache_get(url, &cached_data, &cached_size, &etag, &last_modified) == 0) {
        log_debug("Using cached response for: %s", url);
        response->data = cached_data;
        response->size = cached_size;
        response->http_status = 200;  // Assume valid cache
        return 0;
    }

    // Try conditional request first (with ETag/Last-Modified)
    if (etag || last_modified > 0) {
        int rc = http_get_conditional(url, etag, last_modified, response);
        if (response->http_status == 304) {
            // Not modified, use cache
            if (cached_data) {
                response->data = cached_data;
                response->size = cached_size;
                response->http_status = 200;
            }
            return 0;
        }
        // If we got a full response, cache it below
    }

    // Full request
    membuf_t membuf = {0};
    struct curl_slist *headers = NULL;

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, (long)http_timeout);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "hamclock/1.0");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&membuf);

    // Add conditional headers if available
    if (etag) {
        char etag_header[256];
        snprintf(etag_header, sizeof(etag_header), "If-None-Match: %s", etag);
        headers = curl_slist_append(headers, etag_header);
    }

    if (last_modified > 0) {
        char date_header[64];
        struct tm *tm_info = gmtime(&last_modified);
        strftime(date_header, sizeof(date_header),
                "If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT", tm_info);
        headers = curl_slist_append(headers, date_header);
    }

    if (headers) {
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    }

    // Perform request
    CURLcode res = curl_easy_perform(curl_handle);
    if (res != CURLE_OK) {
        log_error("HTTP request failed for %s: %s", url, curl_easy_strerror(res));
        if (headers) curl_slist_free_all(headers);
        free(membuf.data);
        return -1;
    }

    // Get response code and headers
    long http_code = 0;
    char *content_type = NULL;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_getinfo(curl_handle, CURLINFO_CONTENT_TYPE, &content_type);

    if (headers) curl_slist_free_all(headers);

    response->http_status = (int)http_code;

    if (http_code == 200) {
        response->data = membuf.data;
        response->size = membuf.size;

        // Extract and store ETag if available
        char *etag_value = NULL;
        curl_easy_getinfo(curl_handle, CURLINFO_EFFECTIVE_URL, &etag_value);

        // Cache the response
        if (cache_ttl > 0) {
            time_t expires_at = time(NULL) + cache_ttl;
            cache_set(url, membuf.data, membuf.size, etag, last_modified, expires_at);
        }

        log_debug("HTTP GET %s: %ld (%d bytes)", url, http_code, membuf.size);
        return 0;
    } else if (http_code == 304) {
        response->http_status = 304;
        free(membuf.data);
        log_debug("HTTP GET %s: 304 Not Modified", url);
        return 0;
    } else {
        log_warn("HTTP GET %s: %ld", url, http_code);
        free(membuf.data);
        return -1;
    }
}

int http_get_conditional(const char *url, const char *etag,
                         time_t last_modified, http_response_t *response) {
    if (!url || !response) return -1;

    memset(response, 0, sizeof(*response));

    membuf_t membuf = {0};
    struct curl_slist *headers = NULL;

    curl_easy_setopt(curl_handle, CURLOPT_URL, url);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, (long)http_timeout);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&membuf);

    // Add conditional headers
    if (etag) {
        char header_buf[256];
        snprintf(header_buf, sizeof(header_buf), "If-None-Match: %s", etag);
        headers = curl_slist_append(headers, header_buf);
    }

    if (last_modified > 0) {
        char header_buf[64];
        struct tm *tm_info = gmtime(&last_modified);
        strftime(header_buf, sizeof(header_buf),
                "If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT", tm_info);
        headers = curl_slist_append(headers, header_buf);
    }

    if (headers) {
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(curl_handle);
    if (headers) curl_slist_free_all(headers);

    if (res != CURLE_OK) {
        log_error("Conditional HTTP request failed: %s", curl_easy_strerror(res));
        free(membuf.data);
        return -1;
    }

    long http_code = 0;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &http_code);
    response->http_status = (int)http_code;

    if (http_code == 304) {
        response->http_status = 304;
        free(membuf.data);
        return 0;  // Not modified
    } else if (http_code == 200) {
        response->data = membuf.data;
        response->size = membuf.size;
        return 0;  // Got fresh data
    } else {
        free(membuf.data);
        return -1;  // Error
    }
}

void http_response_free(http_response_t *response) {
    if (response) {
        if (response->data) {
            free(response->data);
            response->data = NULL;
        }
        if (response->etag) {
            free(response->etag);
            response->etag = NULL;
        }
        response->size = 0;
    }
}
