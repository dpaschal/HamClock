#include "earthmap.h"
#include "../core/log.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846
#define DEG2RAD(d) ((d) * PI / 180.0)
#define RAD2DEG(r) ((r) * 180.0 / PI)

// Color scheme
static const SDL_Color COLOR_OCEAN = {20, 60, 120, 255};      // Dark blue
static const SDL_Color COLOR_LAND = {34, 139, 34, 255};       // Forest green
static const SDL_Color COLOR_GRID = {64, 64, 96, 255};        // Light grid
static const SDL_Color COLOR_GREYLINE = {200, 150, 100, 200}; // Tan/twilight
static const SDL_Color COLOR_DAYSIDE = {255, 255, 100, 30};   // Light yellow
static const SDL_Color COLOR_NIGHTSIDE = {50, 50, 100, 100};  // Dark blue
static const SDL_Color COLOR_OBSERVER = {0, 255, 0, 255};     // Bright green

int earthmap_init(earthmap_ctx_t *ctx, SDL_Renderer *renderer,
                 int width, int height) {
    if (!ctx || !renderer) return -1;

    ctx->renderer = renderer;
    ctx->width = width;
    ctx->height = height;
    ctx->projection = PROJ_MERCATOR;
    ctx->greyline_mode = GREYLINE_FUZZY;
    ctx->show_grid = 1;
    ctx->show_daylight = 1;
    ctx->center_latitude = 0;
    ctx->center_longitude = 0;
    ctx->zoom = 1.0;

    log_info("Earthmap initialized: %dx%d with Mercator projection", width, height);
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

    *screen_x = (int)map_x;
    *screen_y = (int)map_y;

    // Return 1 if on-screen
    return (*screen_x >= 0 && *screen_x < ctx->width &&
            *screen_y >= 0 && *screen_y < ctx->height);
}

int earthmap_screen_to_latlon(earthmap_ctx_t *ctx, int screen_x, int screen_y,
                             double *lat, double *lon) {
    if (!ctx) return -1;

    // Reverse Mercator projection
    // Get center in projected coords
    double center_proj_x, center_proj_y;
    mercator_project(ctx->center_latitude, ctx->center_longitude,
                     &center_proj_x, &center_proj_y);

    // Convert screen to projection coords
    double proj_x = (screen_x - ctx->width/2.0) * ctx->zoom / ctx->width + center_proj_x;
    double proj_y = (screen_y - ctx->height/2.0) * ctx->zoom / ctx->height + center_proj_y;

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

    // Fill background (ocean)
    SDL_SetRenderDrawColor(ctx->renderer, COLOR_OCEAN.r, COLOR_OCEAN.g,
                          COLOR_OCEAN.b, COLOR_OCEAN.a);
    SDL_Rect full = {0, 0, ctx->width, ctx->height};
    SDL_RenderFillRect(ctx->renderer, &full);

    // Draw simplified continents as filled regions
    // This is a minimal coastline representation
    // For production, would use proper map data (Natural Earth, GSHHG, etc.)

    // Continental regions (approximate lat/lon bounds)
    typedef struct {
        double lat_min, lat_max;
        double lon_min, lon_max;
    } continent_t;

    continent_t continents[] = {
        // North America
        {25.0, 50.0, -130.0, -65.0},
        // South America
        {-56.0, 13.0, -82.0, -35.0},
        // Europe
        {35.0, 71.0, -11.0, 41.0},
        // Africa
        {-35.0, 37.0, -18.0, 52.0},
        // Asia
        {-10.0, 77.0, 26.0, 180.0},
        // Australia
        {-44.0, -10.0, 113.0, 155.0},
        // Greenland
        {60.0, 84.0, -73.0, -11.0},
    };

    SDL_SetRenderDrawColor(ctx->renderer, COLOR_LAND.r, COLOR_LAND.g,
                          COLOR_LAND.b, COLOR_LAND.a);

    for (size_t i = 0; i < sizeof(continents) / sizeof(continent_t); i++) {
        continent_t *cont = &continents[i];

        int x1, y1, x2, y2;
        earthmap_latlon_to_screen(ctx, cont->lat_min, cont->lon_min, &x1, &y1);
        earthmap_latlon_to_screen(ctx, cont->lat_max, cont->lon_max, &x2, &y2);

        // Draw as filled rectangle
        int w = x2 - x1;
        int h = y2 - y1;
        if (w > 0 && h > 0) {
            SDL_Rect continent = {x1, y1, w, h};
            SDL_RenderFillRect(ctx->renderer, &continent);
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
