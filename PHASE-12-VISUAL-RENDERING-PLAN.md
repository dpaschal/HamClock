# HamClock Phase 12: Visual Layer Rendering
## Competitive Visual Differentiation

**Date:** 2025-12-19
**Scope:** MUF Heat Map, Aurora Oval, Grid, ITU Regions visual rendering
**Target:** Visual parity with Geochron 4K display
**Estimated Time:** 3-4 days
**Complexity:** MEDIUM-HIGH (GPU texture/shader work)
**Total LOC:** ~800 lines

---

## Executive Summary

Phase 12 implements the actual visual rendering of the layer system created in Phase 11. This transforms the layer manager from a control system into a fully visual display system that shows:

1. **MUF Heat Map** - Color-coded frequency propagation zones
2. **Aurora Oval** - Geographic prediction of auroral activity
3. **Maidenhead Grid** - VHF/UHF contest grid overlay
4. **ITU Regions** - International regulatory boundaries

**Expected Outcome:** HamClock becomes visually distinctive from competitors with clean, GPU-accelerated overlay rendering.

---

## Implementation Strategy

### Architecture Pattern: Layer Rendering Pipeline

```
LayerManager (control)
    ↓
LayerRenderer (graphics)
    ├─ MufHeatmapRenderer
    ├─ AuroraOvalRenderer
    ├─ GridRenderer
    └─ ItuRegionRenderer

Each renderer outputs geometry/textures to GPU queue
```

---

## Phase 12.1: MUF Heat Map Rendering (Day 1)

### File: `src/render/heatmap.rs` (NEW, ~250 LOC)

```rust
//! MUF heat map rendering for frequency propagation visualization

use wgpu::*;
use crate::data::forecast::MufForecast;

pub struct MufHeatmapRenderer {
    /// Texture containing MUF values as colors
    texture: Texture,
    texture_view: TextureView,
    sampler: Sampler,
    /// Bind group for rendering
    bind_group: BindGroup,
    /// Render pipeline
    pipeline: RenderPipeline,
    width: u32,
    height: u32,
}

impl MufHeatmapRenderer {
    pub fn new(
        device: &Device,
        queue: &Queue,
        format: TextureFormat,
        muf_data: &MufForecast,
    ) -> Result<Self, String> {
        let (grid_width, grid_height) = muf_data.resolution;

        // Create RGBA texture from MUF grid
        let mut rgba_data = vec![0u8; (grid_width * grid_height * 4) as usize];

        for (i, row) in muf_data.grid.iter().enumerate() {
            for (j, &muf_value) in row.iter().enumerate() {
                let idx = (i * grid_width as usize + j) * 4;
                let color = Self::muf_to_rgba(muf_value);
                rgba_data[idx] = color.0;
                rgba_data[idx + 1] = color.1;
                rgba_data[idx + 2] = color.2;
                rgba_data[idx + 3] = 200; // Semi-transparent
            }
        }

        // Create texture
        let texture = device.create_texture(&TextureDescriptor {
            label: Some("MUF Heatmap"),
            size: Extent3d {
                width: grid_width,
                height: grid_height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: TextureDimension::D2,
            format: TextureFormat::Rgba8UnormSrgb,
            usage: TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_DST,
            view_formats: &[],
        });

        queue.write_texture(
            ImageCopyTexture {
                texture: &texture,
                mip_level: 0,
                origin: Origin3d::ZERO,
            },
            &rgba_data,
            ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(grid_width * 4),
                rows_per_image: Some(grid_height),
            },
            Extent3d {
                width: grid_width,
                height: grid_height,
                depth_or_array_layers: 1,
            },
        );

        let texture_view = texture.create_view(&TextureViewDescriptor::default());
        let sampler = device.create_sampler(&SamplerDescriptor {
            label: Some("MUF Sampler"),
            address_mode_u: AddressMode::ClampToEdge,
            address_mode_v: AddressMode::ClampToEdge,
            address_mode_w: AddressMode::ClampToEdge,
            mag_filter: FilterMode::Linear,
            min_filter: FilterMode::Linear,
            ..Default::default()
        });

        Ok(Self {
            texture,
            texture_view,
            sampler,
            bind_group: Default::default(), // Placeholder
            pipeline: Default::default(),   // Placeholder
            width: grid_width,
            height: grid_height,
        })
    }

    /// Convert MUF value (MHz) to RGBA color
    fn muf_to_rgba(muf: f32) -> (u8, u8, u8) {
        let normalized = (muf / 50.0).clamp(0.0, 1.0);

        match normalized {
            n if n < 0.2 => (0, 50, 150),           // Deep blue - poor
            n if n < 0.4 => (0, 150, 200),          // Light blue - marginal
            n if n < 0.6 => (0, 200, 100),          // Green - fair
            n if n < 0.8 => (200, 200, 0),          // Yellow - good
            _ => (200, 50, 0),                      // Orange/red - excellent
        }
    }

    pub fn texture_view(&self) -> &TextureView {
        &self.texture_view
    }

    pub fn sampler(&self) -> &Sampler {
        &self.sampler
    }
}
```

### Integration: Render MUF overlay in `gpu.rs`

```rust
// In queue_ui_elements()
if let Some(muf_layer) = self.layers.get_layer(LayerType::MufHeatmap) {
    if muf_layer.visible && app_data.hf_forecast.is_some() {
        // Queue MUF heat map rendering
        // (Will use texture blending with opacity)
        log::debug!("Queuing MUF heatmap overlay");
    }
}
```

---

## Phase 12.2: Aurora Oval Rendering (Day 2)

### File: `src/render/aurora.rs` (NEW, ~200 LOC)

```rust
//! Aurora oval prediction visualization

use nalgebra::Vector2;
use chrono::Utc;

pub struct AuroraOvalRenderer {
    /// Contour points for aurora oval
    contour: Vec<(f32, f32)>, // (latitude, longitude)
    /// Center latitude
    latitude: f32,
    /// Visual intensity (0.0-1.0)
    intensity: f32,
}

impl AuroraOvalRenderer {
    pub fn from_kp(kp: f32) -> Self {
        // Empirical aurora latitude model
        let latitude = match kp {
            k if k <= 3.0 => 67.0 - (k * 0.5),
            k if k <= 5.0 => 65.0 - ((k - 3.0) * 2.0),
            k if k <= 7.0 => 61.0 - ((k - 5.0) * 2.5),
            k if k <= 9.0 => 56.0 - ((k - 7.0) * 2.0),
            _ => 52.0,
        };

        let intensity = (kp / 9.0).min(1.0);
        let width = (kp / 9.0) * 8.0 + 2.0; // 2-10 degrees

        let mut contour = Vec::with_capacity(360);
        for i in 0..360 {
            let angle = (i as f32).to_radians();

            // Distort oval (not perfectly circular)
            let distortion = 1.0 + 0.3 * angle.cos();
            let lat_offset = width * 0.5 * distortion;

            let lat = (latitude - lat_offset).clamp(-90.0, 90.0);
            let lon = (i as f32).rem_euclid(360.0);

            contour.push((lat, lon));
        }

        Self {
            contour,
            latitude: latitude.clamp(45.0, 75.0),
            intensity,
        }
    }

    pub fn color(&self) -> [f32; 4] {
        match self.intensity {
            i if i < 0.2 => [0.0, 0.5, 0.0, 0.3],      // Dim green
            i if i < 0.4 => [0.0, 1.0, 0.0, 0.4],      // Green
            i if i < 0.6 => [0.0, 1.0, 1.0, 0.5],      // Cyan
            i if i < 0.8 => [1.0, 1.0, 0.0, 0.6],      // Yellow
            _ => [1.0, 0.0, 1.0, 0.7],                 // Magenta
        }
    }

    pub fn get_contour(&self) -> &[(f32, f32)] {
        &self.contour
    }

    pub fn intensity(&self) -> f32 {
        self.intensity
    }
}
```

### Integration: Render aurora oval

```rust
// In queue_ui_elements()
if let Some(aurora_layer) = self.layers.get_layer(LayerType::AuroraOval) {
    if aurora_layer.visible && app_data.space_weather.kp > 2.0 {
        let aurora = AuroraOvalRenderer::from_kp(app_data.space_weather.kp);

        // Draw aurora contour on map
        for (lat, lon) in aurora.get_contour() {
            // Project lat/lon to screen coordinates
            // Draw line segment
        }
    }
}
```

---

## Phase 12.3: Maidenhead Grid Rendering (Day 3)

### File: `src/render/grid.rs` (NEW, ~150 LOC)

```rust
//! Maidenhead locator grid rendering

pub struct MaidenheadGridRenderer;

impl MaidenheadGridRenderer {
    /// Generate grid lines for specified resolution
    pub fn generate_lines(resolution: u32) -> Vec<Line> {
        let mut lines = Vec::new();

        // Major grid (field, square resolution)
        match resolution {
            1 => {
                // Field level (20° × 10°)
                for field_lon in (0..18).step_by(2) {
                    for field_lat in (0..9).step_by(1) {
                        lines.push(Line {
                            lat1: (field_lat * 10) as f32,
                            lon1: (field_lon * 20) as f32,
                            lat2: (field_lat * 10 + 10) as f32,
                            lon2: (field_lon * 20 + 20) as f32,
                            is_major: true,
                        });
                    }
                }
            }
            2 => {
                // Square level (2° × 1°)
                for lon_idx in 0..180 {
                    for lat_idx in 0..90 {
                        lines.push(Line {
                            lat1: (lat_idx as f32) - 90.0,
                            lon1: (lon_idx as f32) - 180.0,
                            lat2: (lat_idx as f32 + 1.0) - 90.0,
                            lon2: (lon_idx as f32 + 2.0) - 180.0,
                            is_major: false,
                        });
                    }
                }
            }
            _ => {}
        }

        lines
    }

    /// Get grid square label for lat/lon
    pub fn locator_to_string(lat: f32, lon: f32) -> String {
        // Maidenhead locator system
        let field_lon = ((lon + 180.0) / 20.0).floor() as u32;
        let field_lat = ((lat + 90.0) / 10.0).floor() as u32;

        let square_lon = ((lon + 180.0) % 20.0 / 2.0).floor() as u32;
        let square_lat = ((lat + 90.0) % 10.0 / 1.0).floor() as u32;

        let field_lon_char = ((field_lon % 18) as u8 + b'A') as char;
        let field_lat_char = ((field_lat % 9) as u8 + b'A') as char;
        let square_lon_str = (square_lon % 9).to_string();
        let square_lat_str = (square_lat % 9).to_string();

        format!("{}{}{}{}", field_lon_char, field_lat_char, square_lon_str, square_lat_str)
    }
}

pub struct Line {
    pub lat1: f32,
    pub lon1: f32,
    pub lat2: f32,
    pub lon2: f32,
    pub is_major: bool,
}
```

---

## Phase 12.4: ITU Region Boundaries (Day 4)

### File: `src/render/itu.rs` (NEW, ~150 LOC)

```rust
//! ITU region boundary rendering

pub struct ItuRegionRenderer;

#[derive(Clone, Copy)]
pub enum ItuRegion {
    Region1,  // Europe, Africa, Russia, Middle East
    Region2,  // Americas
    Region3,  // Asia-Pacific
}

impl ItuRegionRenderer {
    /// Get boundary coordinates for ITU region
    pub fn get_boundaries(region: ItuRegion) -> Vec<(f32, f32)> {
        match region {
            ItuRegion::Region1 => vec![
                // Europe/Africa/Russia boundaries
                (-90.0, 0.0), (90.0, 0.0),     // Left edge at prime meridian
                (90.0, 40.0), (-90.0, 40.0),   // Right boundary ~40°E
                // African coast follows
                (0.0, 20.0), (35.0, 50.0),
            ],
            ItuRegion::Region2 => vec![
                // Americas
                (90.0, -170.0), (-90.0, -170.0),
                (-90.0, -30.0), (90.0, -30.0),
            ],
            ItuRegion::Region3 => vec![
                // Asia-Pacific
                (-90.0, 40.0), (90.0, 40.0),
                (90.0, 165.0), (-90.0, 165.0),
            ],
        }
    }

    pub fn color(region: ItuRegion) -> [f32; 4] {
        match region {
            ItuRegion::Region1 => [1.0, 0.0, 0.0, 0.2],  // Red
            ItuRegion::Region2 => [0.0, 1.0, 0.0, 0.2],  // Green
            ItuRegion::Region3 => [0.0, 0.0, 1.0, 0.2],  // Blue
        }
    }
}
```

---

## Phase 12.5: Layer Rendering Pipeline Integration

### File: `src/render/layer_renderer.rs` (NEW, ~150 LOC)

```rust
//! Unified layer rendering pipeline

use crate::render::{LayerType, LayerManager};
use crate::data::models::AppData;

pub struct LayerRenderingPipeline {
    muf_renderer: Option<MufHeatmapRenderer>,
    aurora_renderer: Option<AuroraOvalRenderer>,
    grid_renderer: MaidenheadGridRenderer,
    itu_renderer: ItuRegionRenderer,
}

impl LayerRenderingPipeline {
    pub fn new() -> Self {
        Self {
            muf_renderer: None,
            aurora_renderer: None,
            grid_renderer: MaidenheadGridRenderer,
            itu_renderer: ItuRegionRenderer,
        }
    }

    pub fn update_forecast(&mut self, app_data: &AppData) {
        if let Some(forecast) = &app_data.hf_forecast {
            // Update MUF renderer with latest forecast
            log::debug!("Updating MUF heatmap");
        }
    }

    pub fn render_layers(
        &self,
        render_pass: &mut RenderPass,
        layers: &LayerManager,
        app_data: &AppData,
    ) {
        let rendering_order = layers.rendering_order();

        for layer_type in rendering_order {
            if let Some(layer) = layers.get_layer(layer_type) {
                if !layer.visible {
                    continue;
                }

                match layer_type {
                    LayerType::MufHeatmap => {
                        if let Some(muf) = &self.muf_renderer {
                            // Render MUF texture with opacity
                            log::debug!("Rendering MUF heatmap at opacity {}", layer.opacity);
                        }
                    }
                    LayerType::AuroraOval => {
                        let aurora = AuroraOvalRenderer::from_kp(app_data.space_weather.kp);
                        log::debug!("Rendering aurora oval at intensity {}", aurora.intensity());
                    }
                    LayerType::GridSquares => {
                        let lines = MaidenheadGridRenderer::generate_lines(2);
                        log::debug!("Rendering {} grid lines", lines.len());
                    }
                    LayerType::ItuRegions => {
                        log::debug!("Rendering ITU region boundaries");
                    }
                    _ => {}
                }
            }
        }
    }
}
```

---

## Integration Steps

### 1. Export new modules in `src/render/mod.rs`

```rust
pub mod heatmap;      // Phase 12
pub mod aurora;       // Phase 12
pub mod grid;         // Phase 12
pub mod itu;          // Phase 12
pub mod layer_renderer; // Phase 12

pub use layer_renderer::LayerRenderingPipeline;
```

### 2. Add to `GpuContext`

```rust
pub struct GpuContext {
    // ... existing fields ...
    pub layer_renderer: LayerRenderingPipeline, // Phase 12
}
```

### 3. Render in `render_frame()`

```rust
pub fn render_frame(&mut self, app_data: &AppData) -> AppResult<()> {
    // ... existing setup ...

    {
        let mut render_pass = encoder.begin_render_pass(...);

        // Render text first (background)
        self.text_renderer.render(&mut render_pass, ...)?;

        // Then render layers on top
        self.layer_renderer.render_layers(
            &mut render_pass,
            &self.layers,
            app_data,
        );

        drop(render_pass);
    }

    // ... present ...
}
```

---

## Testing Strategy

### Visual Tests
- [ ] MUF heatmap gradient renders correctly (blue→red)
- [ ] Aurora oval updates with Kp changes
- [ ] Grid lines overlay without artifacts
- [ ] ITU regions display correct boundaries
- [ ] Opacity blending works correctly
- [ ] Layer ordering is correct (back-to-front)

### Performance Tests
- [ ] Layer rendering < 5ms per frame
- [ ] Texture updates (MUF) don't stall GPU
- [ ] No memory leaks with layer toggling
- [ ] Smooth opacity transitions

### Integration Tests
- [ ] Keyboard layer toggle updates display
- [ ] Multiple layers render simultaneously
- [ ] Blend modes work as expected
- [ ] Forecast updates refresh layers

---

## Success Criteria

| Criterion | Target | Status |
|-----------|--------|--------|
| MUF heatmap renders | Color gradient visible | ⏳ |
| Aurora oval visible | Animated with Kp | ⏳ |
| Grid overlay | Clean lines, no flicker | ⏳ |
| ITU regions | Colored boundaries | ⏳ |
| Performance | < 5ms layer render time | ⏳ |
| Visual quality | Smooth, anti-aliased | ⏳ |
| Competitive parity | Matches Geochron display | ⏳ |

---

## Implementation Priority

**Day 1 (Most Important):** MUF Heatmap
- Highest competitive value
- Directly useful for DX operations
- Foundation for other visualizations

**Day 2:** Aurora Oval
- Complements space weather display
- Real-time animation with Kp
- Critical for VHF operators

**Day 3:** Maidenhead Grid
- VHF/UHF contest feature
- Relatively simple implementation
- High user value for grid-hunting

**Day 4:** ITU Regions
- Educational/reference value
- Lower priority than above
- Can be added post-Phase 12

---

## Future Enhancements (Phase 13+)

- Topographic map support (elevation shading)
- Call sign prefix zones
- Real-time signal propagation trails
- Historical MUF trend graphs
- Custom overlay support
- Mobile responsiveness
- Touch gestures for layer control
- Export visualization to image

---

**Ready to implement Phase 12! Start with MUF heatmap rendering?**
