#include "cache.h"
#include "database.h"
#include "../core/log.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

int cache_get(const char *url, unsigned char **data, int *size,
              char **etag, time_t *last_modified) {
    if (!url || !data || !size) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "SELECT data, etag, last_modified, expires_at FROM api_cache WHERE url = ?1"
    );
    if (!stmt) return -1;

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        // Check if expired
        time_t expires_at = (time_t)sqlite3_column_int64(stmt, 3);
        if (expires_at > 0 && time(NULL) > expires_at) {
            db_finalize(stmt);
            log_debug("Cache expired for: %s", url);
            return -1;
        }

        // Extract blob
        const void *blob = sqlite3_column_blob(stmt, 0);
        int blob_size = sqlite3_column_bytes(stmt, 0);

        if (blob && blob_size > 0) {
            *data = malloc(blob_size);
            memcpy(*data, blob, blob_size);
            *size = blob_size;

            if (etag) {
                const unsigned char *etag_text = sqlite3_column_text(stmt, 1);
                if (etag_text) {
                    *etag = strdup((const char *)etag_text);
                }
            }

            if (last_modified) {
                *last_modified = (time_t)sqlite3_column_int64(stmt, 2);
            }

            db_finalize(stmt);
            log_debug("Cache hit for: %s (%d bytes)", url, blob_size);
            return 0;
        }
    }

    db_finalize(stmt);
    log_debug("Cache miss for: %s", url);
    return -1;
}

int cache_set(const char *url, const unsigned char *data, int size,
              const char *etag, time_t last_modified, time_t expires_at) {
    if (!url || !data || size <= 0) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "INSERT OR REPLACE INTO api_cache "
        "(url, data, etag, last_modified, fetched_at, expires_at) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6)"
    );
    if (!stmt) return -1;

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, data, size, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, etag ? etag : "", -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 4, last_modified);
    sqlite3_bind_int64(stmt, 5, time(NULL));
    sqlite3_bind_int64(stmt, 6, expires_at);

    int rc = sqlite3_step(stmt);
    db_finalize(stmt);

    if (rc == SQLITE_DONE) {
        log_debug("Cached response for: %s (%d bytes, expires: %ld)", url, size, expires_at);
        return 0;
    }

    log_error("Failed to cache: %s", url);
    return -1;
}

int cache_is_valid(const char *url, time_t max_age) {
    if (!url) return 0;

    sqlite3_stmt *stmt = db_prepare(
        "SELECT fetched_at, expires_at FROM api_cache WHERE url = ?1"
    );
    if (!stmt) return 0;

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);

    int is_valid = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        time_t fetched_at = (time_t)sqlite3_column_int64(stmt, 0);
        time_t expires_at = (time_t)sqlite3_column_int64(stmt, 1);
        time_t now = time(NULL);

        // Check absolute expiration
        if (expires_at > 0 && now > expires_at) {
            is_valid = 0;
        }
        // Check age-based expiration
        else if (max_age > 0 && (now - fetched_at) > max_age) {
            is_valid = 0;
        } else {
            is_valid = 1;
        }
    }

    db_finalize(stmt);
    return is_valid;
}

int cache_cleanup(time_t older_than) {
    sqlite3_stmt *stmt = db_prepare(
        "DELETE FROM api_cache WHERE fetched_at < ?1"
    );
    if (!stmt) return -1;

    sqlite3_bind_int64(stmt, 1, older_than);

    int rc = sqlite3_step(stmt);
    db_finalize(stmt);

    if (rc == SQLITE_DONE) {
        log_info("Cache cleanup completed");
        return 0;
    }

    return -1;
}

int cache_invalidate(const char *url) {
    if (!url) return -1;

    sqlite3_stmt *stmt = db_prepare("DELETE FROM api_cache WHERE url = ?1");
    if (!stmt) return -1;

    sqlite3_bind_text(stmt, 1, url, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    db_finalize(stmt);

    if (rc == SQLITE_DONE) {
        log_info("Invalidated cache for: %s", url);
        return 0;
    }

    return -1;
}
