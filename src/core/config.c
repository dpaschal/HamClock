#include "config.h"
#include "../data/database.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *DEFAULT_CONFIG[] = {
    "de_lat=35.7796",
    "de_lon=-78.6382",
    "de_grid=EM79",
    "theme=dark",
    "brightness=100",
    "language=en",
    NULL
};

int config_init(void) {
    // Load default config if not present
    for (int i = 0; DEFAULT_CONFIG[i]; i++) {
        char *eq = strchr(DEFAULT_CONFIG[i], '=');
        if (eq) {
            char key[64], value[256];
            int key_len = eq - DEFAULT_CONFIG[i];
            strncpy(key, DEFAULT_CONFIG[i], key_len);
            key[key_len] = 0;
            strcpy(value, eq + 1);

            // Only set if not already present
            char test_value[256];
            if (config_get(key, test_value, sizeof(test_value)) != 0) {
                config_set(key, value);
            }
        }
    }

    log_info("Configuration initialized");
    return 0;
}

int config_get(const char *key, char *value, int max_len) {
    if (!key || !value || max_len <= 0) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "SELECT value FROM config WHERE key = ?1"
    );
    if (!stmt) return -1;

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *result = sqlite3_column_text(stmt, 0);
        if (result) {
            strncpy(value, (const char *)result, max_len - 1);
            value[max_len - 1] = 0;
            db_finalize(stmt);
            return 0;
        }
    }

    db_finalize(stmt);
    return -1;
}

int config_set(const char *key, const char *value) {
    if (!key || !value) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "INSERT OR REPLACE INTO config (key, value) VALUES (?1, ?2)"
    );
    if (!stmt) return -1;

    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    db_finalize(stmt);

    if (rc == SQLITE_DONE) {
        log_debug("Config set: %s = %s", key, value);
        return 0;
    }

    return -1;
}

int config_get_int(const char *key, int *value) {
    if (!key || !value) return -1;

    char buf[32];
    if (config_get(key, buf, sizeof(buf)) == 0) {
        *value = atoi(buf);
        return 0;
    }

    return -1;
}

int config_set_int(const char *key, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    return config_set(key, buf);
}
