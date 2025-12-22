#include "noaa.h"
#include "http_client.h"
#include "../core/log.h"
#include "../data/database.h"
#include "../data/cache.h"
#include "../utils/json_simple.h"
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// NOAA Space Weather Prediction Center consolidated API
// This single endpoint provides: Kp, A-index, Solar Flux, Sunspot Number
// vs 3-4 separate calls in the original hamclock
#define NOAA_SPACE_WEATHER_URL \
    "https://services.swpc.noaa.gov/json/solar-cycle/observed-solar-cycle-indices.json"

// Cache TTL: 1 hour (NOAA updates 3x per day, we check hourly)
#define NOAA_CACHE_TTL (60 * 60)

int noaa_fetch_space_weather(noaa_data_t *data) {
    if (!data) return -1;

    memset(data, 0, sizeof(*data));

    log_info("Fetching NOAA space weather...");

    // HTTP request with caching
    http_response_t response = {0};
    if (http_get(NOAA_SPACE_WEATHER_URL, &response, NOAA_CACHE_TTL) != 0) {
        log_error("Failed to fetch NOAA data");
        return -1;
    }

    if (response.size <= 0 || !response.data) {
        log_error("Empty NOAA response");
        return -1;
    }

    // Convert to null-terminated string for JSON parsing
    char *json_str = malloc(response.size + 1);
    if (!json_str) {
        log_error("Memory allocation failed for JSON parsing");
        http_response_free(&response);
        return -1;
    }

    memcpy(json_str, response.data, response.size);
    json_str[response.size] = 0;

    // Parse JSON response
    // NOAA response structure:
    // {"kind":"observed-solar-cycle-indices","description":"...",
    //  "data":[{"time_tag":"YYYY-MM-DD","kp":"x.x","a":"x","solar_flux":"xxx",...}]}

    // Try to get the latest data point (assuming first in array)
    const char *data_elem = json_get_array_element(json_str, "data", 0);
    if (!data_elem) {
        log_error("Invalid NOAA JSON structure");
        free(json_str);
        http_response_free(&response);
        return -1;
    }

    // Extract values from the data element
    int rc = 0;
    float kp = 0;
    float a_index = 0;
    int flux = 0;
    int ssn = 0;

    if (json_get_float(data_elem, "kp", &kp) != 0) {
        log_warn("Failed to parse Kp index from NOAA");
        rc = -1;
    }
    if (json_get_float(data_elem, "a", &a_index) != 0) {
        log_warn("Failed to parse A-index from NOAA");
        rc = -1;
    }
    if (json_get_int(data_elem, "solar_flux", &flux) != 0) {
        log_warn("Failed to parse Solar Flux from NOAA");
        rc = -1;
    }
    if (json_get_int(data_elem, "ssn", &ssn) != 0) {
        log_warn("Failed to parse SSN from NOAA");
        rc = -1;
    }

    if (rc != 0) {
        log_error("Failed to parse NOAA data");
        free(json_str);
        http_response_free(&response);
        return -1;
    }

    // Populate result
    data->kp_index = kp;
    data->a_index = a_index;
    data->solar_flux = (float)flux;
    data->sunspot_number = ssn;
    data->timestamp = time(NULL);
    data->success = 1;

    // Store to database
    noaa_store_space_weather(data);

    log_info("NOAA data fetched: Kp=%.1f A=%d Flux=%d SSN=%d",
             data->kp_index, (int)data->a_index, (int)data->solar_flux,
             data->sunspot_number);

    free(json_str);
    http_response_free(&response);

    return 0;
}

int noaa_store_space_weather(const noaa_data_t *data) {
    if (!data) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "INSERT INTO space_weather "
        "(timestamp, kp_index, a_index, solar_flux, sunspot_number, xray_flux, solar_wind_speed) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"
    );
    if (!stmt) return -1;

    sqlite3_bind_int64(stmt, 1, data->timestamp);
    sqlite3_bind_double(stmt, 2, data->kp_index);
    sqlite3_bind_double(stmt, 3, data->a_index);
    sqlite3_bind_double(stmt, 4, data->solar_flux);
    sqlite3_bind_int(stmt, 5, data->sunspot_number);
    sqlite3_bind_text(stmt, 6, data->xray_flux, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 7, data->solar_wind_speed);

    int rc = sqlite3_step(stmt);
    db_finalize(stmt);

    if (rc == SQLITE_DONE) {
        log_debug("Stored space weather to database");
        return 0;
    }

    log_error("Failed to store space weather");
    return -1;
}

int noaa_get_latest_space_weather(noaa_data_t *data) {
    if (!data) return -1;

    sqlite3_stmt *stmt = db_prepare(
        "SELECT timestamp, kp_index, a_index, solar_flux, sunspot_number, xray_flux, solar_wind_speed "
        "FROM space_weather ORDER BY timestamp DESC LIMIT 1"
    );
    if (!stmt) return -1;

    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        data->timestamp = (time_t)sqlite3_column_int64(stmt, 0);
        data->kp_index = (float)sqlite3_column_double(stmt, 1);
        data->a_index = (float)sqlite3_column_double(stmt, 2);
        data->solar_flux = (float)sqlite3_column_double(stmt, 3);
        data->sunspot_number = sqlite3_column_int(stmt, 4);

        const unsigned char *xray = sqlite3_column_text(stmt, 5);
        if (xray) {
            strncpy(data->xray_flux, (const char *)xray, sizeof(data->xray_flux) - 1);
        }

        data->solar_wind_speed = (float)sqlite3_column_double(stmt, 6);
        data->success = 1;

        db_finalize(stmt);
        return 0;
    }

    db_finalize(stmt);
    return -1;
}

int noaa_get_history(noaa_data_t **history, int max_records, int hours_back) {
    if (!history || max_records <= 0) return 0;

    time_t cutoff = time(NULL) - (hours_back * 3600);

    sqlite3_stmt *stmt = db_prepare(
        "SELECT timestamp, kp_index, a_index, solar_flux, sunspot_number, xray_flux, solar_wind_speed "
        "FROM space_weather WHERE timestamp > ?1 ORDER BY timestamp DESC LIMIT ?2"
    );
    if (!stmt) return 0;

    sqlite3_bind_int64(stmt, 1, cutoff);
    sqlite3_bind_int(stmt, 2, max_records);

    *history = calloc(max_records, sizeof(noaa_data_t));
    if (!*history) {
        db_finalize(stmt);
        return 0;
    }

    int count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_records) {
        noaa_data_t *record = &((*history)[count]);

        record->timestamp = (time_t)sqlite3_column_int64(stmt, 0);
        record->kp_index = (float)sqlite3_column_double(stmt, 1);
        record->a_index = (float)sqlite3_column_double(stmt, 2);
        record->solar_flux = (float)sqlite3_column_double(stmt, 3);
        record->sunspot_number = sqlite3_column_int(stmt, 4);

        const unsigned char *xray = sqlite3_column_text(stmt, 5);
        if (xray) {
            strncpy(record->xray_flux, (const char *)xray, sizeof(record->xray_flux) - 1);
        }

        record->solar_wind_speed = (float)sqlite3_column_double(stmt, 6);
        record->success = 1;

        count++;
    }

    db_finalize(stmt);
    return count;
}
