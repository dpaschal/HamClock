#include "clocks.h"
#include "../core/log.h"
#include <stdlib.h>
#include <string.h>

// Color palette for clocks - improved aesthetics
static const SDL_Color COLOR_DARK_BG = {40, 45, 55, 255};    // Slightly lighter
static const SDL_Color COLOR_GRID = {80, 100, 130, 255};     // Better contrast
static const SDL_Color COLOR_ACCENT = {120, 200, 255, 255};  // Softer cyan

// Clock data (stores current time for each clock)
typedef struct {
    clock_widget_t widget;
    local_time_t current_time;
} clock_data_t;

int clocks_panel_init(clock_panel_t *panel, int max_clocks, int x, int y, int w, int h) {
    if (!panel || max_clocks < 1) return -1;

    panel->clocks = (clock_widget_t *)malloc(sizeof(clock_widget_t) * max_clocks);
    if (!panel->clocks) {
        log_error("Failed to allocate clock array");
        return -1;
    }

    panel->num_clocks = 0;
    panel->max_clocks = max_clocks;
    panel->x = x;
    panel->y = y;
    panel->width = w;
    panel->height = h;

    log_info("Clock panel initialized: %dx%d at (%d,%d), max %d clocks", w, h, x, y, max_clocks);
    return 0;
}

void clocks_panel_deinit(clock_panel_t *panel) {
    if (!panel) return;
    if (panel->clocks) {
        free(panel->clocks);
        panel->clocks = NULL;
    }
    log_info("Clock panel deinitialized");
}

int clocks_add(clock_panel_t *panel, const char *label, timezone_t tz,
              SDL_Color label_color, SDL_Color time_color) {
    if (!panel || panel->num_clocks >= panel->max_clocks) {
        return -1;
    }

    clock_widget_t *clock = &panel->clocks[panel->num_clocks];
    clock->timezone = tz;
    clock->label = label;
    clock->label_color = label_color;
    clock->time_color = time_color;

    // Calculate position - single column layout for better alignment
    int clock_width = panel->width - 20;  // Full width with margins
    int clock_height = 45;  // Compact height
    int row = panel->num_clocks;

    clock->x = panel->x + 10;
    clock->y = panel->y + 10 + row * (clock_height + 8);  // 8px spacing between clocks
    clock->width = clock_width;
    clock->height = clock_height;

    panel->num_clocks++;

    log_info("Added clock: %s (%d) at (%d,%d)", label, tz, clock->x, clock->y);
    return panel->num_clocks - 1;
}

void clocks_update(clock_panel_t *panel, time_t current_time) {
    if (!panel || !panel->clocks) return;

    for (int i = 0; i < panel->num_clocks; i++) {
        clock_widget_t *clock = &panel->clocks[i];
        // Update clock time (would need to store the local_time_t separately)
        // For now, just convert on render
        timezone_convert(current_time, clock->timezone, NULL);
    }
}

local_time_t *clocks_get_time(clock_panel_t *panel, int clock_index) {
    if (!panel || clock_index < 0 || clock_index >= panel->num_clocks) {
        return NULL;
    }
    // Note: Would need to store local_time_t in a persistent structure
    return NULL;
}

void clocks_set_position(clock_widget_t *clock, int x, int y) {
    if (clock) {
        clock->x = x;
        clock->y = y;
    }
}

void clocks_render_widget(clock_widget_t *clock, SDL_Renderer *renderer,
                         TTF_Font *font_large, TTF_Font *font_normal) {
    if (!clock || !renderer) return;

    // Draw widget border
    SDL_SetRenderDrawColor(renderer, COLOR_GRID.r, COLOR_GRID.g, COLOR_GRID.b, COLOR_GRID.a);
    SDL_Rect border = {clock->x, clock->y, clock->width, clock->height};
    SDL_RenderDrawRect(renderer, &border);

    // Get current time in this timezone
    local_time_t local_time;
    time_t now = time(NULL);
    timezone_convert(now, clock->timezone, &local_time);

    // Format time string
    char time_str[16];
    timezone_format_time_short(&local_time, time_str, sizeof(time_str));

    // Draw label (left side)
    if (font_normal) {
        SDL_Surface *label_surf = TTF_RenderText_Shaded(font_normal, clock->label,
                                                        clock->label_color, COLOR_DARK_BG);
        if (label_surf) {
            SDL_Texture *label_tex = SDL_CreateTextureFromSurface(renderer, label_surf);
            if (label_tex) {
                int w, h;
                SDL_QueryTexture(label_tex, NULL, NULL, &w, &h);
                SDL_Rect label_rect = {clock->x + 5, clock->y + 8, w, h};
                SDL_RenderCopy(renderer, label_tex, NULL, &label_rect);
                SDL_DestroyTexture(label_tex);
            }
            SDL_FreeSurface(label_surf);
        }
    }

    // Draw time (right side, aligned to right edge)
    if (font_normal) {  // Use font_normal for compact time display
        SDL_Surface *time_surf = TTF_RenderText_Shaded(font_normal, time_str,
                                                       clock->time_color, COLOR_DARK_BG);
        if (time_surf) {
            SDL_Texture *time_tex = SDL_CreateTextureFromSurface(renderer, time_surf);
            if (time_tex) {
                int w, h;
                SDL_QueryTexture(time_tex, NULL, NULL, &w, &h);
                // Right-align time in widget with 5px margin
                int x = clock->x + clock->width - w - 5;
                int y = clock->y + 10;
                SDL_Rect time_rect = {x, y, w, h};
                SDL_RenderCopy(renderer, time_tex, NULL, &time_rect);
                SDL_DestroyTexture(time_tex);
            }
            SDL_FreeSurface(time_surf);
        }
    }
}

void clocks_render(clock_panel_t *panel, SDL_Renderer *renderer, TTF_Font *font_large,
                  TTF_Font *font_normal, TTF_Font *font_small) {
    if (!panel || !renderer) return;

    // Draw panel background
    SDL_SetRenderDrawColor(renderer, COLOR_DARK_BG.r, COLOR_DARK_BG.g,
                          COLOR_DARK_BG.b, COLOR_DARK_BG.a);
    SDL_Rect panel_rect = {panel->x, panel->y, panel->width, panel->height};
    SDL_RenderFillRect(renderer, &panel_rect);

    // Draw panel border
    SDL_SetRenderDrawColor(renderer, COLOR_GRID.r, COLOR_GRID.g,
                          COLOR_GRID.b, COLOR_GRID.a);
    SDL_RenderDrawRect(renderer, &panel_rect);

    // Render each clock
    for (int i = 0; i < panel->num_clocks; i++) {
        clocks_render_widget(&panel->clocks[i], renderer, font_large, font_normal);
    }

    (void)font_small;  // Unused for now
}
