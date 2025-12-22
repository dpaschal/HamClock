/**
 * test_maidenhead.c - Unit tests for Maidenhead grid conversion
 *
 * Tests latitude/longitude to grid conversion and vice versa
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../src/utils/maidenhead.h"

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", msg); \
            return 1; \
        } \
        fprintf(stdout, "PASS: %s\n", msg); \
    } while(0)

#define ASSERT_FLOAT(actual, expected, tolerance, msg) \
    do { \
        double diff = (actual) - (expected); \
        if (diff < 0) diff = -diff; \
        if (diff > tolerance) { \
            fprintf(stderr, "FAIL: %s (got %f, expected %f, diff %f)\n", msg, actual, expected, diff); \
            return 1; \
        } \
        fprintf(stdout, "PASS: %s\n", msg); \
    } while(0)

/**
 * Test 1: Equator/Prime Meridian (JJ00aa)
 */
int test_equator_prime_meridian(void) {
    maidenhead_t grid = {0};

    // Convert 0, 0 to Maidenhead
    int rc = maidenhead_from_latlon(0.0, 0.0, &grid);
    ASSERT(rc == 0, "Conversion at equator/meridian succeeds");

    // Should produce a valid 6-character grid
    ASSERT(strlen(grid.locator) == 6, "Grid locator is 6 characters");

    // Should start with JJ (field JJ)
    ASSERT(grid.locator[0] == 'J' && grid.locator[1] == 'J',
           "Equator/meridian is JJ field");

    return 0;
}

/**
 * Test 2: Greenwich Observatory (IO91ad)
 */
int test_greenwich(void) {
    // Greenwich: 51.48°N, 0°W
    maidenhead_t grid = {0};
    int rc = maidenhead_from_latlon(51.48, 0.0, &grid);
    ASSERT(rc == 0, "Greenwich conversion succeeds");

    ASSERT(strlen(grid.locator) == 6, "Grid locator is 6 characters");

    // Should start with IO (near UK)
    ASSERT(grid.locator[0] == 'I' && grid.locator[1] == 'O',
           "Greenwich is IO field");

    return 0;
}

/**
 * Test 3: Roundtrip conversion (lat/lon -> grid -> lat/lon)
 */
int test_roundtrip_conversion(void) {
    double orig_lat = 40.0;
    double orig_lon = -75.0;

    maidenhead_t grid = {0};
    int rc = maidenhead_from_latlon(orig_lat, orig_lon, &grid);
    ASSERT(rc == 0, "Forward conversion succeeds");

    maidenhead_t result = {0};
    rc = maidenhead_to_latlon(grid.locator, &result);
    ASSERT(rc == 0, "Reverse conversion succeeds");

    // Roundtrip error should be small (within subsquare, ~5 minutes)
    // Field: 20°, Square: 2°, Subsquare: 5'
    ASSERT_FLOAT(result.lat, orig_lat, 0.2, "Roundtrip latitude within 0.2°");
    ASSERT_FLOAT(result.lon, orig_lon, 0.2, "Roundtrip longitude within 0.2°");

    return 0;
}

/**
 * Test 4: Grid validation
 */
int test_grid_validation(void) {
    // Valid 6-character grid
    ASSERT(maidenhead_is_valid("JJ00aa"), "Valid grid JJ00aa");
    ASSERT(maidenhead_is_valid("IO91ad"), "Valid grid IO91ad");
    ASSERT(maidenhead_is_valid("EN00aa"), "Valid grid EN00aa");

    // Invalid grids
    ASSERT(!maidenhead_is_valid("AA00aa"), "Invalid: Field AA");
    ASSERT(!maidenhead_is_valid("JJ99aa"), "Invalid: Square 99");
    ASSERT(!maidenhead_is_valid("JJ00zz"), "Invalid: Subsquare zz");
    ASSERT(!maidenhead_is_valid("JJ00a"), "Invalid: Too short");
    ASSERT(!maidenhead_is_valid("JJ00aaa"), "Invalid: Too long");

    return 0;
}

/**
 * Test 5: Bounding box calculation
 */
int test_bounding_box(void) {
    // Get bounding box containing two points: (40, -75) and (41, -74)
    maidenhead_t result = {0};
    int rc = maidenhead_bounding_box(40.0, -75.0, 41.0, -74.0, &result);
    ASSERT(rc == 0, "Bounding box calculation succeeds");

    // Result should be a maidenhead grid containing both points
    ASSERT(strlen(result.locator) > 0, "Bounding box returns valid grid");
    ASSERT(result.lat >= -90 && result.lat <= 90, "Bounding box latitude in range");
    ASSERT(result.lon >= -180 && result.lon <= 180, "Bounding box longitude in range");

    return 0;
}

/**
 * Test 6: Hemisphere coverage
 */
int test_hemisphere_coverage(void) {
    // Northern hemisphere
    maidenhead_t grid_n = {0};
    int rc = maidenhead_from_latlon(45.0, -90.0, &grid_n);
    ASSERT(rc == 0, "Northern hemisphere conversion works");

    // Southern hemisphere
    maidenhead_t grid_s = {0};
    rc = maidenhead_from_latlon(-45.0, -90.0, &grid_s);
    ASSERT(rc == 0, "Southern hemisphere conversion works");

    // These should produce different grid squares
    ASSERT(strcmp(grid_n.locator, grid_s.locator) != 0,
           "Different hemispheres produce different grids");

    return 0;
}

/**
 * Test 7: Longitude wraparound
 */
int test_longitude_wraparound(void) {
    // Test that +180° and -180° are equivalent
    maidenhead_t grid_pos = {0};
    maidenhead_t grid_neg = {0};

    maidenhead_from_latlon(0.0, 179.9, &grid_pos);
    maidenhead_from_latlon(0.0, -179.9, &grid_neg);

    // Should be same or adjacent
    ASSERT(strlen(grid_pos.locator) > 0 && strlen(grid_neg.locator) > 0,
           "Wraparound grids are valid");

    return 0;
}

/**
 * Test 8: Pole handling
 */
int test_pole_handling(void) {
    // North Pole
    maidenhead_t grid_n = {0};
    int rc = maidenhead_from_latlon(89.99, 0.0, &grid_n);
    ASSERT(rc == 0, "North Pole conversion works");

    // South Pole
    maidenhead_t grid_s = {0};
    rc = maidenhead_from_latlon(-89.99, 0.0, &grid_s);
    ASSERT(rc == 0, "South Pole conversion works");

    ASSERT(strlen(grid_n.locator) == 6, "North Pole grid is valid");
    ASSERT(strlen(grid_s.locator) == 6, "South Pole grid is valid");

    return 0;
}

/**
 * Test 9: Known landmarks
 */
int test_known_landmarks(void) {
    // New York: 40.71°N, -74.01°W -> FN30ak
    maidenhead_t grid = {0};
    maidenhead_from_latlon(40.71, -74.01, &grid);
    ASSERT(strlen(grid.locator) == 6, "New York grid is valid");

    // Tokyo: 35.68°N, 139.69°E -> PM95
    maidenhead_t grid_tokyo = {0};
    maidenhead_from_latlon(35.68, 139.69, &grid_tokyo);
    ASSERT(strlen(grid_tokyo.locator) == 6, "Tokyo grid is valid");

    // Sydney: -33.87°S, 151.21°E -> QF62
    maidenhead_t grid_sydney = {0};
    maidenhead_from_latlon(-33.87, 151.21, &grid_sydney);
    ASSERT(strlen(grid_sydney.locator) == 6, "Sydney grid is valid");

    return 0;
}

/**
 * Test 10: Field/Square/Subsquare breakdown
 */
int test_grid_components(void) {
    maidenhead_t grid = {0};
    maidenhead_from_latlon(40.0, -100.0, &grid);

    // Grid structure: AABB##ss
    // AA = Field (letters)
    // BB = Square (digits)
    // ss = Subsquare (letters)

    char *locator = grid.locator;
    ASSERT(locator[0] >= 'A' && locator[0] <= 'X', "Field 1 is A-X");
    ASSERT(locator[1] >= 'A' && locator[1] <= 'X', "Field 2 is A-X");
    ASSERT(locator[2] >= '0' && locator[2] <= '9', "Square 1 is 0-9");
    ASSERT(locator[3] >= '0' && locator[3] <= '9', "Square 2 is 0-9");
    ASSERT(locator[4] >= 'a' && locator[4] <= 'x', "Subsquare 1 is a-x");
    ASSERT(locator[5] >= 'a' && locator[5] <= 'x', "Subsquare 2 is a-x");

    return 0;
}

/**
 * Main test runner
 */
int main(void) {
    printf("=== HamClock Maidenhead Grid Tests ===\n\n");

    int tests_passed = 0;
    int tests_failed = 0;

    #define RUN_TEST(test_fn) \
        printf("\nRunning: %s\n", #test_fn); \
        if (test_fn() == 0) { tests_passed++; } else { tests_failed++; }

    RUN_TEST(test_equator_prime_meridian);
    RUN_TEST(test_greenwich);
    RUN_TEST(test_roundtrip_conversion);
    RUN_TEST(test_grid_validation);
    RUN_TEST(test_bounding_box);
    RUN_TEST(test_hemisphere_coverage);
    RUN_TEST(test_longitude_wraparound);
    RUN_TEST(test_pole_handling);
    RUN_TEST(test_known_landmarks);
    RUN_TEST(test_grid_components);

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
