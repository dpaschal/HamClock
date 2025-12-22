#include "moon.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DEG2RAD(d) ((d) * PI / 180.0)
#define RAD2DEG(r) ((r) * 180.0 / PI)

// Lunar month (synodic period) in days
#define LUNAR_MONTH 29.530588861

// Reference new moon: January 6, 2000, 18:14 UTC (JD 2451550.26)
#define REFERENCE_NEW_MOON_JD 2451550.26
#define REFERENCE_NEW_MOON_TIME 947727240  // Unix timestamp

// Convert Unix timestamp to Julian Day Number
static double unix_to_jd(time_t when) {
    // Unix epoch: January 1, 1970, 00:00:00 UTC = JD 2440587.5
    double days_since_1970 = (double)when / 86400.0;
    return 2440587.5 + days_since_1970;
}

// Convert Julian Day to Julian Centuries since J2000.0
static double jd_to_jcentury(double jd) {
    return (jd - 2451545.0) / 36525.0;
}

// Calculate moon's mean longitude (degrees)
// Based on Meeus algorithm
static double moon_mean_longitude(double t) {
    // L' = 218.3164477 + 481267.88123421*T - 0.0015786*T^2 + T^3/538841 - T^4/65194000
    return 218.3164477 + 481267.88123421 * t - 0.0015786 * t * t
         + (t * t * t) / 538841.0 - (t * t * t * t) / 65194000.0;
}

// Calculate moon's mean anomaly (degrees)
static double moon_mean_anomaly(double t) {
    // M' = 134.9634814 + 477198.8676313*T + 0.0089970*T^2 + T^3/69699 - T^4/14712000
    return 134.9634814 + 477198.8676313 * t + 0.0089970 * t * t
         + (t * t * t) / 69699.0 - (t * t * t * t) / 14712000.0;
}

// Normalize angle to 0-360 range
static double normalize_angle(double angle) {
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;
    return angle;
}

// Calculate moon's illumination percentage
// Phase angle = difference between moon's longitude and sun's longitude
static double moon_illumination_from_angle(double phase_angle) {
    // Normalize to 0-180 range for calculating illumination
    double norm_angle = normalize_angle(phase_angle);
    if (norm_angle > 180) norm_angle = 360 - norm_angle;

    // Illumination = (1 + cos(phase_angle)) / 2 * 100
    // But we use simplified: if phase_angle is 0-180, illum = phase/180*100
    // If phase is 180-360, illum = (360-phase)/180*100

    phase_angle = normalize_angle(phase_angle);
    if (phase_angle <= 180) {
        return (phase_angle / 180.0) * 100.0;
    } else {
        return ((360.0 - phase_angle) / 180.0) * 100.0;
    }
}

// Get sun's longitude (simplified)
static double sun_longitude(double t) {
    // Mean solar longitude
    double l0 = 280.46646 + 36000.76983 * t + 0.0003032 * t * t;
    return normalize_angle(l0);
}

int moon_calculate_position(time_t when, moon_position_t *result) {
    if (!result) return -1;

    double jd = unix_to_jd(when);
    double t = jd_to_jcentury(jd);

    // Moon's mean longitude
    double moon_lon = moon_mean_longitude(t);
    double moon_anom = moon_mean_anomaly(t);

    // Sun's longitude (approximate)
    double sun_lon = sun_longitude(t);

    // Phase angle (elongation): difference in longitude
    double phase_angle = normalize_angle(moon_lon - sun_lon);

    // Age since new moon (0 = new, 180 = full)
    result->age = (phase_angle / 360.0) * LUNAR_MONTH;
    result->phase = phase_angle;
    result->illumination = moon_illumination_from_angle(phase_angle);

    // Determine phase number
    if (result->age < 1.84) {
        result->phase_number = 0;  // New Moon
    } else if (result->age < 7.38) {
        result->phase_number = 1;  // Waxing Crescent
    } else if (result->age < 9.23) {
        result->phase_number = 2;  // First Quarter
    } else if (result->age < 14.77) {
        result->phase_number = 2;  // Waxing Gibbous
    } else if (result->age < 16.61) {
        result->phase_number = 3;  // Full Moon
    } else if (result->age < 22.15) {
        result->phase_number = 3;  // Waning Gibbous
    } else if (result->age < 23.99) {
        result->phase_number = 2;  // Last Quarter
    } else {
        result->phase_number = 1;  // Waning Crescent
    }

    result->phase_name = moon_get_phase_name(result->illumination);

    // Moon's declination (simplified - between -28.3 and +28.3 degrees)
    // Full calculation requires more complex ephemeris
    double inclination = 5.145396;  // Moon's orbit inclination
    result->declination = asin(sin(DEG2RAD(inclination)) * sin(DEG2RAD(moon_anom))) * (180.0 / PI);

    // Moon's right ascension (hours, 0-24)
    // Simplified: RA ≈ moon_lon / 15 degrees per hour
    result->right_ascension = normalize_angle(moon_lon) / 15.0;

    // Next new moon
    double days_to_new = LUNAR_MONTH - result->age;
    result->next_new_moon = when + (time_t)(days_to_new * 86400.0);

    // Next full moon
    double days_to_full = (LUNAR_MONTH / 2.0) - result->age;
    if (days_to_full < 0) days_to_full += LUNAR_MONTH;
    result->next_full_moon = when + (time_t)(days_to_full * 86400.0);

    return 0;
}

double moon_get_age(time_t when) {
    moon_position_t pos;
    if (moon_calculate_position(when, &pos) == 0) {
        return pos.age;
    }
    return 0.0;
}

double moon_get_illumination(time_t when) {
    moon_position_t pos;
    if (moon_calculate_position(when, &pos) == 0) {
        return pos.illumination;
    }
    return 0.0;
}

const char *moon_get_phase_name(double illumination) {
    // Based on illumination percentage (0-100)
    if (illumination < 5) return "New Moon";
    if (illumination < 25) return "Waxing Crescent";
    if (illumination < 30) return "First Quarter";
    if (illumination < 50) return "Waxing Gibbous";
    if (illumination < 55) return "Full Moon";
    if (illumination < 75) return "Waning Gibbous";
    if (illumination < 80) return "Last Quarter";
    return "Waning Crescent";
}

void moon_get_next_new_moon(time_t after_this, time_t *next_new) {
    if (!next_new) return;

    moon_position_t pos;
    moon_calculate_position(after_this, &pos);

    // If we're past new moon in current cycle, estimate next
    // Otherwise use calculated next new moon
    if (pos.age < 0.5 || pos.age > LUNAR_MONTH - 0.5) {
        *next_new = after_this + (time_t)((LUNAR_MONTH - pos.age) * 86400.0);
    } else {
        *next_new = pos.next_new_moon;
    }
}

void moon_get_next_full_moon(time_t after_this, time_t *next_full) {
    if (!next_full) return;

    moon_position_t pos;
    moon_calculate_position(after_this, &pos);

    // If we're past full moon in current cycle, estimate next
    // Otherwise use calculated next full moon
    double days_to_full = (LUNAR_MONTH / 2.0) - pos.age;
    if (days_to_full < 0) days_to_full += LUNAR_MONTH;

    *next_full = after_this + (time_t)(days_to_full * 86400.0);
}
