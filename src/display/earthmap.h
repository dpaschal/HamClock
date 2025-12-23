#ifndef HAMCLOCK_EARTHMAP_H
#define HAMCLOCK_EARTHMAP_H

#include <SDL2/SDL.h>
#include "../astro/sun.h"

// Map configuration
#define MAP_WIDTH 800
#define MAP_HEIGHT 500

// Map projection types
typedef enum {
    PROJ_MERCATOR,      // Mercator projection (conformal)
    PROJ_AZIMUTHAL,     // Azimuthal equidistant (experimental)
    PROJ_CYLINDRICAL    // Simple cylindrical (for fast rendering)
} map_projection_t;

// Greyline rendering mode
typedef enum {
    GREYLINE_NONE,      // No greyline
    GREYLINE_SOLID,     // Solid band
    GREYLINE_FUZZY,     // Fuzzy twilight zone
    GREYLINE_ANIMATED   // Animated bands
} greyline_mode_t;

// Observer location
typedef struct {
    double latitude;
    double longitude;
    const char *name;      // e.g., "DE0", "K1"
} observer_t;

// Map rendering context
typedef struct {
    SDL_Renderer *renderer;
    int width;
    int height;
    int offset_x;          // Screen X position (where map starts)
    int offset_y;          // Screen Y position (where map starts)
    map_projection_t projection;
    greyline_mode_t greyline_mode;
    int show_grid;         // 1 = show lat/lon grid
    int show_daylight;     // 1 = shade day/night areas
    int center_latitude;   // Center of map projection
    int center_longitude;
    double zoom;           // 1.0 = full world
} earthmap_ctx_t;

// Initialize map rendering context
int earthmap_init(earthmap_ctx_t *ctx, SDL_Renderer *renderer,
                 int width, int height);

// Cleanup map context
void earthmap_deinit(earthmap_ctx_t *ctx);

// Render base map (land/water, simplified)
void earthmap_render_base(earthmap_ctx_t *ctx);

// Render latitude/longitude grid
void earthmap_render_grid(earthmap_ctx_t *ctx);

// Render greyline (day/night terminator)
// Requires sun position data from Phase 4
void earthmap_render_greyline(earthmap_ctx_t *ctx, sun_position_t *sun);

// Render observer location marker
void earthmap_render_observer(earthmap_ctx_t *ctx, observer_t *observer);

// Convert latitude/longitude to screen coordinates
// Returns 1 if point is on-screen, 0 if off-screen
int earthmap_latlon_to_screen(earthmap_ctx_t *ctx, double lat, double lon,
                             int *screen_x, int *screen_y);

// Convert screen coordinates back to latitude/longitude
int earthmap_screen_to_latlon(earthmap_ctx_t *ctx, int screen_x, int screen_y,
                             double *lat, double *lon);

// Set map projection
void earthmap_set_projection(earthmap_ctx_t *ctx, map_projection_t proj);

// Set greyline rendering mode
void earthmap_set_greyline_mode(earthmap_ctx_t *ctx, greyline_mode_t mode);

// Pan map (shift center)
void earthmap_pan(earthmap_ctx_t *ctx, int dx, int dy);

// Zoom map in/out
void earthmap_zoom(earthmap_ctx_t *ctx, double factor);

#endif // HAMCLOCK_EARTHMAP_H
