#include "sun.h"
#include <math.h>
#include <time.h>

#define PI 3.14159265358979323846
#define DEG2RAD(d) ((d) * PI / 180.0)
#define RAD2DEG(r) ((r) * 180.0 / PI)

// Convert Unix timestamp to Julian Day Number (JD 2000.0 epoch)
// JD 2000.0 = January 1.5, 2000 = JD 2451545.0
static double unix_to_jd(time_t when) {
    // Unix epoch: January 1, 1970, 00:00:00 UTC = JD 2440587.5
    // Seconds since Unix epoch -> days
    double days_since_1970 = (double)when / 86400.0;
    return 2440587.5 + days_since_1970;
}

// Convert Julian Day to Julian Centuries since J2000.0
static double jd_to_jcentury(double jd) {
    return (jd - 2451545.0) / 36525.0;
}

// Calculate mean solar longitude (degrees)
// Based on VSOP87 simplified formula
static double mean_solar_longitude(double t) {
    // L0 = 280.46646 + 36000.76983*T + 0.0003032*T^2
    return 280.46646 + 36000.76983 * t + 0.0003032 * t * t;
}

// Calculate mean anomaly of sun (degrees)
static double mean_solar_anomaly(double t) {
    // M = 357.52911 + 35999.05029*T - 0.0001536*T^2
    return 357.52911 + 35999.05029 * t - 0.0001536 * t * t;
}

// Calculate equation of center (degrees)
// Simplified from full series
static double equation_of_center(double m) {
    double m_rad = DEG2RAD(m);
    double c = (1.914602 - 0.004817 * m - 0.000014 * m * m) * sin(m_rad)
             + (0.019993 - 0.000101 * m) * sin(2 * m_rad)
             + 0.000029 * sin(3 * m_rad);
    return c;
}

// Calculate apparent solar longitude (degrees)
static double apparent_solar_longitude(double t) {
    double l0 = mean_solar_longitude(t);
    double m = mean_solar_anomaly(t);
    double c = equation_of_center(m);

    // Corrections for aberration and nutation
    // Omega = 125.04 - 1934.136 * T
    double omega = 125.04 - 1934.136 * t;
    double lambda = l0 + c - 0.00569 - 0.00478 * sin(DEG2RAD(omega));

    // Normalize to 0-360
    while (lambda < 0) lambda += 360;
    while (lambda >= 360) lambda -= 360;

    return lambda;
}

// Calculate solar declination (degrees)
// Declination = arcsin(sin(epsilon) * sin(lambda))
// where epsilon = obliquity of ecliptic
static double solar_declination(double t) {
    double lambda = apparent_solar_longitude(t);

    // Obliquity of ecliptic
    // epsilon = 23.439291 - 0.0130042*T - 0.00000016*T^2 + 0.000000504*T^3
    double epsilon = 23.439291 - 0.0130042 * t - 0.00000016 * t * t + 0.000000504 * t * t * t;

    double dec = asin(sin(DEG2RAD(epsilon)) * sin(DEG2RAD(lambda)));
    return RAD2DEG(dec);
}

// Calculate equation of time (minutes)
// EoT = L0 - 0.0057183 - alpha + delta_psi * cos(epsilon)
// where alpha = atan2(sin(2*L0), 5.1364 + cos(2*L0))
static double equation_of_time(double t) {
    double l0 = mean_solar_longitude(t);
    double m = mean_solar_anomaly(t);

    // Obliquity
    double epsilon = 23.439291 - 0.0130042 * t;

    // Equation of time (simplified formula)
    // EoT = 229.18 * (0.000075 + 0.001868*cos(B) - 0.032077*sin(B)
    //                 - 0.014615*cos(2*B) - 0.040849*sin(2*B))
    double b = (1 - t / 36525.0) * 360.0;  // Day of year angle
    double b_rad = DEG2RAD(b);

    double eot = 229.18 * (0.000075 + 0.001868 * cos(b_rad)
                          - 0.032077 * sin(b_rad)
                          - 0.014615 * cos(2 * b_rad)
                          - 0.040849 * sin(2 * b_rad));

    return eot;
}

// Calculate sunrise/sunset times (simplified approximation)
// For more precision, this would need iterative calculation
static void calculate_sunrise_sunset(time_t when, double observer_lat, double observer_lon,
                                      time_t *sunrise, time_t *sunset) {
    struct tm *tm_info = gmtime(&when);
    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;
    int day = tm_info->tm_mday;

    // Day of year
    int doy = tm_info->tm_yday + 1;

    // Fractional year angle
    double gamma = 2.0 * PI * (doy - 1.0) / 365.0;

    // Solar declination (simple approximation)
    double declination = 0.006918 - 0.399912 * cos(gamma) + 0.070257 * sin(gamma)
                      - 0.006758 * cos(2 * gamma) + 0.000907 * sin(2 * gamma)
                      - 0.002697 * cos(3 * gamma) + 0.00148 * sin(3 * gamma);

    // Equation of time (simple approximation in minutes)
    double eot = 229.2 * (0.000075 + 0.001868 * cos(gamma) - 0.032077 * sin(gamma)
                         - 0.014615 * cos(2 * gamma) - 0.040849 * sin(2 * gamma));

    // Sunrise/sunset hour angle
    double lat_rad = DEG2RAD(observer_lat);
    double dec_rad = declination;

    double cos_h = -tan(lat_rad) * tan(dec_rad);
    if (cos_h > 1.0) {
        // Polar night
        *sunrise = when + 86400;
        *sunset = when;
        return;
    } else if (cos_h < -1.0) {
        // Polar day
        *sunrise = when;
        *sunset = when + 86400;
        return;
    }

    double h = acos(cos_h);  // Hour angle in radians

    // Solar noon (local)
    double solar_noon_minutes = 720.0 - 4.0 * observer_lon - eot;

    // Sunrise/sunset in minutes from midnight UTC
    double sunrise_minutes = solar_noon_minutes - RAD2DEG(h) * 4.0;
    double sunset_minutes = solar_noon_minutes + RAD2DEG(h) * 4.0;

    // Convert to time_t
    *sunrise = when - (tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec)
             + (time_t)(sunrise_minutes * 60.0);
    *sunset = when - (tm_info->tm_hour * 3600 + tm_info->tm_min * 60 + tm_info->tm_sec)
            + (time_t)(sunset_minutes * 60.0);
}

int sun_calculate_position(time_t when, double observer_lat, double observer_lon,
                          sun_position_t *result) {
    if (!result) return -1;

    // Convert to Julian Day and Julian Century
    double jd = unix_to_jd(when);
    double t = jd_to_jcentury(jd);

    // Calculate solar position
    result->declination = solar_declination(t);
    result->equation_of_time = equation_of_time(t);
    result->subsolar_lat = result->declination;

    // Subsolar longitude (0 at solar noon UTC)
    // Hour angle at observer location
    struct tm *tm_info = gmtime(&when);
    int hour_utc = tm_info->tm_hour;
    int min_utc = tm_info->tm_min;
    int sec_utc = tm_info->tm_sec;

    // Time in hours since midnight UTC
    double time_utc = hour_utc + min_utc / 60.0 + sec_utc / 3600.0;

    // Subsolar longitude (rotates 360 degrees per 24 hours)
    // At time 0 UTC, subsolar point is at 0 longitude (by definition)
    result->subsolar_lon = (time_utc / 24.0) * 360.0 - 180.0;
    while (result->subsolar_lon < -180) result->subsolar_lon += 360;
    while (result->subsolar_lon > 180) result->subsolar_lon -= 360;

    // Get sunrise/sunset times
    calculate_sunrise_sunset(when, observer_lat, observer_lon,
                            &result->sunrise_time, &result->sunset_time);

    // Solar noon
    result->solar_noon_jd = jd + (0.5 - (observer_lon / 360.0));  // Approximate

    // Check if observer is in daylight
    result->is_daylight = sun_is_daylight(observer_lat, result->subsolar_lat);

    return 0;
}

double sun_get_declination(time_t when) {
    double jd = unix_to_jd(when);
    double t = jd_to_jcentury(jd);
    return solar_declination(t);
}

double sun_get_equation_of_time(time_t when) {
    double jd = unix_to_jd(when);
    double t = jd_to_jcentury(jd);
    return equation_of_time(t);
}

int sun_is_daylight(double observer_lat, double subsolar_lat) {
    // Terminator is at latitude = subsolar_lat ± 90 degrees longitude
    // But we need to check if observer is between the terminator lines

    // Simplified: observer is in daylight if lat and subsolar_lat have
    // the same sign and |lat| <= |subsolar_lat| + 90 (nearly always true)
    // More accurate: observer is in daylight if angular distance from
    // subsolar point is < 90 degrees

    // For now, simple heuristic:
    // If observer latitude is between subsolar ± ~90 degrees (accounting for Earth curvature)
    double diff = observer_lat - subsolar_lat;
    while (diff > 180) diff -= 360;
    while (diff < -180) diff += 360;

    // Observer is in daylight if within ~90 degrees of subsolar point
    return (diff > -90.0 && diff < 90.0) ? 1 : 0;
}

double sun_greyline_latitude(double lon) {
    // Greyline (twilight zone) occurs at sun's terminator
    // For a given longitude, find where the sun's terminator passes
    // Terminator is perpendicular to subsolar point

    // This is complex - requires iterative solution
    // For now, return -999 (not implemented)
    (void)lon;
    return -999.0;
}

time_t sun_get_solar_noon(time_t when, double observer_lon) {
    double jd = unix_to_jd(when);
    double t = jd_to_jcentury(jd);

    // Get equation of time
    double eot = equation_of_time(t);

    // Solar noon occurs when hour angle = 0
    // Local solar time at observer = 12:00 (noon)
    // Solar noon in UTC = 12:00 - 4*lon/60 - EoT/60

    // Get midnight UTC of this day
    struct tm *tm_info = gmtime(&when);
    tm_info->tm_hour = 0;
    tm_info->tm_min = 0;
    tm_info->tm_sec = 0;
    time_t midnight = mktime(tm_info);

    // Solar noon in seconds from midnight UTC
    double noon_seconds = 12.0 * 3600.0 - (observer_lon / 15.0) * 3600.0 - (eot / 60.0) * 60.0;

    return midnight + (time_t)noon_seconds;
}

season_t sun_get_season(time_t when) {
    double dec = sun_get_declination(when);

    // Seasons in Northern Hemisphere:
    // Spring: March 20/21 - June 20/21 (declination: 0 -> +23.44)
    // Summer: June 20/21 - Sept 22/23 (declination: +23.44 -> 0)
    // Autumn: Sept 22/23 - Dec 21/22 (declination: 0 -> -23.44)
    // Winter: Dec 21/22 - March 20/21 (declination: -23.44 -> 0)

    if (dec >= -1.75 && dec < 5.0) return SEASON_SPRING;      // Equinox to near solstice
    if (dec >= 5.0 && dec <= 23.44) return SEASON_SUMMER;     // Solstice region
    if (dec > -5.0 && dec < 5.0) {                           // Near equinoxes
        struct tm *tm_info = gmtime(&when);
        int month = tm_info->tm_mon + 1;
        if (month >= 3 && month <= 5) return SEASON_SPRING;
        if (month >= 6 && month <= 8) return SEASON_SUMMER;
        if (month >= 9 && month <= 11) return SEASON_AUTUMN;
        return SEASON_WINTER;
    }
    if (dec < -5.0) return SEASON_AUTUMN;                    // Southern hemisphere fall / NH autumn
    if (dec < -1.75) return SEASON_WINTER;

    // Fallback
    struct tm *tm_info = gmtime(&when);
    int month = tm_info->tm_mon + 1;
    if (month >= 3 && month <= 5) return SEASON_SPRING;
    if (month >= 6 && month <= 8) return SEASON_SUMMER;
    if (month >= 9 && month <= 11) return SEASON_AUTUMN;
    return SEASON_WINTER;
}

const char *sun_get_season_name(season_t season) {
    switch (season) {
        case SEASON_SPRING: return "Spring";
        case SEASON_SUMMER: return "Summer";
        case SEASON_AUTUMN: return "Autumn";
        case SEASON_WINTER: return "Winter";
        default: return "Unknown";
    }
}

time_t sun_get_next_solstice_equinox(time_t when) {
    struct tm *tm_info = gmtime(&when);
    int year = tm_info->tm_year + 1900;
    int month = tm_info->tm_mon + 1;
    int day = tm_info->tm_mday;

    // Approximate dates for solstices/equinoxes
    struct {
        int month, day;
        const char *name;
    } events[] = {
        {3, 20},   // Vernal Equinox (Spring)
        {6, 21},   // Summer Solstice
        {9, 22},   // Autumnal Equinox
        {12, 21},  // Winter Solstice
    };

    // Find next event
    for (int i = 0; i < 4; i++) {
        if (events[i].month > month || (events[i].month == month && events[i].day > day)) {
            // Found next event in current year
            struct tm target = {0};
            target.tm_year = year - 1900;
            target.tm_mon = events[i].month - 1;
            target.tm_mday = events[i].day;
            target.tm_hour = 12;
            target.tm_isdst = -1;
            return mktime(&target);
        }
    }

    // Next event is in next year (Winter solstice Dec 21)
    struct tm target = {0};
    target.tm_year = year + 1 - 1900;
    target.tm_mon = 11;  // December (0-indexed)
    target.tm_mday = 21;
    target.tm_hour = 12;
    target.tm_isdst = -1;
    return mktime(&target);
}
