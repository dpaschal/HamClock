#ifndef HAMCLOCK_NOAA_H
#define HAMCLOCK_NOAA_H

#include "../core/state.h"
#include <time.h>

// NOAA Space Weather consolidated API
// Fetches Kp-index, A-index, Solar Flux, Sunspot Number in ONE call
// (vs 3 separate calls in original hamclock)

typedef struct {
    float kp_index;
    float a_index;
    float solar_flux;
    int sunspot_number;
    char xray_flux[32];
    float solar_wind_speed;
    time_t timestamp;
    int success;
} noaa_data_t;

// Fetch consolidated NOAA space weather data
// Uses ETag/If-Modified-Since caching
// Returns 0 on success, -1 on error
int noaa_fetch_space_weather(noaa_data_t *data);

// Store space weather data to database for historical tracking
int noaa_store_space_weather(const noaa_data_t *data);

// Get latest space weather from database
int noaa_get_latest_space_weather(noaa_data_t *data);

// Get space weather history (last N hours)
// Returns number of records fetched
int noaa_get_history(noaa_data_t **history, int max_records, int hours_back);

#endif // HAMCLOCK_NOAA_H
