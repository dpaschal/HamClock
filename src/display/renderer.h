#ifndef HAMCLOCK_RENDERER_H
#define HAMCLOCK_RENDERER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "earthmap.h"

// Display configuration
#define DEFAULT_WINDOW_WIDTH 1024
#define DEFAULT_WINDOW_HEIGHT 768
#define DEFAULT_WINDOW_TITLE "HamClock v1.0.0"
#define FRAME_RATE 30  // Target 30 FPS

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    int width;
    int height;
    int running;
    int frame_count;
    uint32_t last_frame_time;
} renderer_context_t;

typedef struct {
    TTF_Font *font_large;   // For titles
    TTF_Font *font_normal;  // For labels/values
    TTF_Font *font_small;   // For small text
} font_set_t;

// Initialization and cleanup
int renderer_init(renderer_context_t *ctx, int width, int height);
void renderer_deinit(renderer_context_t *ctx);

// Font management
int renderer_load_fonts(font_set_t *fonts);
void renderer_unload_fonts(font_set_t *fonts);

// Rendering
void renderer_clear(renderer_context_t *ctx);
void renderer_present(renderer_context_t *ctx);
void renderer_limit_frame_rate(renderer_context_t *ctx);

// Text rendering
void renderer_draw_text(renderer_context_t *ctx, TTF_Font *font,
                       const char *text, int x, int y,
                       SDL_Color fg, SDL_Color bg);

// Shape rendering
void renderer_draw_rect(renderer_context_t *ctx, int x, int y, int w, int h, SDL_Color color);
void renderer_fill_rect(renderer_context_t *ctx, int x, int y, int w, int h, SDL_Color color);
void renderer_draw_line(renderer_context_t *ctx, int x1, int y1, int x2, int y2, SDL_Color color);

// Main render frame (orchestrates all drawing)
// Pass optional sun/moon data pointers (can be NULL)
typedef struct {
    double sun_declination;
    double sun_eot;
    time_t sun_sunrise;
    time_t sun_sunset;
    double sun_subsolar_lat;
    double sun_subsolar_lon;
    int sun_is_daylight;
} render_sun_data_t;

typedef struct {
    double moon_illumination;
    double moon_age;
    const char *moon_phase_name;
} render_moon_data_t;

void renderer_render_frame(renderer_context_t *ctx, font_set_t *fonts,
                          render_sun_data_t *sun, render_moon_data_t *moon);

// Input handling
int renderer_handle_events(renderer_context_t *ctx);

#endif // HAMCLOCK_RENDERER_H
