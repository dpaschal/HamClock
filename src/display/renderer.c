#include "renderer.h"
#include "../core/log.h"
#include "../core/state.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Color palette
static const SDL_Color COLOR_BLACK = {0, 0, 0, 255};
static const SDL_Color COLOR_WHITE = {255, 255, 255, 255};
static const SDL_Color COLOR_DARK_BG = {20, 20, 30, 255};
static const SDL_Color COLOR_GRID = {60, 60, 80, 255};
static const SDL_Color COLOR_ACCENT = {0, 200, 255, 255};  // Cyan for active data
static const SDL_Color COLOR_WARNING = {255, 200, 0, 255}; // Yellow for warning levels
static const SDL_Color COLOR_DANGER = {255, 100, 100, 255}; // Red for danger levels
static const SDL_Color COLOR_SUCCESS = {100, 255, 100, 255}; // Green for good conditions

int renderer_init(renderer_context_t *ctx, int width, int height) {
    if (!ctx) {
        log_error("renderer_init: ctx is NULL");
        return -1;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        log_error("SDL_Init failed: %s", SDL_GetError());
        return -1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        log_error("TTF_Init failed: %s", TTF_GetError());
        SDL_Quit();
        return -1;
    }

    // Create window
    ctx->window = SDL_CreateWindow(
        DEFAULT_WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );

    if (!ctx->window) {
        log_error("SDL_CreateWindow failed: %s", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Create renderer
    ctx->renderer = SDL_CreateRenderer(
        ctx->window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!ctx->renderer) {
        log_error("SDL_CreateRenderer failed: %s", SDL_GetError());
        SDL_DestroyWindow(ctx->window);
        TTF_Quit();
        SDL_Quit();
        return -1;
    }

    // Set logical rendering size for scaling
    SDL_RenderSetLogicalSize(ctx->renderer, width, height);

    ctx->width = width;
    ctx->height = height;
    ctx->running = 1;
    ctx->frame_count = 0;
    ctx->last_frame_time = SDL_GetTicks();

    log_info("Renderer initialized: %dx%d window with SDL2", width, height);
    return 0;
}

void renderer_deinit(renderer_context_t *ctx) {
    if (!ctx) return;

    if (ctx->renderer) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = NULL;
    }

    if (ctx->window) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = NULL;
    }

    TTF_Quit();
    SDL_Quit();

    log_info("Renderer deinitialized");
}

int renderer_load_fonts(font_set_t *fonts) {
    if (!fonts) {
        log_error("renderer_load_fonts: fonts is NULL");
        return -1;
    }

    // Try to load system fonts (common Linux paths)
    const char *font_paths[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/gnu-free/FreeSans.ttf",
        "C:\\Windows\\Fonts\\Arial.ttf",  // Windows fallback
        NULL
    };

    const char *font_file = NULL;
    for (int i = 0; font_paths[i]; i++) {
        FILE *f = fopen(font_paths[i], "r");
        if (f) {
            fclose(f);
            font_file = font_paths[i];
            break;
        }
    }

    if (!font_file) {
        log_warn("No TrueType fonts found; text rendering will be limited");
        fonts->font_large = NULL;
        fonts->font_normal = NULL;
        fonts->font_small = NULL;
        return 0;  // Non-fatal, continue with rendering
    }

    // Load fonts at different sizes
    fonts->font_large = TTF_OpenFont(font_file, 32);
    fonts->font_normal = TTF_OpenFont(font_file, 20);
    fonts->font_small = TTF_OpenFont(font_file, 14);

    if (!fonts->font_large || !fonts->font_normal || !fonts->font_small) {
        log_warn("Failed to load fonts: %s", TTF_GetError());
        return 0;  // Non-fatal
    }

    log_info("Fonts loaded successfully");
    return 0;
}

void renderer_unload_fonts(font_set_t *fonts) {
    if (!fonts) return;

    if (fonts->font_large) {
        TTF_CloseFont(fonts->font_large);
        fonts->font_large = NULL;
    }

    if (fonts->font_normal) {
        TTF_CloseFont(fonts->font_normal);
        fonts->font_normal = NULL;
    }

    if (fonts->font_small) {
        TTF_CloseFont(fonts->font_small);
        fonts->font_small = NULL;
    }
}

void renderer_clear(renderer_context_t *ctx) {
    if (!ctx || !ctx->renderer) return;

    SDL_SetRenderDrawColor(ctx->renderer, COLOR_DARK_BG.r, COLOR_DARK_BG.g,
                          COLOR_DARK_BG.b, COLOR_DARK_BG.a);
    SDL_RenderClear(ctx->renderer);
}

void renderer_present(renderer_context_t *ctx) {
    if (!ctx || !ctx->renderer) return;

    SDL_RenderPresent(ctx->renderer);
    ctx->frame_count++;
}

void renderer_limit_frame_rate(renderer_context_t *ctx) {
    if (!ctx) return;

    uint32_t now = SDL_GetTicks();
    uint32_t frame_time = now - ctx->last_frame_time;
    uint32_t target_frame_time = 1000 / FRAME_RATE;  // ms per frame

    if (frame_time < target_frame_time) {
        SDL_Delay(target_frame_time - frame_time);
    }

    ctx->last_frame_time = SDL_GetTicks();
}

void renderer_draw_text(renderer_context_t *ctx, TTF_Font *font,
                       const char *text, int x, int y,
                       SDL_Color fg, SDL_Color bg) {
    if (!ctx || !ctx->renderer || !font || !text) return;

    SDL_Surface *surface = TTF_RenderText_Shaded(font, text, fg, bg);
    if (!surface) return;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(ctx->renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) return;

    int w = 0, h = 0;
    SDL_QueryTexture(texture, NULL, NULL, &w, &h);

    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(ctx->renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
}

void renderer_draw_rect(renderer_context_t *ctx, int x, int y, int w, int h,
                       SDL_Color color) {
    if (!ctx || !ctx->renderer) return;

    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);

    SDL_Rect rect = {x, y, w, h};
    SDL_RenderDrawRect(ctx->renderer, &rect);
}

void renderer_fill_rect(renderer_context_t *ctx, int x, int y, int w, int h,
                       SDL_Color color) {
    if (!ctx || !ctx->renderer) return;

    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);

    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(ctx->renderer, &rect);
}

void renderer_draw_line(renderer_context_t *ctx, int x1, int y1, int x2, int y2,
                       SDL_Color color) {
    if (!ctx || !ctx->renderer) return;

    SDL_SetRenderDrawColor(ctx->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(ctx->renderer, x1, y1, x2, y2);
}

// Helper to determine color based on Kp index severity
static SDL_Color get_kp_color(float kp_index) {
    if (kp_index < 1.0f) return COLOR_SUCCESS;      // Green - quiet
    if (kp_index < 3.0f) return COLOR_ACCENT;       // Cyan - unsettled
    if (kp_index < 5.0f) return COLOR_WARNING;      // Yellow - active
    return COLOR_DANGER;                             // Red - strong/severe
}

// Helper to get Kp description
static const char *get_kp_description(float kp_index) {
    if (kp_index < 1.0f) return "Quiet";
    if (kp_index < 3.0f) return "Unsettled";
    if (kp_index < 5.0f) return "Active";
    if (kp_index < 6.0f) return "Minor Storm";
    if (kp_index < 7.0f) return "Major Storm";
    return "Severe Storm";
}

void renderer_render_frame(renderer_context_t *ctx, font_set_t *fonts,
                          render_sun_data_t *sun, render_moon_data_t *moon) {
    if (!ctx || !ctx->renderer) return;

    // Clear background
    renderer_clear(ctx);

    // Get current space weather data
    space_weather_t *sw = state_get_space_weather();

    // Render title bar
    renderer_fill_rect(ctx, 0, 0, ctx->width, 50, COLOR_GRID);
    if (fonts->font_large) {
        renderer_draw_text(ctx, fonts->font_large, "HamClock", 20, 10,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Render space weather panel (top-left)
    int panel_x = 20;
    int panel_y = 70;
    int panel_w = 300;
    int panel_h = 200;

    renderer_draw_rect(ctx, panel_x, panel_y, panel_w, panel_h, COLOR_GRID);

    if (fonts->font_normal) {
        renderer_draw_text(ctx, fonts->font_normal, "Space Weather", panel_x + 10, panel_y + 10,
                          COLOR_WHITE, COLOR_DARK_BG);
    }

    // Kp Index display
    if (fonts->font_normal) {
        SDL_Color kp_color = get_kp_color(sw->kp_index);
        char kp_str[64];
        snprintf(kp_str, sizeof(kp_str), "Kp: %.1f", sw->kp_index);
        renderer_draw_text(ctx, fonts->font_normal, kp_str, panel_x + 20, panel_y + 50,
                          kp_color, COLOR_DARK_BG);

        const char *kp_desc = get_kp_description(sw->kp_index);
        renderer_draw_text(ctx, fonts->font_small, kp_desc, panel_x + 20, panel_y + 80,
                          kp_color, COLOR_DARK_BG);
    }

    // Solar Flux display
    if (fonts->font_normal) {
        char flux_str[64];
        snprintf(flux_str, sizeof(flux_str), "Solar Flux: %.0f", sw->solar_flux);
        renderer_draw_text(ctx, fonts->font_normal, flux_str, panel_x + 20, panel_y + 115,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Sunspot Number display
    if (fonts->font_normal) {
        char ssn_str[64];
        snprintf(ssn_str, sizeof(ssn_str), "Sunspots: %d", sw->sunspot_number);
        renderer_draw_text(ctx, fonts->font_normal, ssn_str, panel_x + 20, panel_y + 150,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // A Index display
    if (fonts->font_small) {
        char a_str[64];
        snprintf(a_str, sizeof(a_str), "A-Index: %.0f", sw->a_index);
        renderer_draw_text(ctx, fonts->font_small, a_str, panel_x + 170, panel_y + 50,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Solar/Lunar panel (middle-left, below space weather)
    int solar_panel_x = 20;
    int solar_panel_y = 280;
    int solar_panel_w = 300;
    int solar_panel_h = 150;

    renderer_draw_rect(ctx, solar_panel_x, solar_panel_y, solar_panel_w, solar_panel_h, COLOR_GRID);

    if (fonts->font_normal) {
        renderer_draw_text(ctx, fonts->font_normal, "Sun & Moon", solar_panel_x + 10, solar_panel_y + 10,
                          COLOR_WHITE, COLOR_DARK_BG);
    }

    // Sun data
    if (sun && fonts->font_small) {
        char sun_str[64];
        snprintf(sun_str, sizeof(sun_str), "Sun Dec: %.1f°", sun->sun_declination);
        renderer_draw_text(ctx, fonts->font_small, sun_str, solar_panel_x + 20, solar_panel_y + 40,
                          COLOR_ACCENT, COLOR_DARK_BG);

        const char *sun_phase = sun->sun_is_daylight ? "☀ Daylight" : "🌙 Night";
        renderer_draw_text(ctx, fonts->font_small, sun_phase, solar_panel_x + 20, solar_panel_y + 65,
                          COLOR_ACCENT, COLOR_DARK_BG);

        snprintf(sun_str, sizeof(sun_str), "EoT: %+.1f min", sun->sun_eot);
        renderer_draw_text(ctx, fonts->font_small, sun_str, solar_panel_x + 170, solar_panel_y + 40,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Moon data
    if (moon && fonts->font_small) {
        char moon_str[64];
        snprintf(moon_str, sizeof(moon_str), "Moon: %.0f%%", moon->moon_illumination);
        renderer_draw_text(ctx, fonts->font_small, moon_str, solar_panel_x + 20, solar_panel_y + 90,
                          COLOR_SUCCESS, COLOR_DARK_BG);

        if (moon->moon_phase_name) {
            renderer_draw_text(ctx, fonts->font_small, moon->moon_phase_name, solar_panel_x + 170, solar_panel_y + 90,
                              COLOR_SUCCESS, COLOR_DARK_BG);
        }

        snprintf(moon_str, sizeof(moon_str), "Age: %.1f days", moon->moon_age);
        renderer_draw_text(ctx, fonts->font_small, moon_str, solar_panel_x + 20, solar_panel_y + 115,
                          COLOR_SUCCESS, COLOR_DARK_BG);
    }

    // Status panel (top-right)
    int status_x = ctx->width - 320;
    int status_y = 70;
    int status_w = 300;
    int status_h = 200;

    renderer_draw_rect(ctx, status_x, status_y, status_w, status_h, COLOR_GRID);

    if (fonts->font_normal) {
        renderer_draw_text(ctx, fonts->font_normal, "Status", status_x + 10, status_y + 10,
                          COLOR_WHITE, COLOR_DARK_BG);
    }

    // Display frame rate and info
    if (fonts->font_small) {
        char fps_str[64];
        snprintf(fps_str, sizeof(fps_str), "FPS: %d", ctx->frame_count);
        renderer_draw_text(ctx, fonts->font_small, fps_str, status_x + 20, status_y + 50,
                          COLOR_WHITE, COLOR_DARK_BG);

        char info_str[128];
        snprintf(info_str, sizeof(info_str), "Window: %dx%d", ctx->width, ctx->height);
        renderer_draw_text(ctx, fonts->font_small, info_str, status_x + 20, status_y + 85,
                          COLOR_WHITE, COLOR_DARK_BG);

        char hint_str[128];
        snprintf(hint_str, sizeof(hint_str), "Press Q or ESC to quit");
        renderer_draw_text(ctx, fonts->font_small, hint_str, status_x + 20, status_y + 150,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Render timestamp
    if (fonts->font_small && sw->timestamp > 0) {
        char time_str[64];
        time_t now = sw->timestamp;
        struct tm *tm_info = gmtime(&now);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S UTC", tm_info);

        renderer_draw_text(ctx, fonts->font_small, time_str, 20, ctx->height - 30,
                          COLOR_ACCENT, COLOR_DARK_BG);
    }

    // Present the rendered frame
    renderer_present(ctx);
}

int renderer_handle_events(renderer_context_t *ctx) {
    if (!ctx) return 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                ctx->running = 0;
                return 0;

            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                    case SDLK_ESCAPE:
                        ctx->running = 0;
                        return 0;
                    default:
                        break;
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    ctx->width = event.window.data1;
                    ctx->height = event.window.data2;
                    log_debug("Window resized to %dx%d", ctx->width, ctx->height);
                }
                break;

            default:
                break;
        }
    }

    return ctx->running;
}
