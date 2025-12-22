#ifndef HAMCLOCK_MAIDENHEAD_H
#define HAMCLOCK_MAIDENHEAD_H

// Maidenhead grid square system for ham radio
// 6-character locator: AA11aa
// Field (2 chars): 18x18 grid (20 degree wide)
// Square (2 chars): 10x10 grid within field (2 degree wide)
// Subsquare (2 chars): 24x24 grid within square (5 minute wide)

typedef struct {
    char locator[7];    // 6-character Maidenhead grid string + null term
    double lat;         // Latitude center of grid
    double lon;         // Longitude center of grid
} maidenhead_t;

// Convert latitude/longitude to Maidenhead grid locator (6 characters)
// lat: -90 to 90 (South to North)
// lon: -180 to 180 (West to East)
// Returns 0 on success, -1 on error
int maidenhead_from_latlon(double lat, double lon, maidenhead_t *result);

// Convert Maidenhead grid locator to approximate latitude/longitude
// Returns center of grid square
// Supports 4-char (square) or 6-char (subsquare) format
// Returns 0 on success, -1 on error
int maidenhead_to_latlon(const char *locator, maidenhead_t *result);

// Check if grid square is valid
int maidenhead_is_valid(const char *locator);

// Get grid square containing both points (bounding box)
int maidenhead_bounding_box(double lat1, double lon1, double lat2, double lon2,
                           maidenhead_t *result);

#endif // HAMCLOCK_MAIDENHEAD_H
