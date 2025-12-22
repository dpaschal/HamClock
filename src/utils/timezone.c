#include "timezone.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

// Timezone database with UTC offsets
typedef struct {
    timezone_t id;
    const char *name;
    const char *abbrev;
    int offset_hours;
    int offset_minutes;
    // DST info simplified (real implementation would use zoneinfo)
    int has_dst;
} tz_info_t;

static const tz_info_t timezone_db[] = {
    {TZ_UTC,   "Coordinated Universal Time", "UTC", 0, 0, 0},
    {TZ_CET,   "Central European Time", "CET", 1, 0, 1},
    {TZ_CEST,  "Central European Summer Time", "CEST", 2, 0, 0},
    {TZ_EST,   "Eastern Standard Time", "EST", -5, 0, 1},
    {TZ_EDT,   "Eastern Daylight Time", "EDT", -4, 0, 0},
    {TZ_JST,   "Japan Standard Time", "JST", 9, 0, 0},
    {TZ_AEST,  "Australian Eastern Standard Time", "AEST", 10, 0, 1},
    {TZ_NZST,  "New Zealand Standard Time", "NZST", 12, 0, 1},
};

#define NUM_TIMEZONES (sizeof(timezone_db) / sizeof(tz_info_t))

// Determine if date is in daylight saving time (Northern Hemisphere)
// Simplified: DST is last Sunday of March to last Sunday of October
static int is_daylight_saving_nh(int year, int month, int day, int wday) {
    if (month < 3 || month > 10) return 0;  // No DST
    if (month > 3 && month < 10) return 1;  // Always DST

    // March: Last Sunday
    if (month == 3) {
        int last_sunday = 31;
        while ((last_sunday - day + 3 + wday) % 7 != 0 && last_sunday > day) {
            last_sunday--;
        }
        return day >= last_sunday ? 1 : 0;
    }

    // October: Last Sunday
    if (month == 10) {
        int last_sunday = 31;
        while ((last_sunday - day + 3 + wday) % 7 != 0 && last_sunday > day) {
            last_sunday--;
        }
        return day < last_sunday ? 1 : 0;
    }

    return 0;
}

// Determine if date is in daylight saving time (Southern Hemisphere)
// Simplified: DST is last Sunday of October to last Sunday of April
static int is_daylight_saving_sh(int year, int month, int day, int wday) {
    if (month < 10 && month > 4) return 0;  // No DST
    if (month > 10 || month < 4) return 1;  // Always DST

    // October: Last Sunday
    if (month == 10) {
        int last_sunday = 31;
        while ((last_sunday - day + 3 + wday) % 7 != 0 && last_sunday > day) {
            last_sunday--;
        }
        return day >= last_sunday ? 1 : 0;
    }

    // April: Last Sunday
    if (month == 4) {
        int last_sunday = 30;
        while ((last_sunday - day + 3 + wday) % 7 != 0 && last_sunday > day) {
            last_sunday--;
        }
        return day < last_sunday ? 1 : 0;
    }

    return 0;
}

int timezone_convert(time_t timestamp, timezone_t tz, local_time_t *out) {
    if (!out) return -1;

    struct tm *gmt = gmtime(&timestamp);
    if (!gmt) return -1;

    // Find timezone info
    const tz_info_t *tzinfo = NULL;
    if (tz == TZ_LOCAL) {
        // Use system local time
        struct tm *local = localtime(&timestamp);
        if (!local) return -1;

        out->year = local->tm_year + 1900;
        out->month = local->tm_mon + 1;
        out->day = local->tm_mday;
        out->hour = local->tm_hour;
        out->minute = local->tm_min;
        out->second = local->tm_sec;
        out->wday = local->tm_wday;
        out->is_dst = local->tm_isdst;
        strcpy(out->tz_name, local->tm_zone ? local->tm_zone : "Local");
        out->offset_hours = 0;
        out->offset_minutes = 0;
        return 0;
    }

    for (size_t i = 0; i < NUM_TIMEZONES; i++) {
        if (timezone_db[i].id == tz) {
            tzinfo = &timezone_db[i];
            break;
        }
    }

    if (!tzinfo) return -1;

    // Copy base info
    out->year = gmt->tm_year + 1900;
    out->month = gmt->tm_mon + 1;
    out->day = gmt->tm_mday;
    out->hour = gmt->tm_hour;
    out->minute = gmt->tm_min;
    out->second = gmt->tm_sec;
    out->wday = gmt->tm_wday;
    out->offset_hours = tzinfo->offset_hours;
    out->offset_minutes = tzinfo->offset_minutes;

    // Check DST for timezones that have it
    out->is_dst = 0;
    if (tzinfo->has_dst) {
        if (tz == TZ_CET || tz == TZ_EST || tz == TZ_EDT) {
            // Northern Hemisphere DST
            out->is_dst = is_daylight_saving_nh(out->year, out->month, out->day, out->wday);
            if (out->is_dst && tzinfo->offset_hours < 2) {
                out->offset_hours++;  // Add 1 hour for DST
            }
        } else if (tz == TZ_AEST || tz == TZ_NZST) {
            // Southern Hemisphere DST
            out->is_dst = is_daylight_saving_sh(out->year, out->month, out->day, out->wday);
            if (out->is_dst && tzinfo->offset_hours < 13) {
                out->offset_hours++;  // Add 1 hour for DST
            }
        }
    }

    // Apply offset
    int total_seconds = out->hour * 3600 + out->minute * 60 + out->second;
    total_seconds += out->offset_hours * 3600 + out->offset_minutes * 60;

    // Handle day overflow/underflow
    if (total_seconds < 0) {
        total_seconds += 86400;
        out->day--;
        if (out->day < 1) {
            out->month--;
            if (out->month < 1) {
                out->month = 12;
                out->year--;
            }
            // Days in month
            int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
            if (out->month == 2 && (out->year % 4 == 0 && (out->year % 100 != 0 || out->year % 400 == 0))) {
                out->day = 29;
            } else {
                out->day = days_in_month[out->month];
            }
        }
    } else if (total_seconds >= 86400) {
        total_seconds -= 86400;
        out->day++;
        // Days in month
        int days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int max_days = days_in_month[out->month];
        if (out->month == 2 && (out->year % 4 == 0 && (out->year % 100 != 0 || out->year % 400 == 0))) {
            max_days = 29;
        }
        if (out->day > max_days) {
            out->day = 1;
            out->month++;
            if (out->month > 12) {
                out->month = 1;
                out->year++;
            }
        }
    }

    out->hour = total_seconds / 3600;
    out->minute = (total_seconds % 3600) / 60;
    out->second = total_seconds % 60;

    strcpy(out->tz_name, tzinfo->name);
    return 0;
}

int timezone_get_offset_seconds(timezone_t tz, time_t timestamp) {
    local_time_t local_time;
    if (timezone_convert(timestamp, tz, &local_time) != 0) {
        return 0;
    }
    return local_time.offset_hours * 3600 + local_time.offset_minutes * 60;
}

const char *timezone_get_name(timezone_t tz) {
    for (size_t i = 0; i < NUM_TIMEZONES; i++) {
        if (timezone_db[i].id == tz) {
            return timezone_db[i].name;
        }
    }
    return "Unknown";
}

int timezone_is_dst(timezone_t tz, time_t timestamp) {
    local_time_t local_time;
    if (timezone_convert(timestamp, tz, &local_time) != 0) {
        return 0;
    }
    return local_time.is_dst;
}

void timezone_format_time(local_time_t *time, char *buf, int buflen) {
    if (!time || !buf) return;
    snprintf(buf, buflen, "%02d:%02d:%02d", time->hour, time->minute, time->second);
}

void timezone_format_time_short(local_time_t *time, char *buf, int buflen) {
    if (!time || !buf) return;
    snprintf(buf, buflen, "%02d:%02d", time->hour, time->minute);
}

void timezone_format_datetime(local_time_t *time, char *buf, int buflen) {
    if (!time || !buf) return;
    snprintf(buf, buflen, "%04d-%02d-%02d %02d:%02d:%02d %s",
            time->year, time->month, time->day,
            time->hour, time->minute, time->second,
            timezone_get_abbrev(TZ_UTC));  // Would need to pass tz
}

const char *timezone_get_abbrev(timezone_t tz) {
    for (size_t i = 0; i < NUM_TIMEZONES; i++) {
        if (timezone_db[i].id == tz) {
            return timezone_db[i].abbrev;
        }
    }
    return "???";
}
