/**
 * test_sun.c - Unit tests for sun position and season calculations
 *
 * Tests solar declination, equation of time, seasons, and solstice/equinox dates
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../src/astro/sun.h"

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "FAIL: %s\n", msg); \
            return 1; \
        } \
        fprintf(stdout, "PASS: %s\n", msg); \
    } while(0)

#define ASSERT_INT(actual, expected, msg) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "FAIL: %s (got %d, expected %d)\n", msg, actual, expected); \
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
 * Test 1: Vernal Equinox (Spring, ~Mar 20)
 */
int test_vernal_equinox(void) {
    // March 20, 2025
    struct tm eq_tm = {0};
    eq_tm.tm_year = 125;  // 2025
    eq_tm.tm_mon = 2;     // March (0-based)
    eq_tm.tm_mday = 20;
    eq_tm.tm_hour = 12;
    eq_tm.tm_isdst = -1;
    time_t eq_time = mktime(&eq_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(eq_time, 0.0, 0.0, &sun_pos);

    // At vernal equinox, declination should be close to 0°
    ASSERT_FLOAT(sun_pos.declination, 0.0, 2.0, "Vernal equinox declination near 0°");

    sun_pos.season = sun_get_season(eq_time);
    ASSERT_INT(sun_pos.season, SEASON_SPRING, "Vernal equinox is spring");

    return 0;
}

/**
 * Test 2: Summer Solstice (June, ~Jun 20)
 */
int test_summer_solstice(void) {
    // June 20, 2025
    struct tm sol_tm = {0};
    sol_tm.tm_year = 125;  // 2025
    sol_tm.tm_mon = 5;     // June (0-based)
    sol_tm.tm_mday = 20;
    sol_tm.tm_hour = 12;
    sol_tm.tm_isdst = -1;
    time_t sol_time = mktime(&sol_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(sol_time, 0.0, 0.0, &sun_pos);

    // At summer solstice, declination should be ~23.44°
    ASSERT_FLOAT(sun_pos.declination, 23.44, 2.0, "Summer solstice declination ~23.44°");

    sun_pos.season = sun_get_season(sol_time);
    ASSERT_INT(sun_pos.season, SEASON_SUMMER, "Summer solstice is summer");

    return 0;
}

/**
 * Test 3: Autumnal Equinox (September, ~Sep 22)
 */
int test_autumnal_equinox(void) {
    // September 22, 2025
    struct tm eq_tm = {0};
    eq_tm.tm_year = 125;  // 2025
    eq_tm.tm_mon = 8;     // September (0-based)
    eq_tm.tm_mday = 22;
    eq_tm.tm_hour = 12;
    eq_tm.tm_isdst = -1;
    time_t eq_time = mktime(&eq_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(eq_time, 0.0, 0.0, &sun_pos);

    // At autumnal equinox, declination should be close to 0°
    ASSERT_FLOAT(sun_pos.declination, 0.0, 2.0, "Autumnal equinox declination near 0°");

    sun_pos.season = sun_get_season(eq_time);
    ASSERT_INT(sun_pos.season, SEASON_AUTUMN, "Autumnal equinox is autumn");

    return 0;
}

/**
 * Test 4: Winter Solstice (December, ~Dec 21)
 */
int test_winter_solstice(void) {
    // December 21, 2025
    struct tm sol_tm = {0};
    sol_tm.tm_year = 125;  // 2025
    sol_tm.tm_mon = 11;    // December (0-based)
    sol_tm.tm_mday = 21;
    sol_tm.tm_hour = 12;
    sol_tm.tm_isdst = -1;
    time_t sol_time = mktime(&sol_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(sol_time, 0.0, 0.0, &sun_pos);

    // At winter solstice, declination should be ~-23.44°
    ASSERT_FLOAT(sun_pos.declination, -23.44, 2.0, "Winter solstice declination ~-23.44°");

    sun_pos.season = sun_get_season(sol_time);
    ASSERT_INT(sun_pos.season, SEASON_WINTER, "Winter solstice is winter");

    return 0;
}

/**
 * Test 5: Equation of Time accuracy
 */
int test_equation_of_time(void) {
    // January 3 - when EoT is maximum (~14 minutes)
    struct tm jan3 = {0};
    jan3.tm_year = 125;  // 2025
    jan3.tm_mon = 0;     // January (0-based)
    jan3.tm_mday = 3;
    jan3.tm_hour = 12;
    jan3.tm_isdst = -1;
    time_t jan3_time = mktime(&jan3);

    sun_position_t sun_pos = {0};
    sun_calculate_position(jan3_time, 0.0, 0.0, &sun_pos);

    // EoT should be around 3-4 minutes on Jan 3
    ASSERT_FLOAT(sun_pos.equation_of_time, 3.0, 2.0, "EoT in valid range at Jan 3");

    return 0;
}

/**
 * Test 6: Declination range (-23.44 to +23.44)
 */
int test_declination_range(void) {
    // Test throughout the year
    for (int month = 0; month < 12; month++) {
        struct tm tm = {0};
        tm.tm_year = 125;
        tm.tm_mon = month;
        tm.tm_mday = 15;  // Mid-month
        tm.tm_hour = 12;
        tm.tm_isdst = -1;
        time_t time_val = mktime(&tm);

        sun_position_t sun_pos = {0};
        sun_calculate_position(time_val, 0.0, 0.0, &sun_pos);

        double abs_decl = sun_pos.declination;
        if (abs_decl < 0) abs_decl = -abs_decl;

        ASSERT(abs_decl <= 23.5, "Declination within ±23.44°");
    }
    return 0;
}

/**
 * Test 7: Season name strings
 */
int test_season_names(void) {
    const char *spring = sun_get_season_name(SEASON_SPRING);
    const char *summer = sun_get_season_name(SEASON_SUMMER);
    const char *autumn = sun_get_season_name(SEASON_AUTUMN);
    const char *winter = sun_get_season_name(SEASON_WINTER);

    ASSERT(spring != NULL && strlen(spring) > 0, "Spring has name");
    ASSERT(summer != NULL && strlen(summer) > 0, "Summer has name");
    ASSERT(autumn != NULL && strlen(autumn) > 0, "Autumn has name");
    ASSERT(winter != NULL && strlen(winter) > 0, "Winter has name");

    return 0;
}

/**
 * Test 8: Sunrise/Sunset calculation
 */
int test_sunrise_sunset(void) {
    // June 21 - longest day
    struct tm sol_tm = {0};
    sol_tm.tm_year = 125;
    sol_tm.tm_mon = 5;     // June
    sol_tm.tm_mday = 21;
    sol_tm.tm_hour = 12;
    sol_tm.tm_isdst = -1;
    time_t sol_time = mktime(&sol_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(sol_time, 40.0, 0.0, &sun_pos);  // 40°N latitude

    // Sunrise time should be reasonable (4-6 AM at 40°N in June)
    ASSERT(sun_pos.sunrise_time > 4 && sun_pos.sunrise_time < 6,
           "Sunrise time reasonable in summer");

    // Sunset time should be reasonable (8-10 PM at 40°N in June)
    ASSERT(sun_pos.sunset_time > 20 && sun_pos.sunset_time < 22,
           "Sunset time reasonable in summer");

    return 0;
}

/**
 * Test 9: Subsolar point (subsolar_lat and subsolar_lon)
 */
int test_subsolar_point(void) {
    // At vernal equinox, subsolar point is at equator
    struct tm eq_tm = {0};
    eq_tm.tm_year = 125;
    eq_tm.tm_mon = 2;     // March
    eq_tm.tm_mday = 20;
    eq_tm.tm_hour = 12;
    eq_tm.tm_isdst = -1;
    time_t eq_time = mktime(&eq_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(eq_time, 0.0, 0.0, &sun_pos);

    // Subsolar latitude should be near equator (~0°)
    ASSERT_FLOAT(sun_pos.subsolar_lat, 0.0, 5.0, "Subsolar lat near equator at equinox");

    // Subsolar longitude should be in valid range (-180 to 180)
    ASSERT(sun_pos.subsolar_lon >= -180 && sun_pos.subsolar_lon <= 180,
           "Subsolar lon in valid range");

    return 0;
}

/**
 * Test 10: Is daylight flag
 */
int test_daylight_flag(void) {
    // At equator at noon, should be daylight
    struct tm noon_tm = {0};
    noon_tm.tm_year = 125;
    noon_tm.tm_mon = 5;     // June
    noon_tm.tm_mday = 21;
    noon_tm.tm_hour = 12;
    noon_tm.tm_isdst = -1;
    time_t noon_time = mktime(&noon_tm);

    sun_position_t sun_pos = {0};
    sun_calculate_position(noon_time, 0.0, 0.0, &sun_pos);

    ASSERT(sun_pos.is_daylight, "Is daylight at equator at noon");

    // At midnight, should not be daylight
    struct tm midnight_tm = {0};
    midnight_tm.tm_year = 125;
    midnight_tm.tm_mon = 5;     // June
    midnight_tm.tm_mday = 21;
    midnight_tm.tm_hour = 0;
    midnight_tm.tm_isdst = -1;
    time_t midnight_time = mktime(&midnight_tm);

    sun_position_t night_pos = {0};
    sun_calculate_position(midnight_time, 0.0, 0.0, &night_pos);

    ASSERT(!night_pos.is_daylight, "Not daylight at midnight");

    return 0;
}

/**
 * Main test runner
 */
int main(void) {
    printf("=== HamClock Sun Position Module Tests ===\n\n");

    int tests_passed = 0;
    int tests_failed = 0;

    #define RUN_TEST(test_fn) \
        printf("\nRunning: %s\n", #test_fn); \
        if (test_fn() == 0) { tests_passed++; } else { tests_failed++; }

    RUN_TEST(test_vernal_equinox);
    RUN_TEST(test_summer_solstice);
    RUN_TEST(test_autumnal_equinox);
    RUN_TEST(test_winter_solstice);
    RUN_TEST(test_equation_of_time);
    RUN_TEST(test_declination_range);
    RUN_TEST(test_season_names);
    RUN_TEST(test_sunrise_sunset);
    RUN_TEST(test_subsolar_point);
    RUN_TEST(test_daylight_flag);

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
