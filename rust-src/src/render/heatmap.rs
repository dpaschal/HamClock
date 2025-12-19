//! MUF heat map rendering for frequency propagation visualization
//!
//! Converts MUF forecast data into a visual color-coded overlay
//! showing Maximum Usable Frequency across different latitudes.

use crate::data::forecast::MufForecast;

/// Color palette for MUF values (0-50 MHz)
pub struct MufColorPalette;

impl MufColorPalette {
    /// Map MUF value to RGBA color
    /// Blue (poor) → Green (fair) → Yellow (good) → Red (excellent)
    pub fn get_color(muf_mhz: f32) -> (u8, u8, u8, u8) {
        let normalized = (muf_mhz / 50.0).clamp(0.0, 1.0);

        // 5-color gradient
        let (r, g, b) = match normalized {
            n if n < 0.2 => {
                // Deep blue (0-10 MHz, poor propagation)
                let t = n / 0.2;
                (0, (50.0 * t) as u8, (150.0 * (1.0 - t * 0.5)) as u8)
            }
            n if n < 0.4 => {
                // Light blue to green (10-20 MHz, marginal)
                let t = (n - 0.2) / 0.2;
                (0, (150.0 + 50.0 * t) as u8, (150.0 - 50.0 * t) as u8)
            }
            n if n < 0.6 => {
                // Green (20-30 MHz, fair)
                let t = (n - 0.4) / 0.2;
                ((50.0 * t) as u8, 200, (100.0 - 100.0 * t) as u8)
            }
            n if n < 0.8 => {
                // Yellow (30-40 MHz, good)
                let t = (n - 0.6) / 0.2;
                (
                    (50.0 + 150.0 * t) as u8,
                    (200.0 - 50.0 * t) as u8,
                    0,
                )
            }
            _ => {
                // Orange to red (40-50 MHz, excellent)
                (200, 50, 0)
            }
        };

        (r, g, b, 180) // Semi-transparent
    }

    /// Get color description for MUF value
    pub fn description(muf_mhz: f32) -> &'static str {
        match muf_mhz {
            m if m < 10.0 => "Poor",
            m if m < 20.0 => "Marginal",
            m if m < 30.0 => "Fair",
            m if m < 40.0 => "Good",
            _ => "Excellent",
        }
    }
}

/// Heatmap texture generator from MUF forecast
pub struct MufHeatmapGenerator;

impl MufHeatmapGenerator {
    /// Generate RGBA texture data from MUF forecast grid
    pub fn generate_texture(forecast: &MufForecast) -> Vec<u8> {
        let (width, height) = forecast.resolution;
        let mut rgba_data = vec![0u8; (width * height * 4) as usize];

        for (i, row) in forecast.grid.iter().enumerate() {
            for (j, &muf_value) in row.iter().enumerate() {
                let idx = (i * width as usize + j) * 4;
                let (r, g, b, a) = MufColorPalette::get_color(muf_value);

                rgba_data[idx] = r;
                rgba_data[idx + 1] = g;
                rgba_data[idx + 2] = b;
                rgba_data[idx + 3] = a;
            }
        }

        rgba_data
    }

    /// Get texture dimensions
    pub fn dimensions(forecast: &MufForecast) -> (u32, u32) {
        forecast.resolution
    }

    /// Get latitude/longitude ranges
    pub fn coordinate_bounds(forecast: &MufForecast) -> ((f32, f32), (f32, f32)) {
        (forecast.latitude_range, forecast.longitude_range)
    }

    /// Map pixel coordinates to lat/lon
    pub fn pixel_to_coordinates(
        pixel_x: u32,
        pixel_y: u32,
        forecast: &MufForecast,
    ) -> (f32, f32) {
        let (width, height) = forecast.resolution;
        let (lat_min, lat_max) = forecast.latitude_range;
        let (lon_min, lon_max) = forecast.longitude_range;

        let lat = lat_max - (pixel_y as f32 / height as f32) * (lat_max - lat_min);
        let lon = lon_min + (pixel_x as f32 / width as f32) * (lon_max - lon_min);

        (lat, lon)
    }

    /// Map lat/lon to pixel coordinates
    pub fn coordinates_to_pixel(
        lat: f32,
        lon: f32,
        forecast: &MufForecast,
    ) -> (u32, u32) {
        let (width, height) = forecast.resolution;
        let (lat_min, lat_max) = forecast.latitude_range;
        let (lon_min, lon_max) = forecast.longitude_range;

        let x = ((lon - lon_min) / (lon_max - lon_min) * width as f32) as u32;
        let y = ((lat_max - lat) / (lat_max - lat_min) * height as f32) as u32;

        (x.min(width - 1), y.min(height - 1))
    }

    /// Get MUF value at specific coordinates
    pub fn get_muf_at(
        lat: f32,
        lon: f32,
        forecast: &MufForecast,
    ) -> Option<f32> {
        let (x, y) = Self::coordinates_to_pixel(lat, lon, forecast);
        let (width, _height) = forecast.resolution;

        if (x as usize) < forecast.grid[0].len() && (y as usize) < forecast.grid.len() {
            Some(forecast.grid[y as usize][x as usize])
        } else {
            None
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_color_gradient() {
        let (r, g, b, _) = MufColorPalette::get_color(5.0);   // Poor
        assert!(b > g && b > r); // Should be blueish

        let (r, g, b, _) = MufColorPalette::get_color(25.0);  // Fair
        assert!(g > r && g > b); // Should be greenish

        let (r, g, b, _) = MufColorPalette::get_color(45.0);  // Excellent
        assert!(r > g && r > b); // Should be reddish
    }

    #[test]
    fn test_color_description() {
        assert_eq!(MufColorPalette::description(5.0), "Poor");
        assert_eq!(MufColorPalette::description(15.0), "Marginal");
        assert_eq!(MufColorPalette::description(25.0), "Fair");
        assert_eq!(MufColorPalette::description(35.0), "Good");
        assert_eq!(MufColorPalette::description(45.0), "Excellent");
    }

    #[test]
    fn test_coordinate_conversion() {
        let forecast = crate::data::forecast::MufForecast {
            grid: vec![vec![15.0; 36]; 18],
            latitude_range: (-90.0, 90.0),
            longitude_range: (-180.0, 180.0),
            resolution: (36, 18),
            valid_at: chrono::Utc::now(),
        };

        // Equator, prime meridian
        let (x, y) = MufHeatmapGenerator::coordinates_to_pixel(0.0, 0.0, &forecast);
        assert_eq!(x, 18); // Middle
        assert_eq!(y, 9);  // Middle

        // North pole
        let (x, y) = MufHeatmapGenerator::coordinates_to_pixel(90.0, 0.0, &forecast);
        assert_eq!(y, 0);

        // South pole
        let (x, y) = MufHeatmapGenerator::coordinates_to_pixel(-90.0, 0.0, &forecast);
        assert_eq!(y, 17);
    }

    #[test]
    fn test_texture_generation() {
        let forecast = crate::data::forecast::MufForecast {
            grid: vec![vec![20.0; 36]; 18],
            latitude_range: (-90.0, 90.0),
            longitude_range: (-180.0, 180.0),
            resolution: (36, 18),
            valid_at: chrono::Utc::now(),
        };

        let texture = MufHeatmapGenerator::generate_texture(&forecast);
        assert_eq!(texture.len(), 36 * 18 * 4);

        // Check a pixel (should be green for 20 MHz)
        let pixel_r = texture[0];
        let pixel_g = texture[1];
        let pixel_b = texture[2];
        let pixel_a = texture[3];

        assert!(pixel_g > 0);  // Green dominant
        assert!(pixel_a > 0);  // Has alpha
    }
}
