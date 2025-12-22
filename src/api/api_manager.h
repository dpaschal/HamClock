#ifndef HAMCLOCK_API_MANAGER_H
#define HAMCLOCK_API_MANAGER_H

// API Manager - Centralized scheduler for all API calls with optimization
//
// Key optimization: Consolidate APIs to reduce daily calls from 500+ to ~150-200
//
// Update intervals (optimized):
// - NOAA (Kp+Flux+SSN): 1 hour (was 3 separate 57-62min calls)
// - X-Ray: 10 minutes
// - Solar Wind: 10 minutes
// - VOACAP: 2 hours (event-triggered on Kp change >10%)
// - DRAP: 15 minutes (was 5)
// - SDO Images: 30 minutes (with ETag checking)
// - Weather: 30 minutes
// - TLE: 6 hours

int api_manager_init(void);
void api_manager_deinit(void);

// Register APIs and start scheduling
int api_manager_start(void);

// Stop API scheduling
void api_manager_stop(void);

// Get next task that needs updating (or NULL if none due)
void *api_manager_get_next_task(void);

// Update task and reschedule
int api_manager_update_task(void *task);

#endif // HAMCLOCK_API_MANAGER_H
