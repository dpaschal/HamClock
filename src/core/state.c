#include "state.h"
#include "config.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

static space_weather_t g_space_weather = {0};
static location_t g_location = {0};

int state_init(void) {
    // Initialize location from config
    char buf[64];

    if (config_get("de_lat", buf, sizeof(buf)) == 0) {
        g_location.de_lat = atof(buf);
    }
    if (config_get("de_lon", buf, sizeof(buf)) == 0) {
        g_location.de_lon = atof(buf);
    }
    if (config_get("de_grid", buf, sizeof(buf)) == 0) {
        strncpy(g_location.de_grid, buf, sizeof(g_location.de_grid) - 1);
    }

    log_info("State initialized");
    return 0;
}

void state_deinit(void) {
    // Nothing to clean up yet
}

space_weather_t *state_get_space_weather(void) {
    return &g_space_weather;
}

int state_set_space_weather(const space_weather_t *data) {
    if (!data) return -1;
    memcpy(&g_space_weather, data, sizeof(space_weather_t));
    return 0;
}

location_t *state_get_location(void) {
    return &g_location;
}

int state_set_location(const location_t *data) {
    if (!data) return -1;
    memcpy(&g_location, data, sizeof(location_t));
    return 0;
}
