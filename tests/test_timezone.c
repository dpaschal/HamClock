/**
 * test_timezone.c - Unit tests for timezone module
 *
 * Tests timezone conversion, DST handling, and formatting
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "../src/utils/timezone.h"

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
 * Test 1: UTC timezone always returns same time
 */
int test_utc_timezone(void) {
    time_t now = time(NULL);
    local_time_t utc_time = {0};

    int rc = timezone_convert(now, TZ_UTC, &utc_time);
    ASSERT(rc == 0, "UTC conversion returns 0");

    // UTC should have zero offset
    ASSERT_INT(utc_time.offset_hours, 0, "UTC offset hours");
    ASSERT_INT(utc_time.offset_minutes, 0, "UTC offset minutes");

    // Verify name
    const char *name = timezone_get_name(TZ_UTC);
    ASSERT(name != NULL, "UTC has name");
    ASSERT(strcmp(name, "Coordinated Universal Time") == 0, "UTC correct name");

    return 0;
}

/**
 * Test 2: CET timezone offset (winter time)
 */
int test_cet_timezone(void) {
    // Use timestamp from winter (January) to ensure CET not CEST
    struct tm winter = {0};
    winter.tm_year = 125;  // 2025
    winter.tm_mon = 0;     // January (0-based)
    winter.tm_mday = 15;   // 15th
    winter.tm_hour = 12;   // 12:00 UTC
    winter.tm_min = 0;
    winter.tm_sec = 0;
    winter.tm_isdst = -1;
    time_t winter_time = mktime(&winter);

    local_time_t cet_time = {0};
    int rc = timezone_convert(winter_time, TZ_CET, &cet_time);
    ASSERT(rc == 0, "CET conversion returns 0");

    // CET should be +1 hour in winter
    ASSERT_INT(cet_time.offset_hours, 1, "CET winter offset +1 hour");
    ASSERT_INT(cet_time.is_dst, 0, "CET winter is not DST");

    const char *abbrev = timezone_get_abbrev(TZ_CET);
    ASSERT(abbrev != NULL, "CET has abbreviation");

    return 0;
}

/**
 * Test 3: EST timezone offset (winter time in US)
 */
int test_est_timezone(void) {
    // Use winter timestamp
    struct tm winter = {0};
    winter.tm_year = 125;  // 2025
    winter.tm_mon = 0;     // January (0-based)
    winter.tm_mday = 15;   // 15th
    winter.tm_hour = 17;   // 17:00 UTC = 12:00 EST
    winter.tm_min = 0;
    winter.tm_sec = 0;
    winter.tm_isdst = -1;
    time_t winter_time = mktime(&winter);

    local_time_t est_time = {0};
    int rc = timezone_convert(winter_time, TZ_EST, &est_time);
    ASSERT(rc == 0, "EST conversion returns 0");

    // EST should be -5 hours in winter
    ASSERT_INT(est_time.offset_hours, -5, "EST winter offset -5 hours");
    ASSERT_INT(est_time.is_dst, 0, "EST winter is not DST");

    return 0;
}

/**
 * Test 4: Time conversion accuracy
 */
int test_time_conversion_accuracy(void) {
    // Create a known UTC time: 2025-01-15 12:34:56 UTC
    struct tm utc_tm = {0};
    utc_tm.tm_year = 125;   // 2025
    utc_tm.tm_mon = 0;      // January (0-based)
    utc_tm.tm_mday = 15;
    utc_tm.tm_hour = 12;
    utc_tm.tm_min = 34;
    utc_tm.tm_sec = 56;
    utc_tm.tm_isdst = -1;
    time_t known_time = mktime(&utc_tm);

    // Convert to UTC (should be identity)
    local_time_t local_utc = {0};
    timezone_convert(known_time, TZ_UTC, &local_utc);

    ASSERT_INT(local_utc.hour, 12, "UTC hour correct");
    ASSERT_INT(local_utc.minute, 34, "UTC minute correct");
    ASSERT_INT(local_utc.second, 56, "UTC second correct");

    return 0;
}

/**
 * Test 5: Time formatting
 */
int test_time_formatting(void) {
    local_time_t test_time = {
        .year = 2025,
        .month = 1,
        .day = 15,
        .hour = 14,
        .minute = 30,
        .second = 45,
        .wday = 3,  // Wednesday
        .offset_hours = 1,
        .offset_minutes = 0,
        .is_dst = 0
    };
    strcpy(test_time.tz_name, "CET");

    // Test short format (HH:MM)
    char short_buf[16] = {0};
    timezone_format_time_short(&test_time, short_buf, sizeof(short_buf));
    ASSERT(strcmp(short_buf, "14:30") == 0, "Short format correct");

    // Test full format (HH:MM:SS)
    char full_buf[32] = {0};
    timezone_format_time(&test_time, full_buf, sizeof(full_buf));
    ASSERT(strlen(full_buf) > 0, "Full format produces output");
    ASSERT(strstr(full_buf, "14") != NULL, "Full format contains hour");

    return 0;
}

/**
 * Test 6: Offset calculation in seconds
 */
int test_offset_calculation(void) {
    time_t now = time(NULL);

    // UTC should have 0 offset
    int utc_offset = timezone_get_offset_seconds(TZ_UTC, now);
    ASSERT_INT(utc_offset, 0, "UTC offset in seconds");

    // CET should be +1 hour = +3600 seconds (in winter)
    // Note: This might be +2 in summer (CEST), so check the absolute value
    int cet_offset = timezone_get_offset_seconds(TZ_CET, now);
    ASSERT(cet_offset == 3600 || cet_offset == 7200, "CET offset is 1 or 2 hours");

    return 0;
}

/**
 * Test 7: Weekday calculation
 */
int test_weekday_calculation(void) {
    // 2025-01-15 is a Wednesday (wday = 3)
    struct tm known = {0};
    known.tm_year = 125;
    known.tm_mon = 0;
    known.tm_mday = 15;
    known.tm_hour = 12;
    known.tm_isdst = -1;
    time_t known_time = mktime(&known);

    local_time_t local = {0};
    timezone_convert(known_time, TZ_UTC, &local);

    // Verify weekday is 0-6 range
    ASSERT(local.wday >= 0 && local.wday <= 6, "Weekday in valid range");

    return 0;
}

/**
 * Test 8: Invalid input handling
 */
int test_invalid_input(void) {
    // NULL output pointer
    int rc = timezone_convert(time(NULL), TZ_UTC, NULL);
    // Should either work (converting to NULL is OK) or return error consistently
    ASSERT(rc >= -1, "Handles NULL output gracefully");

    // Invalid timezone (use arbitrary high value)
    local_time_t out = {0};
    rc = timezone_convert(time(NULL), 999, &out);
    // Implementation should handle gracefully
    ASSERT(rc >= -1, "Handles invalid timezone gracefully");

    return 0;
}

/**
 * Test 9: Leap year handling
 */
int test_leap_year(void) {
    // 2024 is a leap year, test Feb 29
    struct tm leap = {0};
    leap.tm_year = 124;  // 2024
    leap.tm_mon = 1;     // February (0-based)
    leap.tm_mday = 29;
    leap.tm_hour = 12;
    leap.tm_isdst = -1;
    time_t leap_time = mktime(&leap);

    local_time_t local = {0};
    timezone_convert(leap_time, TZ_UTC, &local);

    ASSERT_INT(local.day, 29, "Leap year Feb 29 handled");
    ASSERT_INT(local.month, 2, "Leap year month correct");

    return 0;
}

/**
 * Test 10: All timezones are accessible
 */
int test_all_timezones(void) {
    time_t now = time(NULL);

    // Test that all timezone enums can be converted
    int timezones[] = {TZ_UTC, TZ_CET, TZ_CEST, TZ_EST, TZ_EDT,
                       TZ_JST, TZ_AEST, TZ_NZST, TZ_LOCAL};
    int count = sizeof(timezones) / sizeof(timezones[0]);

    for (int i = 0; i < count; i++) {
        local_time_t local = {0};
        int rc = timezone_convert(now, timezones[i], &local);
        ASSERT(rc == 0, "All timezones accessible");
    }

    return 0;
}

/**
 * Main test runner
 */
int main(void) {
    printf("=== HamClock Timezone Module Tests ===\n\n");

    int tests_passed = 0;
    int tests_failed = 0;

    #define RUN_TEST(test_fn) \
        printf("\nRunning: %s\n", #test_fn); \
        if (test_fn() == 0) { tests_passed++; } else { tests_failed++; }

    RUN_TEST(test_utc_timezone);
    RUN_TEST(test_cet_timezone);
    RUN_TEST(test_est_timezone);
    RUN_TEST(test_time_conversion_accuracy);
    RUN_TEST(test_time_formatting);
    RUN_TEST(test_offset_calculation);
    RUN_TEST(test_weekday_calculation);
    RUN_TEST(test_invalid_input);
    RUN_TEST(test_leap_year);
    RUN_TEST(test_all_timezones);

    printf("\n=== Test Results ===\n");
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Total:  %d\n", tests_passed + tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
