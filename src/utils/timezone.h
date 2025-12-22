#ifndef HAMCLOCK_TIMEZONE_H
#define HAMCLOCK_TIMEZONE_H

#include <time.h>

// Common timezone offsets for ham radio
typedef enum {
    TZ_UTC,         // Coordinated Universal Time (Z)
    TZ_CET,         // Central European Time (DE primary) +1
    TZ_CEST,        // Central European Summer Time +2
    TZ_EST,         // Eastern Standard Time (US) -5
    TZ_EDT,         // Eastern Daylight Time (US) -4
    TZ_JST,         // Japan Standard Time +9
    TZ_AEST,        // Australian Eastern Standard Time +10
    TZ_NZST,        // New Zealand Standard Time +12
    TZ_LOCAL,       // System local timezone (auto-detect)
    TZ_CUSTOM       // Custom offset
} timezone_t;

// Time in a specific timezone
typedef struct {
    int year;
    int month;      // 1-12
    int day;        // 1-31
    int hour;       // 0-23
    int minute;     // 0-59
    int second;     // 0-59
    int wday;       // 0-6 (Sunday-Saturday)
    char tz_name[16];      // e.g., "UTC", "CET", "EST"
    int offset_hours;       // Offset from UTC in hours (can be ±13)
    int offset_minutes;     // Additional minutes (rare)
    int is_dst;     // 1 if daylight saving time, 0 if not
} local_time_t;

// Get current time in specific timezone
// timestamp: Unix timestamp (UTC)
// tz: Timezone to convert to
// out: Output local time
int timezone_convert(time_t timestamp, timezone_t tz, local_time_t *out);

// Get timezone offset from UTC in seconds
// Returns positive for east of UTC, negative for west
int timezone_get_offset_seconds(timezone_t tz, time_t timestamp);

// Get human-readable timezone name
const char *timezone_get_name(timezone_t tz);

// Check if timezone is in daylight saving time
int timezone_is_dst(timezone_t tz, time_t timestamp);

// Convert local time to string (HH:MM:SS)
void timezone_format_time(local_time_t *time, char *buf, int buflen);

// Convert local time to short string (HH:MM)
void timezone_format_time_short(local_time_t *time, char *buf, int buflen);

// Format full date/time string
void timezone_format_datetime(local_time_t *time, char *buf, int buflen);

// Get timezone abbreviation (CET, EST, UTC, etc.)
const char *timezone_get_abbrev(timezone_t tz);

#endif // HAMCLOCK_TIMEZONE_H
