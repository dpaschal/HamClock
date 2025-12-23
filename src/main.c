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
#include "display/earthmap.h"
#include "display/clocks.h"
#include "astro/sun.h"
#include "astro/moon.h"
#include "utils/maidenhead.h"
#include "utils/timezone.h"

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

    // Initialize world map with greyline (Phase 5)
    earthmap_ctx_t map_ctx = {0};
    observer_t observer = {0.0, 0.0, "Observer"};  // Greenwich at equator for demo

    if (earthmap_init(&map_ctx, render_ctx.renderer, 800, 500) != 0) {
        log_warn("Failed to initialize earthmap");
    } else {
        // Set the screen position for the map (top-left corner at 10, 60)
        map_ctx.offset_x = 10;
        map_ctx.offset_y = 60;
        log_info("Earthmap positioned at (%d, %d)", map_ctx.offset_x, map_ctx.offset_y);
    }

    // Initialize clock panel (Phase 6)
    clock_panel_t clock_panel = {0};
    SDL_Color white = {255, 255, 255, 255};
    SDL_Color cyan = {0, 200, 255, 255};
    SDL_Color yellow = {255, 255, 100, 255};

    if (clocks_panel_init(&clock_panel, 4, 820, 70, 190, 380) != 0) {
        log_warn("Failed to initialize clock panel");
    } else {
        // Add clocks for different timezones
        clocks_add(&clock_panel, "UTC", TZ_UTC, white, cyan);
        clocks_add(&clock_panel, "DE (CET)", TZ_CET, white, yellow);
        clocks_add(&clock_panel, "US (EST)", TZ_EST, white, yellow);
        clocks_add(&clock_panel, "Local", TZ_LOCAL, white, cyan);
    }

    log_info("Display initialized - rendering loop starting");

    // Main event loop with rendering
    time_t now = time(NULL);
    while (!g_shutdown && render_ctx.running) {
        // Handle input events
        if (!renderer_handle_events(&render_ctx)) {
            break;
        }

        // Update astronomical data (Phase 4)
        now = time(NULL);
        sun_position_t sun_pos = {0};
        moon_position_t moon_pos = {0};

        // Calculate sun position (observer at 0,0 for now - can be customized)
        sun_calculate_position(now, 0.0, 0.0, &sun_pos);
        sun_pos.season = sun_get_season(now);
        sun_pos.season_name = sun_get_season_name(sun_pos.season);
        sun_pos.solstice_equinox = sun_get_next_solstice_equinox(now);

        // Calculate moon position
        moon_calculate_position(now, &moon_pos);

        // Prepare rendering data
        render_sun_data_t render_sun = {
            .sun_declination = sun_pos.declination,
            .sun_eot = sun_pos.equation_of_time,
            .sun_sunrise = sun_pos.sunrise_time,
            .sun_sunset = sun_pos.sunset_time,
            .sun_subsolar_lat = sun_pos.subsolar_lat,
            .sun_subsolar_lon = sun_pos.subsolar_lon,
            .sun_is_daylight = sun_pos.is_daylight
        };

        render_moon_data_t render_moon = {
            .moon_illumination = moon_pos.illumination,
            .moon_age = moon_pos.age,
            .moon_phase_name = moon_pos.phase_name
        };

        // Clear screen first
        renderer_clear(&render_ctx);

        // Render title bar with improved styling
        renderer_fill_rect(&render_ctx, 0, 0, render_ctx.width, 50,
                          (SDL_Color){50, 60, 80, 255});  // Slightly better gradient
        if (fonts.font_large) {
            renderer_draw_text(&render_ctx, fonts.font_large, "HamClock v1.0.0", 15, 8,
                              (SDL_Color){120, 200, 255, 255},  // Softer cyan text
                              (SDL_Color){50, 60, 80, 255});
        }

        // Render world map with greyline (Phase 5)
        SDL_Rect map_rect = {10, 60, 800, 500};
        earthmap_render_base(&map_ctx);
        earthmap_render_grid(&map_ctx);
        earthmap_render_greyline(&map_ctx, &sun_pos);
        earthmap_render_observer(&map_ctx, &observer);

        // Draw map border
        SDL_SetRenderDrawColor(render_ctx.renderer, 100, 100, 100, 255);
        SDL_RenderDrawRect(render_ctx.renderer, &map_rect);

        // Render clock panel (Phase 6)
        clocks_update(&clock_panel, now);
        clocks_render(&clock_panel, render_ctx.renderer, fonts.font_large,
                     fonts.font_normal, fonts.font_small);

        // Render space weather data (right side, below clocks)
        space_weather_t *sw = state_get_space_weather();
        if (sw && fonts.font_normal) {
            int sw_x = 820;
            int sw_y = 470;
            int sw_width = 190;
            int sw_height = 180;

            // Draw border around space weather panel
            SDL_SetRenderDrawColor(render_ctx.renderer, 80, 100, 130, 255);
            SDL_Rect sw_border = {sw_x, sw_y, sw_width, sw_height};
            SDL_RenderDrawRect(render_ctx.renderer, &sw_border);

            // Title: "Space Weather"
            renderer_draw_text(&render_ctx, fonts.font_normal, "Space Weather",
                              sw_x + 5, sw_y + 5,
                              (SDL_Color){120, 200, 255, 255},  // Cyan
                              (SDL_Color){40, 45, 55, 255});

            // Kp Index with color coding
            SDL_Color kp_color = (SDL_Color){255, 200, 80, 255};  // Default: yellow
            if (sw->kp_index < 3) {
                kp_color = (SDL_Color){120, 200, 120, 255};  // Green: quiet
            } else if (sw->kp_index < 5) {
                kp_color = (SDL_Color){255, 255, 120, 255};  // Yellow: unsettled
            } else if (sw->kp_index < 7) {
                kp_color = (SDL_Color){255, 200, 80, 255};   // Orange: active
            } else if (sw->kp_index < 9) {
                kp_color = (SDL_Color){255, 120, 80, 255};   // Red-orange: severe
            } else {
                kp_color = (SDL_Color){255, 80, 80, 255};    // Red: extreme
            }

            char kp_text[32];
            snprintf(kp_text, sizeof(kp_text), "Kp: %.1f", sw->kp_index);
            renderer_draw_text(&render_ctx, fonts.font_small, kp_text,
                              sw_x + 10, sw_y + 35,
                              kp_color, (SDL_Color){40, 45, 55, 255});

            // Solar Flux
            SDL_Color flux_color = (SDL_Color){255, 200, 80, 255};  // Default: yellow
            if (sw->solar_flux < 70) {
                flux_color = (SDL_Color){120, 200, 120, 255};  // Green: low
            } else if (sw->solar_flux < 100) {
                flux_color = (SDL_Color){255, 255, 120, 255};  // Yellow: moderate
            } else if (sw->solar_flux < 150) {
                flux_color = (SDL_Color){255, 200, 80, 255};   // Orange: high
            } else if (sw->solar_flux < 200) {
                flux_color = (SDL_Color){255, 120, 80, 255};   // Red-orange: very high
            } else {
                flux_color = (SDL_Color){255, 80, 80, 255};    // Red: extreme
            }

            char flux_text[32];
            snprintf(flux_text, sizeof(flux_text), "Flux: %.0f", sw->solar_flux);
            renderer_draw_text(&render_ctx, fonts.font_small, flux_text,
                              sw_x + 10, sw_y + 55,
                              flux_color, (SDL_Color){40, 45, 55, 255});
        }

        // Present the frame
        renderer_present(&render_ctx);

        // Limit frame rate
        renderer_limit_frame_rate(&render_ctx);

        // TODO: Phase 5+ - Additional widgets (clocks, plots)
        // TODO: Phase 6 - Advanced features (DX cluster, satellites)
    }

    // Cleanup
    log_info("Shutting down...");

    clocks_panel_deinit(&clock_panel);
    earthmap_deinit(&map_ctx);
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
