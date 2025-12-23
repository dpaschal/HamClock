#include "earthmap.h"
#include "../core/log.h"
#include <math.h>
#include <string.h>
#include <SDL2/SDL_image.h>

#define PI 3.14159265358979323846
#define DEG2RAD(d) ((d) * PI / 180.0)
#define RAD2DEG(r) ((r) * 180.0 / PI)

// Color scheme - improved aesthetics
static const SDL_Color COLOR_OCEAN = {70, 130, 180, 255};     // Steel blue (more visible)
static const SDL_Color COLOR_LAND = {107, 142, 70, 255};      // Olive green (muted, realistic)
static const SDL_Color COLOR_GRID = {100, 120, 160, 255};     // Cool gray-blue
static const SDL_Color COLOR_GREYLINE = {210, 170, 120, 200}; // Tan/twilight (warmer)
static const SDL_Color COLOR_DAYSIDE = {255, 255, 150, 40};   // Warm yellow (subtle)
static const SDL_Color COLOR_NIGHTSIDE = {60, 70, 120, 100};  // Deep blue (less harsh)
static const SDL_Color COLOR_OBSERVER = {100, 200, 100, 255}; // Softer green

int earthmap_init(earthmap_ctx_t *ctx, SDL_Renderer *renderer,
                 int width, int height) {
    if (!ctx || !renderer) return -1;

    ctx->renderer = renderer;
    ctx->width = width;
    ctx->height = height;
    ctx->offset_x = 0;     // Initialize offsets (can be set later)
    ctx->offset_y = 0;
    ctx->projection = PROJ_MERCATOR;
    ctx->greyline_mode = GREYLINE_FUZZY;
    ctx->show_grid = 1;
    ctx->show_daylight = 1;
    ctx->center_latitude = 0;
    ctx->center_longitude = 0;
    ctx->zoom = 1.0;

    log_info("Earthmap initialized: %dx%d with Mercator projection at offset (%d, %d)",
             width, height, ctx->offset_x, ctx->offset_y);
    return 0;
}

void earthmap_deinit(earthmap_ctx_t *ctx) {
    if (ctx) {
        log_info("Earthmap deinitialized");
    }
}

// Mercator projection: converts lat/lon to screen coordinates
// Web Mercator: x = lon, y = log(tan(lat))
static void mercator_project(double lat, double lon, double *proj_x, double *proj_y) {
    // Normalized to 0-1 range
    *proj_x = (lon + 180.0) / 360.0;  // -180 to 180 -> 0 to 1

    // Mercator: y = log(tan(pi/4 + lat/2)) / pi
    // Clamp latitude to avoid singularity at poles
    if (lat > 85.0511) lat = 85.0511;
    if (lat < -85.0511) lat = -85.0511;

    double lat_rad = DEG2RAD(lat);
    *proj_y = log(tan(PI/4.0 + lat_rad/2.0)) / PI;
    *proj_y = (1.0 - (*proj_y)) / 2.0;  // Flip Y (screen coords)
}

int earthmap_latlon_to_screen(earthmap_ctx_t *ctx, double lat, double lon,
                             int *screen_x, int *screen_y) {
    if (!ctx) return 0;

    double proj_x, proj_y;
    mercator_project(lat, lon, &proj_x, &proj_y);

    // Account for center offset
    double center_proj_x, center_proj_y;
    mercator_project(ctx->center_latitude, ctx->center_longitude,
                     &center_proj_x, &center_proj_y);

    // Apply zoom and centering
    double map_x = (proj_x - center_proj_x) * ctx->width / ctx->zoom + ctx->width / 2;
    double map_y = (proj_y - center_proj_y) * ctx->height / ctx->zoom + ctx->height / 2;

    // Apply screen position offset
    *screen_x = (int)map_x + ctx->offset_x;
    *screen_y = (int)map_y + ctx->offset_y;

    // Return 1 if on-screen (within map bounds, not absolute screen bounds)
    return ((int)map_x >= 0 && (int)map_x < ctx->width &&
            (int)map_y >= 0 && (int)map_y < ctx->height);
}

int earthmap_screen_to_latlon(earthmap_ctx_t *ctx, int screen_x, int screen_y,
                             double *lat, double *lon) {
    if (!ctx) return -1;

    // Remove screen position offset first
    int map_x = screen_x - ctx->offset_x;
    int map_y = screen_y - ctx->offset_y;

    // Reverse Mercator projection
    // Get center in projected coords
    double center_proj_x, center_proj_y;
    mercator_project(ctx->center_latitude, ctx->center_longitude,
                     &center_proj_x, &center_proj_y);

    // Convert screen to projection coords
    double proj_x = (map_x - ctx->width/2.0) * ctx->zoom / ctx->width + center_proj_x;
    double proj_y = (map_y - ctx->height/2.0) * ctx->zoom / ctx->height + center_proj_y;

    // Reverse Mercator
    *lon = proj_x * 360.0 - 180.0;

    // y = log(tan(pi/4 + lat/2)) / pi
    // lat = 2 * atan(exp(y * pi)) - pi/2
    double y = (1.0 - proj_y) * 2.0 - 1.0;
    *lat = RAD2DEG(2.0 * atan(exp(y * PI)) - PI/2.0);

    return 0;
}

void earthmap_render_base(earthmap_ctx_t *ctx) {
    if (!ctx || !ctx->renderer) return;

    // Draw ocean background
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_OCEAN.r, COLOR_OCEAN.g, COLOR_OCEAN.b, COLOR_OCEAN.a);
    SDL_Rect ocean = {ctx->offset_x, ctx->offset_y, ctx->width, ctx->height};
    SDL_RenderFillRect(ctx->renderer, &ocean);

    // Draw world map using proper lat/lon to screen coordinates
    SDL_SetRenderDrawColor(ctx->renderer, 107, 142, 70, 255);  // Olive green for land

    // Continent bounds: (lat_min, lat_max, lon_min, lon_max)
    struct { double lat_min, lat_max, lon_min, lon_max; } continents[] = {
        {25.0, 50.0, -130.0, -65.0},      // North America
        {-56.0, 12.0, -82.0, -35.0},      // South America
        {35.0, 71.0, -11.0, 41.0},        // Europe
        {-35.0, 37.0, -18.0, 52.0},       // Africa
        {15.0, 77.0, 26.0, 180.0},        // Asia
        {-47.0, -10.0, 113.0, 155.0},     // Australia
        {60.0, 84.0, -73.0, -11.0},       // Greenland
    };

    // Draw each continent
    for (int i = 0; i < 7; i++) {
        int x1, y1, x2, y2;
        earthmap_latlon_to_screen(ctx, continents[i].lat_min, continents[i].lon_min, &x1, &y1);
        earthmap_latlon_to_screen(ctx, continents[i].lat_max, continents[i].lon_max, &x2, &y2);

        SDL_Rect rect;
        rect.x = (x1 < x2) ? x1 : x2;
        rect.y = (y1 < y2) ? y1 : y2;
        rect.w = (x1 > x2) ? (x1 - x2) : (x2 - x1);
        rect.h = (y1 > y2) ? (y1 - y2) : (y2 - y1);

        if (rect.w > 0 && rect.h > 0) {
            SDL_SetRenderDrawColor(ctx->renderer, 107, 142, 70, 255);  // Green
            SDL_RenderFillRect(ctx->renderer, &rect);
            SDL_SetRenderDrawColor(ctx->renderer, 40, 80, 60, 255);    // Dark outline
            SDL_RenderDrawRect(ctx->renderer, &rect);
        }
    }
}

void earthmap_render_grid(earthmap_ctx_t *ctx) {
    if (!ctx || !ctx->renderer || !ctx->show_grid) return;

    SDL_SetRenderDrawColor(ctx->renderer, COLOR_GRID.r, COLOR_GRID.g,
                          COLOR_GRID.b, COLOR_GRID.a);

    // Latitude grid lines (every 15 degrees)
    for (int lat = -75; lat <= 75; lat += 15) {
        int prev_x = -999, prev_y = -999;
        for (int lon = -180; lon <= 180; lon += 5) {
            int x, y;
            if (earthmap_latlon_to_screen(ctx, (double)lat, (double)lon, &x, &y)) {
                if (prev_x != -999) {
                    SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
                }
                prev_x = x;
                prev_y = y;
            }
        }
    }

    // Longitude grid lines (every 15 degrees)
    for (int lon = -180; lon < 180; lon += 15) {
        int prev_x = -999, prev_y = -999;
        for (int lat = -85; lat <= 85; lat += 2) {
            int x, y;
            if (earthmap_latlon_to_screen(ctx, (double)lat, (double)lon, &x, &y)) {
                if (prev_x != -999) {
                    SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
                }
                prev_x = x;
                prev_y = y;
            }
        }
    }

    // Prime meridian (special line)
    SDL_SetRenderDrawColor(ctx->renderer, 128, 128, 160, 200);
    int prev_x = -999, prev_y = -999;
    for (int lat = -85; lat <= 85; lat += 2) {
        int x, y;
        if (earthmap_latlon_to_screen(ctx, (double)lat, 0.0, &x, &y)) {
            if (prev_x != -999) {
                SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
            }
            prev_x = x;
            prev_y = y;
        }
    }

    // Equator (special line)
    prev_x = -999;
    prev_y = -999;
    for (int lon = -180; lon <= 180; lon += 5) {
        int x, y;
        if (earthmap_latlon_to_screen(ctx, 0.0, (double)lon, &x, &y)) {
            if (prev_x != -999) {
                SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
            }
            prev_x = x;
            prev_y = y;
        }
    }
}

void earthmap_render_greyline(earthmap_ctx_t *ctx, sun_position_t *sun) {
    if (!ctx || !ctx->renderer || !sun) return;

    // Greyline is the twilight zone - where the sun is at the horizon
    // At terminator (day/night boundary), sun elevation angle = -0.833° (including refraction)
    // Simplified: greyline occurs where cos(hour_angle) = -tan(lat) * tan(declination)

    // Draw greyline as a curved line
    // For each longitude, calculate where sun is at horizon

    int prev_x = -999, prev_y = -999;

    for (int lon = -180; lon <= 180; lon += 2) {
        // At this longitude, find the latitude where terminator occurs
        // Hour angle = 0 at subsolar point (sun directly overhead)
        // Hour angle = 90 degrees at day/night boundary

        // Simplified terminator calculation:
        // Subsolar point is at (sun->subsolar_lat, sun->subsolar_lon)
        // Terminator is perpendicular to sun-earth line
        // Distance from subsolar point along terminator = 90 degrees

        double lon_diff = lon - sun->subsolar_lon;

        // Normalize to -180 to 180
        while (lon_diff > 180) lon_diff -= 360;
        while (lon_diff < -180) lon_diff += 360;

        // At terminator, the hour angle is 90 degrees
        // Approximate: terminator latitude varies with cos(hour_angle / 90)
        // But simplified: use straight line approximation

        double hour_angle = lon_diff;
        if (hour_angle > 90) hour_angle = 180 - hour_angle;
        if (hour_angle < -90) hour_angle = -180 - hour_angle;

        // Terminator latitude (simplified)
        double term_lat = sun->subsolar_lat;
        double lat_offset = sqrt(1.0 - (hour_angle / 90.0) * (hour_angle / 90.0)) * 90.0;

        // Draw both day/night boundary lines
        // Day side
        int x, y;
        if (earthmap_latlon_to_screen(ctx, term_lat + lat_offset, (double)lon, &x, &y)) {
            SDL_SetRenderDrawColor(ctx->renderer, COLOR_GREYLINE.r, COLOR_GREYLINE.g,
                                  COLOR_GREYLINE.b, 255);
            if (prev_x != -999) {
                SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
            }
            prev_x = x;
            prev_y = y;
        }
    }

    // Draw night side boundary
    prev_x = -999;
    prev_y = -999;
    for (int lon = -180; lon <= 180; lon += 2) {
        double lon_diff = lon - sun->subsolar_lon;
        while (lon_diff > 180) lon_diff -= 360;
        while (lon_diff < -180) lon_diff += 360;

        double hour_angle = lon_diff;
        if (hour_angle > 90) hour_angle = 180 - hour_angle;
        if (hour_angle < -90) hour_angle = -180 - hour_angle;

        double term_lat = sun->subsolar_lat;
        double lat_offset = sqrt(1.0 - (hour_angle / 90.0) * (hour_angle / 90.0)) * 90.0;

        int x, y;
        if (earthmap_latlon_to_screen(ctx, term_lat - lat_offset, (double)lon, &x, &y)) {
            SDL_SetRenderDrawColor(ctx->renderer, COLOR_GREYLINE.r, COLOR_GREYLINE.g,
                                  COLOR_GREYLINE.b, 255);
            if (prev_x != -999) {
                SDL_RenderDrawLine(ctx->renderer, prev_x, prev_y, x, y);
            }
            prev_x = x;
            prev_y = y;
        }
    }

    // Mark subsolar point
    int sub_x, sub_y;
    if (earthmap_latlon_to_screen(ctx, sun->subsolar_lat, sun->subsolar_lon,
                                 &sub_x, &sub_y)) {
        SDL_SetRenderDrawColor(ctx->renderer, 255, 255, 0, 255);  // Yellow for sun
        SDL_Rect sun_marker = {sub_x - 4, sub_y - 4, 8, 8};
        SDL_RenderFillRect(ctx->renderer, &sun_marker);
    }
}

void earthmap_render_observer(earthmap_ctx_t *ctx, observer_t *observer) {
    if (!ctx || !ctx->renderer || !observer) return;

    int x, y;
    if (earthmap_latlon_to_screen(ctx, observer->latitude, observer->longitude, &x, &y)) {
        // Draw observer as green circle with crosshairs
        SDL_SetRenderDrawColor(ctx->renderer, COLOR_OBSERVER.r, COLOR_OBSERVER.g,
                              COLOR_OBSERVER.b, COLOR_OBSERVER.a);

        // Draw circle (8 points)
        int radius = 5;
        for (int i = 0; i < 8; i++) {
            double angle1 = 2.0 * PI * i / 8.0;
            double angle2 = 2.0 * PI * (i + 1) / 8.0;
            int x1 = x + (int)(radius * cos(angle1));
            int y1 = y + (int)(radius * sin(angle1));
            int x2 = x + (int)(radius * cos(angle2));
            int y2 = y + (int)(radius * sin(angle2));
            SDL_RenderDrawLine(ctx->renderer, x1, y1, x2, y2);
        }

        // Crosshairs
        SDL_RenderDrawLine(ctx->renderer, x - 10, y, x + 10, y);
        SDL_RenderDrawLine(ctx->renderer, x, y - 10, x, y + 10);
    }
}

void earthmap_set_projection(earthmap_ctx_t *ctx, map_projection_t proj) {
    if (ctx) {
        ctx->projection = proj;
        log_info("Map projection changed to %d", proj);
    }
}

void earthmap_set_greyline_mode(earthmap_ctx_t *ctx, greyline_mode_t mode) {
    if (ctx) {
        ctx->greyline_mode = mode;
    }
}

void earthmap_pan(earthmap_ctx_t *ctx, int dx, int dy) {
    if (!ctx) return;

    // Convert pixel movement to lat/lon movement
    double lat_per_pixel = 180.0 / ctx->height * ctx->zoom;
    double lon_per_pixel = 360.0 / ctx->width * ctx->zoom;

    ctx->center_latitude += dy * lat_per_pixel;
    ctx->center_longitude += dx * lon_per_pixel;

    // Clamp latitude
    if (ctx->center_latitude > 85.0511) ctx->center_latitude = 85.0511;
    if (ctx->center_latitude < -85.0511) ctx->center_latitude = -85.0511;

    // Wrap longitude
    while (ctx->center_longitude > 180) ctx->center_longitude -= 360;
    while (ctx->center_longitude < -180) ctx->center_longitude += 360;
}

void earthmap_zoom(earthmap_ctx_t *ctx, double factor) {
    if (!ctx) return;

    ctx->zoom *= factor;

    // Clamp zoom
    if (ctx->zoom < 0.5) ctx->zoom = 0.5;
    if (ctx->zoom > 4.0) ctx->zoom = 4.0;
}
