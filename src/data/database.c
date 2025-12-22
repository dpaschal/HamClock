#include "database.h"
#include "../core/log.h"
#include <stdlib.h>
#include <string.h>

static sqlite3 *g_db = NULL;
static const int CURRENT_SCHEMA_VERSION = 1;

// Database schema
static const char *SCHEMA_SQL[] = {
    // Version 1: Initial schema
    "CREATE TABLE IF NOT EXISTS config ("
    "    key TEXT PRIMARY KEY,"
    "    value TEXT"
    ");"

    "CREATE TABLE IF NOT EXISTS api_cache ("
    "    url TEXT PRIMARY KEY,"
    "    etag TEXT,"
    "    last_modified INTEGER,"
    "    data BLOB,"
    "    fetched_at INTEGER,"
    "    expires_at INTEGER"
    ");"

    "CREATE TABLE IF NOT EXISTS space_weather ("
    "    timestamp INTEGER PRIMARY KEY,"
    "    kp_index REAL,"
    "    a_index REAL,"
    "    solar_flux REAL,"
    "    sunspot_number INTEGER,"
    "    xray_flux TEXT,"
    "    solar_wind_speed REAL"
    ");"

    "CREATE TABLE IF NOT EXISTS dx_spots ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    timestamp INTEGER,"
    "    callsign TEXT,"
    "    frequency REAL,"
    "    spotter TEXT,"
    "    comment TEXT"
    ");"

    "CREATE TABLE IF NOT EXISTS satellite_tles ("
    "    name TEXT PRIMARY KEY,"
    "    line1 TEXT,"
    "    line2 TEXT,"
    "    fetched_at INTEGER"
    ");"
};

sqlite3 *db_get_handle(void) {
    return g_db;
}

int db_init(const char *db_path) {
    int rc = sqlite3_open(db_path, &g_db);
    if (rc != SQLITE_OK) {
        log_error("Failed to open database: %s", sqlite3_errmsg(g_db));
        return -1;
    }

    log_info("Database opened: %s", db_path);

    // Enable foreign keys
    sqlite3_exec(g_db, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL);

    // Check schema version
    int current_version = db_get_schema_version();
    if (current_version < CURRENT_SCHEMA_VERSION) {
        log_info("Creating/upgrading database schema (current: %d, target: %d)",
                 current_version, CURRENT_SCHEMA_VERSION);

        // Create schema for version 1
        for (size_t i = 0; i < sizeof(SCHEMA_SQL) / sizeof(SCHEMA_SQL[0]); i++) {
            if (db_exec(SCHEMA_SQL[i]) != SQLITE_OK) {
                log_error("Failed to create schema");
                return -1;
            }
        }

        // Update schema version
        db_set_schema_version(CURRENT_SCHEMA_VERSION);
    }

    log_info("Database initialized (schema v%d)", CURRENT_SCHEMA_VERSION);
    return 0;
}

void db_deinit(void) {
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
        log_info("Database closed");
    }
}

int db_exec(const char *sql) {
    char *err_msg = NULL;
    int rc = sqlite3_exec(g_db, sql, NULL, NULL, &err_msg);

    if (rc != SQLITE_OK) {
        log_error("SQL error: %s", err_msg);
        sqlite3_free(err_msg);
        return rc;
    }

    return SQLITE_OK;
}

sqlite3_stmt *db_prepare(const char *sql) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        log_error("Failed to prepare statement: %s", sqlite3_errmsg(g_db));
        return NULL;
    }

    return stmt;
}

int db_step(sqlite3_stmt *stmt) {
    return sqlite3_step(stmt);
}

void db_finalize(sqlite3_stmt *stmt) {
    if (stmt) {
        sqlite3_finalize(stmt);
    }
}

int db_query_int(const char *sql, int *result) {
    if (!result) return -1;

    sqlite3_stmt *stmt = db_prepare(sql);
    if (!stmt) return -1;

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        *result = sqlite3_column_int(stmt, 0);
        db_finalize(stmt);
        return 0;
    }

    db_finalize(stmt);
    return -1;
}

int db_query_text(const char *sql, char **result) {
    if (!result) return -1;

    sqlite3_stmt *stmt = db_prepare(sql);
    if (!stmt) return -1;

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char *text = sqlite3_column_text(stmt, 0);
        if (text) {
            *result = strdup((const char *)text);
            db_finalize(stmt);
            return 0;
        }
    }

    db_finalize(stmt);
    return -1;
}

int db_query_blob(const char *sql, unsigned char **result, int *size) {
    if (!result || !size) return -1;

    sqlite3_stmt *stmt = db_prepare(sql);
    if (!stmt) return -1;

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const void *blob = sqlite3_column_blob(stmt, 0);
        int blob_size = sqlite3_column_bytes(stmt, 0);
        if (blob && blob_size > 0) {
            *result = malloc(blob_size);
            memcpy(*result, blob, blob_size);
            *size = blob_size;
            db_finalize(stmt);
            return 0;
        }
    }

    db_finalize(stmt);
    return -1;
}

int db_begin(void) {
    return db_exec("BEGIN TRANSACTION;");
}

int db_commit(void) {
    return db_exec("COMMIT;");
}

int db_rollback(void) {
    return db_exec("ROLLBACK;");
}

int db_get_schema_version(void) {
    int version = 0;
    char *err_msg = NULL;
    sqlite3_stmt *stmt = NULL;

    // Check if schema_version table exists
    int rc = sqlite3_prepare_v2(g_db,
        "SELECT user_version FROM pragma_user_version;", -1, &stmt, NULL);

    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }

    if (stmt) sqlite3_finalize(stmt);

    log_debug("Current schema version: %d", version);
    return version;
}

int db_set_schema_version(int version) {
    char sql[64];
    snprintf(sql, sizeof(sql), "PRAGMA user_version = %d;", version);
    return db_exec(sql);
}
