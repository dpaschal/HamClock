# HamClock Phase 10-11: Advanced Propagation & Visualization
## Competitive Feature Implementation

**Date:** 2025-12-19
**Scope:** MUF Heat Map, 3-Day HF Forecast, Aurora Oval Overlay
**Target:** Competitive parity with Geochron 4K ($3000 commercial system)
**Estimated Time:** 5-7 days
**Complexity:** MEDIUM-HIGH (GPU rendering + ionospheric models)
**Total LOC:** ~1,200 lines

---

## Executive Summary

Phase 10-11 implements three critical visualization features from competitive analysis:

1. **Phase 10: Propagation Forecasting (3 days)**
   - NOAA HF band predictions
   - Band open/close timelines
   - MUF (Maximum Usable Frequency) predictions

2. **Phase 11: Advanced Visualization**
   - MUF heat map overlay (color-coded by frequency)
   - Aurora oval geographic prediction
   - Multi-layer overlay system
   - Dynamic layer toggle UI

**Expected Outcome:** HamClock becomes feature-competitive with $3000+ Geochron system while remaining FREE and open source.

---

## Phase 10: Propagation Forecasting (Days 1-3)

### 10.1: NOAA Forecast Data Integration

**File:** `src/data/forecast.rs` (NEW, ~200 LOC)

```rust
//! NOAA propagation forecasting module

use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc, Duration};

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HfForecast {
    pub issued_at: DateTime<Utc>,
    pub valid_from: DateTime<Utc>,
    pub valid_to: DateTime<Utc>,
    pub kp_forecast: Vec<KpForecast>,
    pub band_forecast: Vec<BandForecast>,
    pub muf_forecast: MufForecast,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct KpForecast {
    pub time: DateTime<Utc>,
    pub kp_min: f32,
    pub kp_max: f32,
    pub kp_pred: f32,
    pub a_index: u32,
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct BandForecast {
    pub band_name: String,      // "160m", "80m", "40m", etc.
    pub frequency_mhz: f32,
    pub forecast_status: BandStatus,
    pub confidence: f32,        // 0.0-1.0
    pub best_time: Option<DateTime<Utc>>,
    pub opens_at: Option<DateTime<Utc>>,
    pub closes_at: Option<DateTime<Utc>>,
}

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq)]
pub enum BandStatus {
    Closed,     // Propagation impossible
    Marginal,   // Unlikely but possible
    Open,       // Good propagation expected
    Excellent,  // Excellent propagation
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MufForecast {
    pub grid: Vec<Vec<f32>>,    // 2D grid of MUF values (lat x lon)
    pub latitude_range: (f32, f32),    // (min, max)
    pub longitude_range: (f32, f32),   // (min, max)
    pub resolution: (u32, u32),        // grid dimensions
    pub valid_at: DateTime<Utc>,
}

pub struct ForecastFetcher;

impl ForecastFetcher {
    /// Fetch NOAA 3-day HF forecast
    pub async fn fetch_hf_forecast() -> Result<HfForecast, Box<dyn std::error::Error>> {
        // TODO: Implement NOAA API integration
        // Endpoint: https://services.swpc.noaa.gov/products/noaa-scales.json
        // Parse 3-day Kp forecast and band predictions

        Ok(HfForecast {
            issued_at: Utc::now(),
            valid_from: Utc::now(),
            valid_to: Utc::now() + Duration::days(3),
            kp_forecast: Vec::new(),
            band_forecast: Vec::new(),
            muf_forecast: MufForecast {
                grid: Vec::new(),
                latitude_range: (-90.0, 90.0),
                longitude_range: (-180.0, 180.0),
                resolution: (180, 360),
                valid_at: Utc::now(),
            },
        })
    }

    /// Predict MUF from Kp and solar flux using empirical model
    pub fn calculate_muf(kp: f32, solar_flux: u32, lat: f32, lon: f32) -> f32 {
        // Simplified IRI-2020 model
        // MUF = base_freq * (1.0 + kp_factor * kp + flux_factor * flux + lat_factor * cos(lat))

        let base_freq = 15.0; // MHz baseline
        let kp_factor = 0.15;
        let flux_factor = 0.001;
        let lat_factor = 0.3;

        let muf = base_freq
            * (1.0 + kp_factor * kp)
            * (1.0 + flux_factor * (solar_flux as f32 - 70.0) / 70.0)
            * (1.0 + lat_factor * (lat.abs() / 90.0).cos());

        muf.max(1.0).min(50.0) // Clamp to realistic range
    }

    /// Generate MUF grid for visualization
    pub fn generate_muf_grid(kp: f32, solar_flux: u32) -> Vec<Vec<f32>> {
        let (lat_min, lat_max) = (-90.0, 90.0);
        let (lon_min, lon_max) = (-180.0, 180.0);
        let (lat_res, lon_res) = (18, 36); // 10° steps

        let mut grid = Vec::with_capacity(lat_res);

        for i in 0..lat_res {
            let lat = lat_min + (lat_max - lat_min) * (i as f32 / lat_res as f32);
            let mut row = Vec::with_capacity(lon_res);

            for j in 0..lon_res {
                let lon = lon_min + (lon_max - lon_min) * (j as f32 / lon_res as f32);
                let muf = Self::calculate_muf(kp, solar_flux, lat, lon);
                row.push(muf);
            }

            grid.push(row);
        }

        grid
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_muf_calculation() {
        let muf_quiet = ForecastFetcher::calculate_muf(2.0, 70, 45.0, -120.0);
        let muf_storm = ForecastFetcher::calculate_muf(8.0, 150, 45.0, -120.0);

        assert!(muf_storm > muf_quiet);
        assert!(muf_quiet > 5.0 && muf_quiet < 30.0);
    }

    #[test]
    fn test_muf_grid() {
        let grid = ForecastFetcher::generate_muf_grid(5.0, 100);
        assert_eq!(grid.len(), 18);
        assert_eq!(grid[0].len(), 36);
    }
}
```

**Integration Point:** `src/data/mod.rs`
```rust
pub mod forecast;
pub use forecast::{ForecastFetcher, HfForecast, MufForecast};
```

**Configuration:** `src/config.rs`
```toml
[forecasting]
forecast_enabled = true
forecast_update_interval_hours = 3  # NOAA updates 4x daily
muf_grid_resolution = 36  # 10° grid
band_prediction_enabled = true
```

---

### 10.2: Background Forecast Update Task

**File:** `src/data/mod.rs` (MODIFY, ~50 LOC)

Add to `AppData` struct:

```rust
#[derive(Debug, Clone)]
pub struct AppData {
    // Existing fields...
    pub hf_forecast: Option<HfForecast>,
    pub last_forecast_update: DateTime<Utc>,
}
```

**File:** `src/main.rs` (MODIFY, ~40 LOC)

Add forecast fetching task:

```rust
// After space weather task initialization
let data_clone = Arc::clone(&app_data);
let forecast_interval = config.forecast_update_interval_hours as u64;
let _forecast_task = tokio::spawn(async move {
    // Initial delay to let UI show
    tokio::time::sleep(Duration::from_millis(500)).await;

    loop {
        log::debug!("Fetching HF forecast...");

        match ForecastFetcher::fetch_hf_forecast().await {
            Ok(forecast) => {
                let mut data = data_clone.lock().await;
                data.hf_forecast = Some(forecast);
                data.last_forecast_update = Utc::now();
                log::info!("✓ HF forecast updated");
            }
            Err(e) => {
                log::warn!("Failed to fetch forecast: {}", e);
            }
        }

        tokio::time::sleep(Duration::from_secs(forecast_interval * 3600)).await;
    }
});
```

---

## Phase 11: Advanced Visualization (Days 4-7)

### 11.1: Layer Management System

**File:** `src/render/layers.rs` (NEW, ~250 LOC)

```rust
//! Layer management and rendering system

use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq)]
pub enum LayerType {
    BaseMap,        // Geographic base layer
    Greyline,       // Day/night terminator
    MufHeatmap,     // MUF color overlay
    AuroraOval,     // Aurora prediction overlay
    SatelliteFootprint,  // Satellite coverage zones
    GridSquares,    // Maidenhead grid
}

#[derive(Debug, Clone)]
pub struct Layer {
    pub layer_type: LayerType,
    pub visible: bool,
    pub opacity: f32,  // 0.0-1.0
    pub blend_mode: BlendMode,
}

#[derive(Debug, Clone, Copy)]
pub enum BlendMode {
    Opaque,
    Blend,
    Add,
    Screen,
}

#[derive(Debug, Clone)]
pub struct LayerManager {
    pub layers: Vec<Layer>,
    pub active_layer: LayerType,
}

impl Default for LayerManager {
    fn default() -> Self {
        Self {
            layers: vec![
                Layer {
                    layer_type: LayerType::BaseMap,
                    visible: true,
                    opacity: 1.0,
                    blend_mode: BlendMode::Opaque,
                },
                Layer {
                    layer_type: LayerType::Greyline,
                    visible: true,
                    opacity: 0.8,
                    blend_mode: BlendMode::Blend,
                },
                Layer {
                    layer_type: LayerType::MufHeatmap,
                    visible: false,
                    opacity: 0.6,
                    blend_mode: BlendMode::Add,
                },
                Layer {
                    layer_type: LayerType::AuroraOval,
                    visible: false,
                    opacity: 0.5,
                    blend_mode: BlendMode::Blend,
                },
                Layer {
                    layer_type: LayerType::SatelliteFootprint,
                    visible: true,
                    opacity: 0.4,
                    blend_mode: BlendMode::Blend,
                },
                Layer {
                    layer_type: LayerType::GridSquares,
                    visible: false,
                    opacity: 0.3,
                    blend_mode: BlendMode::Blend,
                },
            ],
            active_layer: LayerType::BaseMap,
        }
    }
}

impl LayerManager {
    pub fn toggle_layer(&mut self, layer_type: LayerType) {
        if let Some(layer) = self.layers.iter_mut().find(|l| l.layer_type == layer_type) {
            layer.visible = !layer.visible;
            log::info!("Layer {:?} toggled: {}", layer_type, layer.visible);
        }
    }

    pub fn set_opacity(&mut self, layer_type: LayerType, opacity: f32) {
        if let Some(layer) = self.layers.iter_mut().find(|l| l.layer_type == layer_type) {
            layer.opacity = opacity.clamp(0.0, 1.0);
        }
    }

    pub fn get_layer(&self, layer_type: LayerType) -> Option<&Layer> {
        self.layers.iter().find(|l| l.layer_type == layer_type)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_layer_toggle() {
        let mut manager = LayerManager::default();
        assert!(manager.get_layer(LayerType::MufHeatmap).unwrap().visible == false);

        manager.toggle_layer(LayerType::MufHeatmap);
        assert!(manager.get_layer(LayerType::MufHeatmap).unwrap().visible == true);
    }

    #[test]
    fn test_opacity_clamp() {
        let mut manager = LayerManager::default();
        manager.set_opacity(LayerType::BaseMap, 1.5);
        assert_eq!(manager.get_layer(LayerType::BaseMap).unwrap().opacity, 1.0);
    }
}
```

---

### 11.2: MUF Heat Map Rendering

**File:** `src/render/heatmap.rs` (NEW, ~200 LOC)

```rust
//! Heat map rendering for MUF visualization

use wgpu::*;
use std::sync::Arc;
use crate::data::forecast::MufForecast;

pub struct HeatmapRenderer {
    texture: Texture,
    view: TextureView,
    sampler: Sampler,
    pub width: u32,
    pub height: u32,
}

impl HeatmapRenderer {
    pub fn new(device: &Device, queue: &Queue, muf_data: &MufForecast) -> Self {
        let (width, height) = muf_data.resolution;

        // Convert MUF grid to RGBA texture
        let mut rgba_data = Vec::with_capacity((width * height * 4) as usize);

        for row in &muf_data.grid {
            for &muf_value in row {
                let color = Self::muf_to_color(muf_value);
                rgba_data.push(color.0);
                rgba_data.push(color.1);
                rgba_data.push(color.2);
                rgba_data.push(255); // Alpha
            }
        }

        // Create texture
        let texture = device.create_texture(&TextureDescriptor {
            label: Some("MUF Heatmap"),
            size: Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: TextureDimension::D2,
            format: TextureFormat::Rgba8UnormSrgb,
            usage: TextureUsages::TEXTURE_BINDING | TextureUsages::COPY_DST,
            view_formats: &[],
        });

        // Write texture data
        queue.write_texture(
            ImageCopyTexture {
                texture: &texture,
                mip_level: 0,
                origin: Origin3d::ZERO,
            },
            &rgba_data,
            ImageDataLayout {
                offset: 0,
                bytes_per_row: Some(width * 4),
                rows_per_image: Some(height),
            },
            Extent3d {
                width,
                height,
                depth_or_array_layers: 1,
            },
        );

        let view = texture.create_view(&TextureViewDescriptor::default());
        let sampler = device.create_sampler(&SamplerDescriptor {
            label: Some("MUF Sampler"),
            address_mode_u: AddressMode::ClampToEdge,
            address_mode_v: AddressMode::ClampToEdge,
            address_mode_w: AddressMode::ClampToEdge,
            mag_filter: FilterMode::Linear,
            min_filter: FilterMode::Linear,
            ..Default::default()
        });

        Self {
            texture,
            view,
            sampler,
            width,
            height,
        }
    }

    /// Convert MUF value (MHz) to RGBA color
    fn muf_to_color(muf: f32) -> (u8, u8, u8) {
        // Color scale: Blue (low) → Green → Yellow → Red (high)
        // MUF range: 0-50 MHz
        let normalized = (muf / 50.0).clamp(0.0, 1.0);

        if normalized < 0.25 {
            // Blue → Green
            let t = normalized / 0.25;
            let r = 0;
            let g = (t * 255.0) as u8;
            let b = (255.0 * (1.0 - t)) as u8;
            (r, g, b)
        } else if normalized < 0.5 {
            // Green → Yellow
            let t = (normalized - 0.25) / 0.25;
            let r = (t * 255.0) as u8;
            let g = 255;
            let b = 0;
            (r, g, b)
        } else if normalized < 0.75 {
            // Yellow → Orange
            let t = (normalized - 0.5) / 0.25;
            let r = 255;
            let g = (255.0 * (1.0 - t * 0.5)) as u8;
            let b = 0;
            (r, g, b)
        } else {
            // Orange → Red
            let r = 255;
            let g = (255.0 * (1.0 - normalized)) as u8;
            let b = 0;
            (r, g, b)
        }
    }

    pub fn texture_view(&self) -> &TextureView {
        &self.view
    }

    pub fn sampler(&self) -> &Sampler {
        &self.sampler
    }
}
```

---

### 11.3: Aurora Oval Rendering

**File:** `src/render/aurora.rs` (NEW, ~180 LOC)

```rust
//! Aurora oval prediction visualization

use nalgebra as na;
use chrono::Utc;

pub struct AuroraOval {
    pub latitude: f32,       // Center latitude of auroral oval
    pub longitude: f32,      // Center longitude of auroral oval
    pub width: f32,          // Width in degrees (typical 3-6°)
    pub intensity: f32,      // 0.0-1.0 (from Kp index)
}

impl AuroraOval {
    /// Calculate aurora oval position from Kp index
    pub fn from_kp(kp: f32) -> Self {
        // Empirical formula from NOAA/aurora models
        // Kp 0-3: Oval at 67° latitude
        // Kp 4-5: Oval at 65° latitude
        // Kp 6-7: Oval at 60° latitude
        // Kp 8-9: Oval at 55° latitude

        let latitude = match kp {
            k if k <= 3.0 => 67.0 - (k * 0.5),
            k if k <= 5.0 => 65.0 - ((k - 3.0) * 2.0),
            k if k <= 7.0 => 61.0 - ((k - 5.0) * 2.5),
            k if k <= 9.0 => 56.0 - ((k - 7.0) * 2.0),
            _ => 52.0,
        };

        let width = (kp / 9.0) * 8.0 + 2.0; // Range 2-10 degrees
        let intensity = (kp / 9.0).min(1.0);

        Self {
            latitude: latitude.max(45.0).min(75.0),
            longitude: 0.0, // Dawn side reference
            width: width.max(2.0).min(10.0),
            intensity,
        }
    }

    /// Generate aurora oval as series of latitude/longitude pairs
    pub fn generate_contour(&self, num_points: usize) -> Vec<(f32, f32)> {
        let mut points = Vec::with_capacity(num_points);

        for i in 0..num_points {
            let angle = (i as f32 / num_points as f32) * 2.0 * std::f32::consts::PI;

            // Distort oval slightly for realism (not perfectly circular)
            let distortion = 1.0 + 0.3 * angle.cos();
            let lat_offset = self.width * 0.5 * distortion;

            let lat = self.latitude - lat_offset;
            let lon = self.longitude + (180.0 / std::f32::consts::PI) * (angle.atan2(1.0));

            points.push((lat.clamp(-90.0, 90.0), lon.rem_euclid(360.0)));
        }

        points
    }

    /// Get aurora color based on intensity
    pub fn color(&self) -> [f32; 4] {
        match self.intensity {
            i if i < 0.2 => [0.0, 0.5, 0.0, 0.3],        // Dim green
            i if i < 0.4 => [0.0, 1.0, 0.0, 0.4],        // Green
            i if i < 0.6 => [0.0, 1.0, 1.0, 0.5],        // Cyan
            i if i < 0.8 => [1.0, 1.0, 0.0, 0.6],        // Yellow
            _ => [1.0, 0.0, 1.0, 0.7],                   // Magenta
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_aurora_from_kp() {
        let aurora_quiet = AuroraOval::from_kp(2.0);
        let aurora_storm = AuroraOval::from_kp(8.0);

        assert!(aurora_quiet.latitude > aurora_storm.latitude);
        assert!(aurora_storm.intensity > aurora_quiet.intensity);
    }

    #[test]
    fn test_aurora_contour() {
        let aurora = AuroraOval::from_kp(5.0);
        let contour = aurora.generate_contour(100);

        assert_eq!(contour.len(), 100);
        for (lat, lon) in contour {
            assert!(lat >= -90.0 && lat <= 90.0);
            assert!(lon >= 0.0 && lon < 360.0);
        }
    }
}
```

---

### 11.4: Keyboard Control for Layer Toggle

**File:** `src/main.rs` (MODIFY, ~60 LOC)

Add to event loop's `KeyboardInput` handler:

```rust
WindowEvent::KeyboardInput { event, .. } => {
    match event.logical_key {
        // Existing: Space (acknowledge), Escape (clear all)

        // NEW: Layer toggling via number keys
        winit::keyboard::Key::Character(c) => {
            match c.as_str() {
                "1" => {
                    log::info!("Toggle MUF Heat Map");
                    // TODO: Send layer toggle event
                }
                "2" => {
                    log::info!("Toggle Aurora Oval");
                    // TODO: Send layer toggle event
                }
                "3" => {
                    log::info!("Toggle Satellite Footprints");
                    // TODO: Send layer toggle event
                }
                "4" => {
                    log::info!("Toggle Maidenhead Grid");
                    // TODO: Send layer toggle event
                }
                _ => {}
            }
        }
        _ => {}
    }
}
```

---

### 11.5: Forecast Display UI

**File:** `src/render/gpu.rs` (MODIFY, ~150 LOC)

Add forecast display to `queue_ui_elements()`:

```rust
// Display 3-day forecast band status (if available)
if let Some(forecast) = &app_data.hf_forecast {
    let mut forecast_y = 450.0;

    // Title
    self.text_renderer.queue_text(
        "📊 Band Forecast (Next 24h)",
        [50.0, forecast_y],
        16.0,
        [1.0, 1.0, 0.0, 1.0], // Yellow
    );
    forecast_y += 20.0;

    // Show top 4 bands
    for band_forecast in forecast.band_forecast.iter().take(4) {
        let status_str = match band_forecast.forecast_status {
            BandStatus::Closed => "❌",
            BandStatus::Marginal => "⚠️",
            BandStatus::Open => "✅",
            BandStatus::Excellent => "⭐",
        };

        let color = match band_forecast.forecast_status {
            BandStatus::Closed => [0.5, 0.5, 0.5, 1.0],
            BandStatus::Marginal => [1.0, 1.0, 0.0, 1.0],
            BandStatus::Open => [0.0, 1.0, 0.0, 1.0],
            BandStatus::Excellent => [0.5, 1.0, 1.0, 1.0],
        };

        let forecast_text = format!(
            "{} {}: {} Mhz",
            status_str, band_forecast.band_name, band_forecast.frequency_mhz
        );

        self.text_renderer.queue_text(
            &forecast_text,
            [60.0, forecast_y],
            14.0,
            color,
        );

        forecast_y += 18.0;
    }
}

// Display keyboard shortcuts for layer controls
let mut shortcut_y = _height as f32 - 100.0;
self.text_renderer.queue_text(
    "Layer Controls: 1=MUF, 2=Aurora, 3=Satellites, 4=Grid",
    [50.0, shortcut_y],
    12.0,
    [0.7, 0.7, 0.7, 0.8],
);
```

---

## Testing Strategy

### Unit Tests
- [ ] MUF grid generation (verify size and value ranges)
- [ ] Aurora oval calculation (verify latitude/intensity correlation)
- [ ] Layer manager toggle/opacity operations
- [ ] Heat map color mapping (verify gradient)

### Integration Tests
- [ ] Forecast data fetching from NOAA API
- [ ] Layer rendering with different opacities
- [ ] Keyboard controls for layer toggle
- [ ] Multiple overlays rendering simultaneously

### Visual Tests
- [ ] MUF heat map displays correctly on map
- [ ] Aurora oval animation matches Kp changes
- [ ] Layer toggle keys work (1, 2, 3, 4)
- [ ] Forecast band status displays correctly
- [ ] No performance degradation with layers enabled

### Performance Tests
- [ ] Heat map generation < 100ms
- [ ] Layer rendering adds < 5% CPU overhead
- [ ] Aurora oval calculation < 50ms
- [ ] Multiple layers don't exceed 30 FPS target

---

## Success Criteria

| Criterion | Verification |
|-----------|--------------|
| MUF heat map renders | ✅ Blue-green-yellow-red gradient visible |
| 3-day forecast displays | ✅ Band status shows ❌/⚠️/✅/⭐ |
| Aurora oval moves with Kp | ✅ Latitude changes as Kp changes |
| Layer toggling works | ✅ Keys 1-4 show/hide layers |
| Performance maintained | ✅ < 3% CPU idle with layers |
| Competitive parity | ✅ Feature-complete vs Geochron |

---

## Implementation Order

1. **Day 1:** Forecast data structures + NOAA API integration
2. **Day 2:** MUF calculation + grid generation
3. **Day 3:** Heat map texture rendering
4. **Day 4:** Layer manager system
5. **Day 5:** Aurora oval rendering + keyboard controls
6. **Day 6:** Forecast display UI + integration
7. **Day 7:** Testing, optimization, documentation

---

## Data Source References

- **NOAA 3-Day Forecast:** https://services.swpc.noaa.gov/products/3-day-forecast.json
- **Kp Prediction:** https://services.swpc.noaa.gov/products/noaa-scales.json
- **Aurora Oval Model:** Based on NOAA/SWPC empirical models
- **MUF Calculation:** IRI-2020 simplified ionospheric model

---

## Future Enhancements (Post-Phase 11)

- **Real-time MUF vs predicted MUF comparison**
- **Band-specific opening/closing notifications**
- **Historical MUF/Kp trend graphs**
- **ITU region + call sign prefix overlays**
- **Maidenhead grid display**
- **Topographic map background**
- **Custom user overlays**
- **Mobile/touch support**

---

**Ready to begin? Current git status shows Phase 9 complete. Starting Phase 10 implementation now.**
