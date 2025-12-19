//! Maidenhead locator grid rendering
//!
//! Displays grid squares for VHF/UHF contesting and location identification.
//! The Maidenhead Locator System divides Earth into hierarchical grid squares.

/// Maidenhead grid line for rendering
pub struct GridLine {
    pub lat1: f32,
    pub lon1: f32,
    pub lat2: f32,
    pub lon2: f32,
    pub is_major: bool,  // Major (field) vs minor (square) line
    pub label: Option<String>,
}

/// Maidenhead grid renderer
pub struct MaidenheadGridRenderer;

impl MaidenheadGridRenderer {
    /// Generate grid lines at specified resolution level
    /// Level 1: Fields (20° × 10°)
    /// Level 2: Squares (2° × 1°)
    pub fn generate_lines(level: u32) -> Vec<GridLine> {
        let mut lines = Vec::new();

        match level {
            1 => {
                // Field level: 18 × 9 grid
                // Longitude: 18 fields of 20° each (-180° to +180°)
                for lon_field in 0..18 {
                    let lon1 = -180.0 + (lon_field as f32) * 20.0;
                    let lon2 = lon1 + 20.0;

                    // Latitude: 9 fields of 10° each (-90° to +90°)
                    for lat_field in 0..9 {
                        let lat1 = -90.0 + (lat_field as f32) * 10.0;
                        let lat2 = lat1 + 10.0;

                        // Vertical lines (longitude)
                        lines.push(GridLine {
                            lat1,
                            lon1,
                            lat2,
                            lon2: lon1,
                            is_major: true,
                            label: None,
                        });

                        // Horizontal lines (latitude)
                        lines.push(GridLine {
                            lat1,
                            lon1,
                            lat2: lat1,
                            lon2,
                            is_major: true,
                            label: None,
                        });

                        // Field labels at corners
                        if lat_field == 0 && lon_field == 0 {
                            lines.push(GridLine {
                                lat1: lat1 + 5.0,
                                lon1: lon1 + 10.0,
                                lat2: lat1 + 5.0,
                                lon2: lon1 + 10.0,
                                is_major: true,
                                label: Some(Self::field_to_string(lon_field, lat_field)),
                            });
                        }
                    }
                }
            }
            2 => {
                // Square level: 2° × 1° grid
                // Each field subdivided into 10 × 10 squares
                for lon_field in 0..18 {
                    for lat_field in 0..9 {
                        for lon_square in 0..10 {
                            for lat_square in 0..10 {
                                let lon1 = -180.0 + (lon_field as f32) * 20.0 + (lon_square as f32) * 2.0;
                                let lon2 = lon1 + 2.0;
                                let lat1 = -90.0 + (lat_field as f32) * 10.0 + (lat_square as f32) * 1.0;
                                let lat2 = lat1 + 1.0;

                                // Vertical lines
                                lines.push(GridLine {
                                    lat1,
                                    lon1,
                                    lat2,
                                    lon2: lon1,
                                    is_major: false,
                                    label: None,
                                });

                                // Horizontal lines
                                lines.push(GridLine {
                                    lat1,
                                    lon1,
                                    lat2: lat1,
                                    lon2,
                                    is_major: false,
                                    label: None,
                                });
                            }
                        }
                    }
                }
            }
            _ => {}
        }

        lines
    }

    /// Convert field coordinates to letter designation (AA-RR)
    fn field_to_string(lon_field: u32, lat_field: u32) -> String {
        let lon_char = ((lon_field % 18) as u8 + b'A') as char;
        let lat_char = ((lat_field % 9) as u8 + b'A') as char;
        format!("{}{}", lon_char, lat_char)
    }

    /// Convert square coordinates to locator (e.g., "JN86")
    fn square_to_string(
        lon_field: u32,
        lat_field: u32,
        lon_square: u32,
        lat_square: u32,
    ) -> String {
        let field_str = Self::field_to_string(lon_field, lat_field);
        let square_str = format!("{}{}", lon_square % 10, lat_square % 10);
        format!("{}{}", field_str, square_str)
    }

    /// Get Maidenhead locator for lat/lon
    /// Example: 52.5°N, 13.4°E → JO61
    pub fn locator_from_coordinates(lat: f32, lon: f32) -> String {
        let lon_norm = lon + 180.0;
        let lat_norm = lat + 90.0;

        let lon_field = (lon_norm / 20.0).floor() as u32 % 18;
        let lat_field = (lat_norm / 10.0).floor() as u32 % 9;

        let lon_rem = (lon_norm % 20.0) / 2.0;
        let lat_rem = (lat_norm % 10.0) / 1.0;

        let lon_square = lon_rem.floor() as u32 % 10;
        let lat_square = lat_rem.floor() as u32 % 10;

        Self::square_to_string(lon_field, lat_field, lon_square, lat_square)
    }

    /// Get approximate center coordinates of a locator
    /// Example: "JN86" → (52.5, 13.4)
    pub fn coordinates_from_locator(locator: &str) -> Option<(f32, f32)> {
        if locator.len() < 4 {
            return None;
        }

        let chars: Vec<char> = locator.chars().collect();

        // Parse field (first two letters)
        let lon_field = ((chars[0] as u32 - 'A' as u32) % 18) as f32;
        let lat_field = ((chars[1] as u32 - 'A' as u32) % 9) as f32;

        // Parse square (digits)
        let lon_square = (chars[2].to_digit(10)? as f32) % 10.0;
        let lat_square = (chars[3].to_digit(10)? as f32) % 10.0;

        // Calculate center coordinates
        let lon = -180.0 + (lon_field * 20.0) + (lon_square * 2.0) + 1.0;
        let lat = -90.0 + (lat_field * 10.0) + (lat_square * 1.0) + 0.5;

        Some((lat, lon))
    }

    /// Get grid color (less visible, background layer)
    pub fn grid_color(is_major: bool) -> [f32; 4] {
        if is_major {
            [0.5, 0.5, 1.0, 0.25]  // Light blue, major lines
        } else {
            [0.3, 0.3, 0.7, 0.15]  // Dimmer blue, minor lines
        }
    }

    /// Get label color
    pub fn label_color() -> [f32; 4] {
        [0.6, 0.6, 1.0, 0.5]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_field_to_string() {
        assert_eq!(MaidenheadGridRenderer::field_to_string(0, 0), "AA");
        assert_eq!(MaidenheadGridRenderer::field_to_string(9, 4), "JE");
        assert_eq!(MaidenheadGridRenderer::field_to_string(17, 8), "RR");
    }

    #[test]
    fn test_square_to_string() {
        assert_eq!(
            MaidenheadGridRenderer::square_to_string(9, 4, 3, 5),
            "JE35"
        );
    }

    #[test]
    fn test_locator_from_coordinates() {
        // Berlin, Germany: approximately JO61
        let locator = MaidenheadGridRenderer::locator_from_coordinates(52.5, 13.4);
        assert!(locator.starts_with("JO"));

        // Equator, Prime Meridian: approximately II05
        let locator = MaidenheadGridRenderer::locator_from_coordinates(0.5, 0.5);
        assert_eq!(locator.len(), 4);

        // Check format
        let locator = MaidenheadGridRenderer::locator_from_coordinates(-45.0, 170.0);
        assert_eq!(locator.len(), 4);
    }

    #[test]
    fn test_coordinates_from_locator() {
        // "JO61" should decode to somewhere around Berlin
        let (lat, lon) = MaidenheadGridRenderer::coordinates_from_locator("JO61")
            .expect("Valid locator");
        assert!(lat > 50.0 && lat < 55.0);
        assert!(lon > 10.0 && lon < 15.0);
    }

    #[test]
    fn test_locator_roundtrip() {
        let orig_lat = 52.5;
        let orig_lon = 13.4;

        let locator = MaidenheadGridRenderer::locator_from_coordinates(orig_lat, orig_lon);
        let (new_lat, new_lon) =
            MaidenheadGridRenderer::coordinates_from_locator(&locator).expect("Valid locator");

        // Should be within 1 degree (locator precision)
        assert!((orig_lat - new_lat).abs() < 1.0);
        assert!((orig_lon - new_lon).abs() < 2.0);
    }

    #[test]
    fn test_grid_lines_generation() {
        let lines = MaidenheadGridRenderer::generate_lines(1);
        assert!(!lines.is_empty());

        let lines = MaidenheadGridRenderer::generate_lines(2);
        assert!(!lines.is_empty());
    }

    #[test]
    fn test_grid_colors() {
        let major = MaidenheadGridRenderer::grid_color(true);
        let minor = MaidenheadGridRenderer::grid_color(false);

        // Major should be more visible
        assert!(major[3] > minor[3]);
    }
}
