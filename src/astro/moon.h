#ifndef HAMCLOCK_MOON_H
#define HAMCLOCK_MOON_H

#include <time.h>

// Moon phase and position data
typedef struct {
    double age;              // Days since new moon (0-29.53)
    double illumination;     // Percentage illuminated (0-100)
    double phase;            // Phase angle in degrees (0-360)
    int phase_number;        // 0=new, 1=waxing, 2=full, 3=waning
    const char *phase_name;  // "New Moon", "Waxing Crescent", "First Quarter", etc.
    double declination;      // Moon's declination in degrees
    double right_ascension;  // Moon's right ascension in hours (0-24)
    time_t next_new_moon;    // Estimated time of next new moon
    time_t next_full_moon;   // Estimated time of next full moon
} moon_position_t;

// Calculate lunar position and phase for given time
int moon_calculate_position(time_t when, moon_position_t *result);

// Get moon age (days since last new moon) - simple approximation
double moon_get_age(time_t when);

// Get moon illumination percentage (0-100)
double moon_get_illumination(time_t when);

// Get human-readable phase name
const char *moon_get_phase_name(double illumination);

// Get next new/full moon times (estimated)
void moon_get_next_new_moon(time_t after_this, time_t *next_new);
void moon_get_next_full_moon(time_t after_this, time_t *next_full);

#endif // HAMCLOCK_MOON_H
