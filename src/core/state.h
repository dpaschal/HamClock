#ifndef HAMCLOCK_STATE_H
#define HAMCLOCK_STATE_H

#include <time.h>

typedef struct {
    float kp_index;
    float a_index;
    float solar_flux;
    int sunspot_number;
    char xray_flux[32];
    float solar_wind_speed;
    time_t timestamp;
} space_weather_t;

typedef struct {
    float de_lat;
    float de_lon;
    float dx_lat;
    float dx_lon;
    char de_grid[16];
    char dx_grid[16];
} location_t;

// Global application state
int state_init(void);
void state_deinit(void);

// Space weather accessors
space_weather_t *state_get_space_weather(void);
int state_set_space_weather(const space_weather_t *data);

// Location accessors
location_t *state_get_location(void);
int state_set_location(const location_t *data);

#endif // HAMCLOCK_STATE_H
