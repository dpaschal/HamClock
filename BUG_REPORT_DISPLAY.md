# Bug Report: Earthmap Rendering Position Issue

**Date:** 2025-12-22
**Severity:** HIGH
**Status:** REPORTED
**Component:** Display (SDL2 Rendering)

---

## Summary

The earthmap (world map with greyline) is not rendering at the correct position on screen. The window opens but displays mostly black content instead of showing the map and interface elements.

## Steps to Reproduce

1. Build HamClock C:
   ```bash
   cd /home/paschal/projects/hamclock-c
   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   make -j4
   ```

2. Run the application:
   ```bash
   ./hamclock
   ```

3. Observe the window - it will show mostly black with no visible:
   - Greyline map
   - Geographic grid
   - Clock displays
   - Any title bar or UI elements

## Expected Behavior

The 1024x768 window should display:
- **Title bar** (top): "HamClock" text, dark gray background, 50px high
- **Earthmap** (left side): 800x500 world map with:
  - Ocean in dark blue
  - Continents in green
  - Greyline (day/night terminator) in tan/orange
  - Geographic grid lines
  - Observer location marker
- **Clock panel** (right side): 190x380 area showing:
  - UTC time
  - CET (Central Europe) time
  - EST (Eastern US) time
  - Local system time

## Root Cause Analysis

The bug is in the rendering pipeline:

### Issue 1: Positioning Not Implemented

In `src/main.c` (lines 182-188), the code creates a `map_rect` with correct positioning:
```c
SDL_Rect map_rect = {10, 60, 800, 500};

earthmap_render_base(&map_ctx);          // Renders at (0,0)
earthmap_render_grid(&map_ctx);          // Renders at (0,0)
earthmap_render_greyline(&map_ctx, &sun_pos);  // Renders at (0,0)
earthmap_render_observer(&map_ctx, &observer); // Renders at (0,0)
```

However, the `map_rect` is **never used**. Instead, these functions render from absolute coordinates (0,0).

### Issue 2: Earthmap Context Missing Offset

The `earthmap_ctx_t` structure in `src/display/earthmap.h` does not have offset fields:
```c
typedef struct {
    SDL_Renderer *renderer;
    int width;           // Map width (800)
    int height;          // Map height (500)
    // MISSING: int offset_x; and int offset_y;
    // ...
} earthmap_ctx_t;
```

### Issue 3: Rendering Functions Ignore Positioning

All earthmap rendering functions (`earthmap_render_base()`, `earthmap_render_grid()`, `earthmap_render_greyline()`, `earthmap_render_observer()`) render directly to absolute screen coordinates without accounting for the intended position (10, 60).

## Affected Files

1. **src/main.c** (lines 182-188)
   - Creates map_rect but doesn't use it
   - Calls earthmap functions without positioning

2. **src/display/earthmap.h**
   - `earthmap_ctx_t` missing offset_x, offset_y fields

3. **src/display/earthmap.c**
   - `earthmap_render_base()` - fills background without offset
   - `earthmap_render_grid()` - draws grid without offset
   - `earthmap_render_greyline()` - draws greyline without offset
   - `earthmap_render_observer()` - draws marker without offset
   - `earthmap_init()` - doesn't initialize offsets

## Proposed Solutions

### Option 1: Add Offset to Context (Recommended)
```c
// In earthmap.h
typedef struct {
    SDL_Renderer *renderer;
    int width, height;
    int offset_x, offset_y;  // ADD THESE
    // ... rest of fields
} earthmap_ctx_t;

// In main.c
earthmap_init(&map_ctx, render_ctx.renderer, 800, 500);
map_ctx.offset_x = 10;   // Set offset
map_ctx.offset_y = 60;
```

Then modify all rendering functions to use offset when drawing.

### Option 2: Use SDL Viewport
```c
// Before rendering earthmap
SDL_Rect viewport = {10, 60, 800, 500};
SDL_RenderSetViewport(render_ctx.renderer, &viewport);

// Render earthmap (renders within viewport)
earthmap_render_base(&map_ctx);
earthmap_render_grid(&map_ctx);
// ...

// Restore viewport
SDL_RenderSetViewport(render_ctx.renderer, NULL);
```

### Option 3: Pass Offset as Parameters
Add offset parameters to each earthmap render function:
```c
void earthmap_render_base(earthmap_ctx_t *ctx, int offset_x, int offset_y);
void earthmap_render_grid(earthmap_ctx_t *ctx, int offset_x, int offset_y);
// etc.
```

## Impact

**High Priority** - This is a core feature that prevents users from seeing the main content (world map with greyline).

## Test Case

Run the compiled binary and verify:
```bash
./hamclock &
# Window should show properly positioned elements
# Kill with: killall hamclock
```

If the window is mostly black, the bug is confirmed.

## Notes

- All other systems initialize correctly (logging, database, config, API manager)
- SDL2 window creation and rendering context are working
- The issue is purely in the positioning logic for the earthmap component
- Clock rendering may also have positioning issues

---

**Tagged:** rendering, display, SDL2, UI-layout
