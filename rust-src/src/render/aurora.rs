//! Aurora oval prediction visualization
//!
//! Renders the auroral oval position based on Kp index,
//! showing predicted geographic location of northern lights.

/// Aurora oval visualization and prediction
pub struct AuroraOvalRenderer {
    /// Latitude of aurora oval center (north magnetic pole)
    latitude: f32,
    /// Width of oval in degrees
    width: f32,
    /// Intensity level (0.0-1.0)
    intensity: f32,
}

impl AuroraOvalRenderer {
    /// Create aurora oval from Kp index
    /// Uses empirical NOAA aurora forecast model
    pub fn from_kp(kp: f32) -> Self {
        // Auroral oval latitude estimation
        // Based on NOAA Space Weather Prediction Center empirical data
        let latitude = match kp {
            k if k <= 0.0 => 68.0,
            k if k <= 1.0 => 67.5 - (k * 0.5),
            k if k <= 3.0 => 67.0 - (k * 0.3),
            k if k <= 5.0 => 65.0 - ((k - 3.0) * 1.5),
            k if k <= 6.0 => 62.0 - ((k - 5.0) * 2.0),
            k if k <= 7.0 => 60.0 - ((k - 6.0) * 2.5),
            k if k <= 8.0 => 57.5 - ((k - 7.0) * 2.5),
            k if k <= 9.0 => 55.0 - ((k - 8.0) * 2.0),
            _ => 53.0,
        };

        // Width of auroral oval
        let width = match kp {
            k if k < 3.0 => 3.0 + k * 0.5,
            k if k < 6.0 => 4.5 + (k - 3.0) * 1.0,
            k if k < 9.0 => 7.5 + (k - 6.0) * 0.8,
            _ => 10.0,
        };

        let intensity = (kp / 9.0).clamp(0.0, 1.0);

        Self {
            latitude: latitude.clamp(45.0, 75.0),
            width: width.clamp(2.0, 15.0),
            intensity,
        }
    }

    /// Get color for aurora based on intensity
    pub fn color(&self) -> [f32; 4] {
        match self.intensity {
            i if i < 0.1 => [0.0, 0.3, 0.0, 0.15],      // Very dim green
            i if i < 0.2 => [0.0, 0.5, 0.0, 0.25],      // Dim green
            i if i < 0.35 => [0.0, 0.8, 0.0, 0.35],     // Green
            i if i < 0.5 => [0.0, 1.0, 0.3, 0.45],      // Yellow-green
            i if i < 0.65 => [0.0, 1.0, 1.0, 0.50],     // Cyan
            i if i < 0.75 => [0.5, 1.0, 0.0, 0.55],     // Yellow
            i if i < 0.85 => [1.0, 0.8, 0.0, 0.60],     // Orange
            i if i < 0.95 => [1.0, 0.5, 0.0, 0.65],     // Red-orange
            _ => [1.0, 0.0, 1.0, 0.70],                 // Magenta (extreme)
        }
    }

    /// Get contour points for aurora oval
    pub fn generate_contour(&self, num_points: usize) -> Vec<(f32, f32)> {
        let mut contour = Vec::with_capacity(num_points);

        for i in 0..num_points {
            let angle = (i as f32 / num_points as f32) * 2.0 * std::f32::consts::PI;

            // Distort oval - not perfectly circular
            // More distorted at night side (angle 180°)
            let distortion_factor = 1.0 + 0.4 * angle.cos();
            let lat_offset = self.width * 0.5 * distortion_factor;

            let lat = (self.latitude - lat_offset).clamp(-90.0, 90.0);

            // Longitude follows local solar time
            // Dawn side: -90°, Dusk side: +90°, Noon side: 0°, Midnight: 180°
            let lon = (i as f32 / num_points as f32) * 360.0 - 180.0;

            contour.push((lat, lon));
        }

        contour
    }

    /// Get aurora oval parameters
    pub fn latitude(&self) -> f32 {
        self.latitude
    }

    pub fn width(&self) -> f32 {
        self.width
    }

    pub fn intensity(&self) -> f32 {
        self.intensity
    }

    /// Determine if location is within aurora oval
    pub fn is_in_oval(&self, lat: f32, lon: f32) -> bool {
        if lat < self.latitude - self.width * 0.75 {
            return false; // Too far south
        }

        if lat > self.latitude + self.width * 0.25 {
            return false; // Too far north
        }

        // Simple lat check (more complex models would use geomagnetic coordinates)
        true
    }

    /// Aurora status description
    pub fn status(&self) -> &'static str {
        match self.intensity {
            i if i < 0.1 => "Not visible",
            i if i < 0.2 => "Very unlikely",
            i if i < 0.35 => "Unlikely",
            i if i < 0.5 => "Possible",
            i if i < 0.65 => "Likely",
            i if i < 0.75 => "Very likely",
            i if i < 0.85 => "Probable",
            i if i < 0.95 => "Highly probable",
            _ => "Certain (extreme storm)",
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_aurora_from_quiet_kp() {
        let aurora = AuroraOvalRenderer::from_kp(1.0);
        assert!(aurora.latitude > 65.0);
        assert!(aurora.intensity < 0.2);
        assert_eq!(aurora.status(), "Very unlikely");
    }

    #[test]
    fn test_aurora_from_active_kp() {
        let aurora = AuroraOvalRenderer::from_kp(5.0);
        assert!(aurora.latitude > 60.0 && aurora.latitude < 65.0);
        assert!(aurora.intensity > 0.4 && aurora.intensity < 0.6);
        assert!(["Likely", "Possible"].contains(&aurora.status()));
    }

    #[test]
    fn test_aurora_from_storm_kp() {
        let aurora = AuroraOvalRenderer::from_kp(8.0);
        assert!(aurora.latitude > 50.0 && aurora.latitude < 60.0);
        assert!(aurora.intensity > 0.8);
        assert!(["Highly probable", "Probable"].contains(&aurora.status()));
    }

    #[test]
    fn test_aurora_contour() {
        let aurora = AuroraOvalRenderer::from_kp(5.0);
        let contour = aurora.generate_contour(360);

        assert_eq!(contour.len(), 360);

        for (lat, lon) in contour {
            assert!(lat >= -90.0 && lat <= 90.0);
            assert!(lon >= -180.0 && lon <= 180.0);
        }
    }

    #[test]
    fn test_aurora_color() {
        let aurora_quiet = AuroraOvalRenderer::from_kp(1.0);
        let aurora_storm = AuroraOvalRenderer::from_kp(8.0);

        let color_quiet = aurora_quiet.color();
        let color_storm = aurora_storm.color();

        // Quiet should be more green
        assert!(color_quiet[1] > color_quiet[0]);

        // Storm should be more red/magenta
        assert!(color_storm[0] >= color_storm[1]);
    }

    #[test]
    fn test_aurora_in_oval() {
        let aurora = AuroraOvalRenderer::from_kp(5.0);

        // Should be in oval
        assert!(aurora.is_in_oval(aurora.latitude, 0.0));

        // Should not be in oval (too far south)
        assert!(!aurora.is_in_oval(aurora.latitude - aurora.width, 0.0));
    }
}
