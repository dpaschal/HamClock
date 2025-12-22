#include "api_manager.h"
#include "noaa.h"
#include "../core/log.h"
#include "../core/timing.h"
#include "../core/state.h"
#include <stdlib.h>
#include <string.h>

// Task IDs
#define TASK_NOAA_SPACE_WEATHER 0
#define MAX_API_TASKS 16

typedef struct {
    int id;
    const char *name;
    time_t interval;
    int (*fetch_func)(void);  // Function to call for this task
} api_task_t;

static api_task_t g_tasks[MAX_API_TASKS];
static int g_task_count = 0;
static int g_manager_running = 0;

// NOAA fetch wrapper that returns 0 (success) or -1 (failure)
static int fetch_noaa_wrapper(void) {
    noaa_data_t data;
    int rc = noaa_fetch_space_weather(&data);
    if (rc == 0) {
        // Update application state
        space_weather_t sw = {
            .kp_index = data.kp_index,
            .a_index = data.a_index,
            .solar_flux = data.solar_flux,
            .sunspot_number = data.sunspot_number,
            .timestamp = data.timestamp
        };
        strcpy(sw.xray_flux, data.xray_flux);
        state_set_space_weather(&sw);
    }
    return rc;
}

int api_manager_init(void) {
    g_task_count = 0;
    g_manager_running = 0;

    // Register NOAA task (consolidated API: Kp+Flux+SSN in ONE call)
    // Update every 1 hour (NOAA updates 3x/day, we check hourly)
    g_tasks[g_task_count].id = TASK_NOAA_SPACE_WEATHER;
    g_tasks[g_task_count].name = "NOAA Space Weather";
    g_tasks[g_task_count].interval = 60 * 60;  // 1 hour
    g_tasks[g_task_count].fetch_func = fetch_noaa_wrapper;
    g_task_count++;

    // TODO: Phase 2+ features
    // - X-Ray flux (10 min)
    // - Solar wind (10 min)
    // - VOACAP (2 hours, event-triggered)
    // - DRAP (15 min)
    // - SDO images (30 min)
    // - Weather (30 min)
    // - TLE data (6 hours)

    log_info("API Manager initialized with %d tasks", g_task_count);
    return 0;
}

void api_manager_deinit(void) {
    api_manager_stop();
    log_info("API Manager deinitialized");
}

int api_manager_start(void) {
    if (g_manager_running) {
        log_warn("API Manager already running");
        return 0;
    }

    // Register all tasks with timing system
    scheduled_task_t *sched_tasks = malloc(sizeof(scheduled_task_t) * g_task_count);
    if (!sched_tasks) {
        log_error("Failed to allocate task array");
        return -1;
    }

    for (int i = 0; i < g_task_count; i++) {
        sched_tasks[i].name = g_tasks[i].name;
        sched_tasks[i].interval = g_tasks[i].interval;
        sched_tasks[i].event_triggered = 0;

        if (timing_register_task(&sched_tasks[i]) != 0) {
            log_error("Failed to register task: %s", g_tasks[i].name);
            free(sched_tasks);
            return -1;
        }
    }

    g_manager_running = 1;
    log_info("API Manager started - %d tasks scheduled", g_task_count);

    return 0;
}

void api_manager_stop(void) {
    if (g_manager_running) {
        g_manager_running = 0;
        log_info("API Manager stopped");
    }
}

void *api_manager_get_next_task(void) {
    if (!g_manager_running) return NULL;

    // This would typically be called from the main loop
    // to get the next task that needs updating
    // For now, return NULL (Phase 2 placeholder for main loop integration)
    return NULL;
}

int api_manager_update_task(void *task) {
    if (!task) return -1;

    // Execute the appropriate API fetch based on task type
    // This will be fully integrated in Phase 2 when main loop is updated

    return 0;
}
