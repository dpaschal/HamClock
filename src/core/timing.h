#ifndef HAMCLOCK_TIMING_H
#define HAMCLOCK_TIMING_H

#include <time.h>

typedef struct {
    const char *name;
    time_t interval;        // Update interval in seconds
    time_t next_update;     // Next scheduled update time
    int event_triggered;    // Set to 1 to force immediate update
} scheduled_task_t;

// Initialize timing system
int timing_init(void);

// Register a scheduled task
int timing_register_task(scheduled_task_t *task);

// Get next task that needs updating
scheduled_task_t *timing_get_next_task(void);

// Mark task as updated
void timing_task_updated(scheduled_task_t *task);

// Sleep until next task is due
int timing_sleep_until_next(void);

// Get seconds until next task
int timing_seconds_to_next_task(void);

#endif // HAMCLOCK_TIMING_H
