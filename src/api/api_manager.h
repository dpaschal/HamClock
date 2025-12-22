#ifndef HAMCLOCK_API_MANAGER_H
#define HAMCLOCK_API_MANAGER_H

// API Manager - Centralized scheduler for all API calls
// Phase 2 implementation

int api_manager_init(void);
void api_manager_deinit(void);

// Register APIs and start scheduling
int api_manager_start(void);

// Stop API scheduling
void api_manager_stop(void);

#endif // HAMCLOCK_API_MANAGER_H
