#ifndef HAMCLOCK_CLOCKS_H
#define HAMCLOCK_CLOCKS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <time.h>
#include "../utils/timezone.h"

// Clock widget
typedef struct {
    timezone_t timezone;
    const char *label;      // e.g., "UTC", "DE", "DX", "Local"
    int x, y;               // Position on screen
    int width, height;      // Widget size
    SDL_Color label_color;
    SDL_Color time_color;
} clock_widget_t;

// Clock panel (collection of clocks)
typedef struct {
    clock_widget_t *clocks;
    int num_clocks;
    int max_clocks;
    int x, y;               // Panel position
    int width, height;      // Panel size
} clock_panel_t;

// Initialize clock panel
int clocks_panel_init(clock_panel_t *panel, int max_clocks, int x, int y, int w, int h);

// Cleanup clock panel
void clocks_panel_deinit(clock_panel_t *panel);

// Add a clock to the panel
int clocks_add(clock_panel_t *panel, const char *label, timezone_t tz,
              SDL_Color label_color, SDL_Color time_color);

// Render clock panel
void clocks_render(clock_panel_t *panel, SDL_Renderer *renderer, TTF_Font *font_large,
                  TTF_Font *font_normal, TTF_Font *font_small);

// Update all clocks (called once per frame)
void clocks_update(clock_panel_t *panel, time_t current_time);

// Get time in specific clock
local_time_t *clocks_get_time(clock_panel_t *panel, int clock_index);

// Set clock position
void clocks_set_position(clock_widget_t *clock, int x, int y);

// Render single clock widget
void clocks_render_widget(clock_widget_t *clock, SDL_Renderer *renderer,
                         TTF_Font *font_large, TTF_Font *font_normal);

#endif // HAMCLOCK_CLOCKS_H
