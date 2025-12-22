#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "core/log.h"
#include "core/config.h"
#include "core/state.h"
#include "core/timing.h"
#include "data/database.h"
#include "data/cache.h"
#include "api/http_client.h"
#include "api/api_manager.h"
#include "api/noaa.h"
#include "display/renderer.h"

// Global state for signal handling
static volatile int g_shutdown = 0;

// Signal handler for graceful shutdown
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_info("Received signal %d, shutting down gracefully...", sig);
        g_shutdown = 1;
    }
}

int main(int argc, char *argv[]) {
    int rc = 0;

    // Initialize logging
    if (log_init(NULL) != 0) {
        fprintf(stderr, "Failed to initialize logging\n");
        return 1;
    }

    log_info("========================================");
    log_info("HamClock v1.0.0 Starting");
    log_info("========================================");

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize database
    const char *db_path = "/tmp/hamclock.db";
    if (db_init(db_path) != 0) {
        log_fatal("Failed to initialize database");
        return 1;
    }

    // Initialize configuration
    if (config_init() != 0) {
        log_fatal("Failed to initialize configuration");
        db_deinit();
        return 1;
    }

    // Initialize application state
    if (state_init() != 0) {
        log_fatal("Failed to initialize state");
        db_deinit();
        return 1;
    }

    // Initialize timing system
    if (timing_init() != 0) {
        log_fatal("Failed to initialize timing");
        db_deinit();
        return 1;
    }

    // Initialize HTTP client
    if (http_init() != 0) {
        log_fatal("Failed to initialize HTTP client");
        db_deinit();
        return 1;
    }

    // Initialize API manager
    if (api_manager_init() != 0) {
        log_fatal("Failed to initialize API manager");
        db_deinit();
        return 1;
    }

    // Start API scheduling
    if (api_manager_start() != 0) {
        log_fatal("Failed to start API manager");
        db_deinit();
        return 1;
    }

    log_info("All systems initialized successfully");

    // Initialize display (Phase 3)
    renderer_context_t render_ctx = {0};
    font_set_t fonts = {0};

    if (renderer_init(&render_ctx, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT) != 0) {
        log_warn("Failed to initialize renderer; running in data-only mode");
    } else {
        if (renderer_load_fonts(&fonts) != 0) {
            log_warn("Failed to load fonts; continuing with renderer");
        }
    }

    log_info("Display initialized - rendering loop starting");

    // Main event loop with rendering
    while (!g_shutdown && render_ctx.running) {
        // Handle input events
        if (!renderer_handle_events(&render_ctx)) {
            break;
        }

        // Render current frame with NOAA data
        renderer_render_frame(&render_ctx, &fonts);

        // Limit frame rate
        renderer_limit_frame_rate(&render_ctx);

        // TODO: Phase 4 - Astronomical calculations
        // TODO: Phase 5 - Additional widgets (clocks, plots)
        // TODO: Phase 6 - Advanced features (DX cluster, satellites)
    }

    // Cleanup
    log_info("Shutting down...");

    renderer_unload_fonts(&fonts);
    renderer_deinit(&render_ctx);
    api_manager_deinit();
    http_deinit();
    state_deinit();
    db_deinit();
    log_deinit();

    log_info("========================================");
    log_info("HamClock Shutdown Complete");
    log_info("========================================");

    return rc;
}
