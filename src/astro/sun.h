#ifndef HAMCLOCK_SUN_H
#define HAMCLOCK_SUN_H

#include <time.h>
#include <math.h>

// Seasons
typedef enum {
    SEASON_SPRING,    // Vernal Equinox (March 20/21)
    SEASON_SUMMER,    // Summer Solstice (June 20/21)
    SEASON_AUTUMN,    // Autumnal Equinox (Sept 22/23)
    SEASON_WINTER     // Winter Solstice (Dec 21/22)
} season_t;

// Solar position and timing data
typedef struct {
    double declination;      // Solar declination in degrees (-23.44 to +23.44)
    double equation_of_time; // Equation of time in minutes
    double subsolar_lat;     // Latitude of subsolar point (always 0 or +/-declination)
    double subsolar_lon;     // Longitude of subsolar point (0 at solar noon)
    time_t sunrise_time;     // Sunrise time (local, UTC if not specified)
    time_t sunset_time;      // Sunset time (local, UTC if not specified)
    double solar_noon_jd;    // Julian Day of solar noon
    int is_daylight;         // 1 if observer is in daylight, 0 if night
    season_t season;         // Current season
    const char *season_name; // Human-readable season ("Spring", "Summer", etc.)
    time_t solstice_equinox; // Time of next solstice/equinox
} sun_position_t;

// Calculate solar position for given time
// time_t can be any Unix timestamp
// observer_lat, observer_lon in decimal degrees (-90 to 90, -180 to 180)
int sun_calculate_position(time_t when, double observer_lat, double observer_lon,
                          sun_position_t *result);

// Get current solar declination (simplified, ~1.2 degree error)
double sun_get_declination(time_t when);

// Get equation of time in minutes
double sun_get_equation_of_time(time_t when);

// Check if location is in daylight (simple: latitude vs sun declination)
int sun_is_daylight(double observer_lat, double subsolar_lat);

// Calculate greyline (twilight zone) latitude at given longitude
// Returns the latitude of the twilight zone edge
// Returns -999 if location is in daylight or full night (no twilight edge)
double sun_greyline_latitude(double lon);

// Get solar noon time (in UTC) for given date at observer location
time_t sun_get_solar_noon(time_t when, double observer_lon);

// Get current season based on solar declination
season_t sun_get_season(time_t when);

// Get human-readable season name
const char *sun_get_season_name(season_t season);

// Get next solstice/equinox time
time_t sun_get_next_solstice_equinox(time_t when);

#endif // HAMCLOCK_SUN_H
