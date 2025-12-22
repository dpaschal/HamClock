#ifndef HAMCLOCK_DATABASE_H
#define HAMCLOCK_DATABASE_H

#include <sqlite3.h>
#include <time.h>

// Database handle
sqlite3 *db_get_handle(void);

// Initialize database
int db_init(const char *db_path);
void db_deinit(void);

// Execute SQL statement
int db_exec(const char *sql);

// Prepared statement helpers
sqlite3_stmt *db_prepare(const char *sql);
int db_step(sqlite3_stmt *stmt);
void db_finalize(sqlite3_stmt *stmt);

// Query helpers
int db_query_int(const char *sql, int *result);
int db_query_text(const char *sql, char **result);
int db_query_blob(const char *sql, unsigned char **result, int *size);

// Transaction helpers
int db_begin(void);
int db_commit(void);
int db_rollback(void);

// Schema version
int db_get_schema_version(void);
int db_set_schema_version(int version);

#endif // HAMCLOCK_DATABASE_H
