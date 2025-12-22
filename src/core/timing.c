#include "timing.h"
#include "log.h"
#include <stdlib.h>
#include <unistd.h>

#define MAX_TASKS 32

static scheduled_task_t *g_tasks[MAX_TASKS];
static int g_task_count = 0;

int timing_init(void) {
    g_task_count = 0;
    log_info("Timing system initialized");
    return 0;
}

int timing_register_task(scheduled_task_t *task) {
    if (!task || g_task_count >= MAX_TASKS) return -1;

    task->next_update = time(NULL) + task->interval;
    g_tasks[g_task_count++] = task;

    log_info("Registered task: %s (interval: %ld sec)", task->name, task->interval);
    return 0;
}

scheduled_task_t *timing_get_next_task(void) {
    time_t now = time(NULL);
    scheduled_task_t *next = NULL;
    time_t next_time = now + 86400;  // Start with tomorrow

    for (int i = 0; i < g_task_count; i++) {
        scheduled_task_t *task = g_tasks[i];

        // Check if due or event-triggered
        if (task->event_triggered || task->next_update <= now) {
            return task;  // Task is due now
        }

        // Track earliest future task
        if (task->next_update < next_time) {
            next = task;
            next_time = task->next_update;
        }
    }

    return next;
}

void timing_task_updated(scheduled_task_t *task) {
    if (task) {
        task->next_update = time(NULL) + task->interval;
        task->event_triggered = 0;
        log_debug("Task updated: %s (next: %ld)", task->name, task->next_update);
    }
}

int timing_sleep_until_next(void) {
    int seconds = timing_seconds_to_next_task();
    if (seconds > 0) {
        sleep(seconds);
    }
    return 0;
}

int timing_seconds_to_next_task(void) {
    scheduled_task_t *next = timing_get_next_task();
    if (!next) return 60;  // Default 1 minute

    time_t now = time(NULL);
    time_t diff = next->next_update - now;
    return (diff > 0) ? (int)diff : 0;
}
