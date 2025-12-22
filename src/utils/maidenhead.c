#include "maidenhead.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

// Maidenhead grid system:
// Field: 18 wide (20 degrees) x 18 tall (10 degrees)
// Square: 10 wide (2 degrees) x 10 tall (1 degree)
// Subsquare: 24 wide (5 min) x 24 tall (2.5 min)

int maidenhead_from_latlon(double lat, double lon, maidenhead_t *result) {
    if (!result) return -1;

    // Normalize latitude to 0-180 (South Pole=0, Equator=90, North Pole=180)
    double lat_norm = lat + 90.0;
    if (lat_norm < 0 || lat_norm > 180) return -1;

    // Normalize longitude to 0-360 (Int'l Date Line=0, Prime Meridian=180, Date Line=360)
    double lon_norm = lon + 180.0;
    if (lon_norm < 0 || lon_norm >= 360) lon_norm = fmod(lon_norm, 360.0);
    if (lon_norm < 0) lon_norm += 360.0;

    // Field: 20 degrees latitude, 20 degrees longitude (but lat is halved to 10 per field)
    int field_lat = (int)(lat_norm / 10.0);  // 0-17
    int field_lon = (int)(lon_norm / 20.0);  // 0-17
    char field_lat_char = 'A' + field_lat;
    char field_lon_char = 'A' + field_lon;

    // Remainder after field
    double lat_rem = fmod(lat_norm, 10.0);
    double lon_rem = fmod(lon_norm, 20.0);

    // Square: 1 degree latitude, 2 degrees longitude
    int sq_lat = (int)(lat_rem / 1.0);  // 0-9
    int sq_lon = (int)(lon_rem / 2.0);  // 0-9

    // Remainder after square
    double lat_rem2 = fmod(lat_rem, 1.0);
    double lon_rem2 = fmod(lon_rem, 2.0);

    // Subsquare: 2.5 minutes latitude, 5 minutes longitude
    int subsq_lat = (int)(lat_rem2 * 24.0);  // 0-23
    int subsq_lon = (int)(lon_rem2 * 12.0);  // 0-23

    // Build 6-character locator: AA11aa
    snprintf(result->locator, sizeof(result->locator), "%c%c%d%d%c%c",
            field_lon_char, field_lat_char,
            sq_lon, sq_lat,
            'a' + subsq_lon, 'a' + subsq_lat);

    // Store center of grid
    // Field center
    result->lon = -180.0 + (field_lon + 0.5) * 20.0;
    result->lat = -90.0 + (field_lat + 0.5) * 10.0;
    // Square center offset
    result->lon += (sq_lon + 0.5) * 2.0;
    result->lat += (sq_lat + 0.5) * 1.0;
    // Subsquare center offset
    result->lon += (subsq_lon + 0.5) * (5.0 / 60.0);
    result->lat += (subsq_lat + 0.5) * (2.5 / 60.0);

    return 0;
}

int maidenhead_to_latlon(const char *locator, maidenhead_t *result) {
    if (!result || !locator) return -1;

    int len = strlen(locator);
    if (len != 4 && len != 6) return -1;  // Must be 4 or 6 characters

    // Extract field characters
    char field_lon = toupper(locator[0]);
    char field_lat = toupper(locator[1]);

    if (field_lon < 'A' || field_lon > 'R' || field_lat < 'A' || field_lat > 'R') {
        return -1;  // Invalid field letters (must be A-R)
    }

    int field_lon_idx = field_lon - 'A';
    int field_lat_idx = field_lat - 'A';

    // Extract square digits
    if (!isdigit(locator[2]) || !isdigit(locator[3])) return -1;

    int sq_lon = locator[2] - '0';
    int sq_lat = locator[3] - '0';

    if (sq_lon > 9 || sq_lat > 9) return -1;

    double lon = -180.0 + field_lon_idx * 20.0 + sq_lon * 2.0;
    double lat = -90.0 + field_lat_idx * 10.0 + sq_lat * 1.0;

    // If 6-character format, extract subsquare
    if (len == 6) {
        char subsq_lon = tolower(locator[4]);
        char subsq_lat = tolower(locator[5]);

        if (subsq_lon < 'a' || subsq_lon > 'x' || subsq_lat < 'a' || subsq_lat > 'x') {
            return -1;  // Invalid subsquare letters
        }

        int subsq_lon_idx = subsq_lon - 'a';
        int subsq_lat_idx = subsq_lat - 'a';

        lon += subsq_lon_idx * (5.0 / 60.0);
        lat += subsq_lat_idx * (2.5 / 60.0);

        // Add half-subsquare for center
        lon += 2.5 / 60.0;
        lat += 1.25 / 60.0;
    } else {
        // 4-character format: center of square
        lon += 1.0;
        lat += 0.5;
    }

    result->lat = lat;
    result->lon = lon;
    strncpy(result->locator, locator, 6);
    result->locator[6] = '\0';

    return 0;
}

int maidenhead_is_valid(const char *locator) {
    if (!locator) return 0;

    int len = strlen(locator);
    if (len != 4 && len != 6) return 0;

    // Check field letters (A-R for 18 fields)
    if (locator[0] < 'A' || locator[0] > 'R') return 0;
    if (locator[1] < 'A' || locator[1] > 'R') return 0;

    // Check square digits (0-9)
    if (!isdigit(locator[2]) || !isdigit(locator[3])) return 0;

    // Check subsquare if present
    if (len == 6) {
        char c4 = tolower(locator[4]);
        char c5 = tolower(locator[5]);
        if (c4 < 'a' || c4 > 'x') return 0;
        if (c5 < 'a' || c5 > 'x') return 0;
    }

    return 1;
}

int maidenhead_bounding_box(double lat1, double lon1, double lat2, double lon2,
                           maidenhead_t *result) {
    if (!result) return -1;

    // Get grid square for both points
    maidenhead_t grid1, grid2;
    if (maidenhead_from_latlon(lat1, lon1, &grid1) != 0) return -1;
    if (maidenhead_from_latlon(lat2, lon2, &grid2) != 0) return -1;

    // Use 4-character format for bounding box (larger area)
    char locator[5];
    strncpy(locator, grid1.locator, 4);
    locator[4] = '\0';

    // If both points in different squares, find bounding field
    if (strncmp(grid1.locator, grid2.locator, 4) != 0) {
        // Use first 2 chars (field) as bounding box
        locator[0] = grid1.locator[0];
        locator[1] = grid1.locator[1];
        locator[2] = '\0';
    }

    return maidenhead_to_latlon(locator, result);
}
