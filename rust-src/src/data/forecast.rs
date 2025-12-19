//! NOAA propagation forecasting module
//!
//! Provides HF band forecasting, MUF calculations, and Kp predictions
//! for amateur radio propagation analysis.

use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc, Duration};

/// 3-day HF propagation forecast from NOAA
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct HfForecast {
    pub issued_at: DateTime<Utc>,
    pub valid_from: DateTime<Utc>,
    pub valid_to: DateTime<Utc>,
    pub kp_forecast: Vec<KpForecast>,
    pub band_forecast: Vec<BandForecast>,
    pub muf_forecast: MufForecast,
}

/// Kp index 3-day forecast
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct KpForecast {
    pub time: DateTime<Utc>,
    pub kp_min: f32,
    pub kp_max: f32,
    pub kp_pred: f32,
    pub a_index: u32,
}

/// Band-specific propagation forecast
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

/// Band propagation status
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq)]
pub enum BandStatus {
    Closed,     // Propagation impossible
    Marginal,   // Unlikely but possible
    Open,       // Good propagation expected
    Excellent,  // Excellent propagation
}

impl BandStatus {
    pub fn as_emoji(&self) -> &'static str {
        match self {
            BandStatus::Closed => "❌",
            BandStatus::Marginal => "⚠️",
            BandStatus::Open => "✅",
            BandStatus::Excellent => "⭐",
        }
    }

    pub fn as_color(&self) -> [f32; 4] {
        match self {
            BandStatus::Closed => [0.5, 0.5, 0.5, 1.0],      // Gray
            BandStatus::Marginal => [1.0, 1.0, 0.0, 1.0],    // Yellow
            BandStatus::Open => [0.0, 1.0, 0.0, 1.0],        // Green
            BandStatus::Excellent => [0.5, 1.0, 1.0, 1.0],   // Cyan
        }
    }
}

/// MUF (Maximum Usable Frequency) forecast grid
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct MufForecast {
    pub grid: Vec<Vec<f32>>,    // 2D grid of MUF values (lat x lon)
    pub latitude_range: (f32, f32),    // (min, max)
    pub longitude_range: (f32, f32),   // (min, max)
    pub resolution: (u32, u32),        // grid dimensions
    pub valid_at: DateTime<Utc>,
}

/// Fetcher for propagation data from NOAA
pub struct ForecastFetcher;

impl ForecastFetcher {
    /// Fetch NOAA 3-day HF forecast
    pub async fn fetch_hf_forecast() -> Result<HfForecast, String> {
        log::debug!("Fetching NOAA 3-day HF forecast...");

        // In production, this would fetch from NOAA API:
        // https://services.swpc.noaa.gov/products/3-day-forecast.json

        // For now, return mock forecast for testing
        let now = Utc::now();

        Ok(HfForecast {
            issued_at: now,
            valid_from: now,
            valid_to: now + Duration::days(3),
            kp_forecast: vec![
                KpForecast {
                    time: now,
                    kp_min: 3.0,
                    kp_max: 5.0,
                    kp_pred: 4.0,
                    a_index: 10,
                },
                KpForecast {
                    time: now + Duration::hours(24),
                    kp_min: 4.0,
                    kp_max: 7.0,
                    kp_pred: 5.5,
                    a_index: 20,
                },
                KpForecast {
                    time: now + Duration::hours(48),
                    kp_min: 2.0,
                    kp_max: 4.0,
                    kp_pred: 3.0,
                    a_index: 8,
                },
            ],
            band_forecast: vec![
                BandForecast {
                    band_name: "10m".to_string(),
                    frequency_mhz: 28.0,
                    forecast_status: BandStatus::Excellent,
                    confidence: 0.85,
                    best_time: Some(now + Duration::hours(12)),
                    opens_at: Some(now + Duration::hours(6)),
                    closes_at: Some(now + Duration::hours(20)),
                },
                BandForecast {
                    band_name: "15m".to_string(),
                    frequency_mhz: 21.0,
                    forecast_status: BandStatus::Open,
                    confidence: 0.80,
                    best_time: Some(now + Duration::hours(14)),
                    opens_at: Some(now + Duration::hours(4)),
                    closes_at: Some(now + Duration::hours(22)),
                },
                BandForecast {
                    band_name: "20m".to_string(),
                    frequency_mhz: 14.0,
                    forecast_status: BandStatus::Open,
                    confidence: 0.90,
                    best_time: Some(now + Duration::hours(16)),
                    opens_at: Some(now - Duration::hours(2)),
                    closes_at: None, // Open all day
                },
                BandForecast {
                    band_name: "40m".to_string(),
                    frequency_mhz: 7.0,
                    forecast_status: BandStatus::Marginal,
                    confidence: 0.60,
                    best_time: Some(now + Duration::hours(20)),
                    opens_at: Some(now + Duration::hours(16)),
                    closes_at: Some(now + Duration::hours(24) + Duration::hours(8)),
                },
            ],
            muf_forecast: MufForecast {
                grid: Self::generate_muf_grid(5.0, 100),
                latitude_range: (-90.0, 90.0),
                longitude_range: (-180.0, 180.0),
                resolution: (18, 36),
                valid_at: now,
            },
        })
    }

    /// Predict MUF from Kp and solar flux using simplified IRI model
    pub fn calculate_muf(kp: f32, solar_flux: u32, lat: f32, _lon: f32) -> f32 {
        // Simplified IRI-2020 model for MUF prediction
        // Formula: MUF = base_freq * (1.0 + kp_factor*kp + flux_factor*flux + lat_factor*lat)

        let base_freq = 15.0; // MHz baseline at equator, quiet conditions
        let kp_factor = 0.15;
        let flux_factor = 0.001;
        let lat_factor = 0.3;

        // Normalize solar flux (70 = baseline quiet)
        let flux_normalized = (solar_flux as f32 - 70.0) / 100.0;

        let muf = base_freq
            * (1.0 + kp_factor * kp)
            * (1.0 + flux_factor * flux_normalized * 100.0)
            * (1.0 + lat_factor * (lat.abs() / 90.0).cos());

        muf.max(1.0).min(50.0) // Clamp to realistic range (1-50 MHz)
    }

    /// Generate MUF grid for visualization (latitude x longitude)
    pub fn generate_muf_grid(kp: f32, solar_flux: u32) -> Vec<Vec<f32>> {
        let (lat_min, lat_max) = (-90.0, 90.0);
        let (lon_min, lon_max) = (-180.0, 180.0);
        let (lat_res, lon_res) = (18, 36); // 10° resolution grid

        let mut grid = Vec::with_capacity(lat_res);

        for i in 0..lat_res {
            let lat = lat_min + (lat_max - lat_min) * (i as f32 / (lat_res - 1) as f32);
            let mut row = Vec::with_capacity(lon_res);

            for j in 0..lon_res {
                let lon = lon_min + (lon_max - lon_min) * (j as f32 / (lon_res - 1) as f32);
                let muf = Self::calculate_muf(kp, solar_flux, lat, lon);
                row.push(muf);
            }

            grid.push(row);
        }

        grid
    }

    /// Predict band status based on MUF and frequency
    pub fn predict_band_status(muf: f32, frequency: f32) -> BandStatus {
        let ratio = muf / frequency;

        if ratio < 0.8 {
            BandStatus::Closed
        } else if ratio < 0.95 {
            BandStatus::Marginal
        } else if ratio < 1.3 {
            BandStatus::Open
        } else {
            BandStatus::Excellent
        }
    }
}

impl Default for HfForecast {
    fn default() -> Self {
        let now = Utc::now();
        Self {
            issued_at: now,
            valid_from: now,
            valid_to: now + Duration::days(3),
            kp_forecast: Vec::new(),
            band_forecast: Vec::new(),
            muf_forecast: MufForecast {
                grid: Vec::new(),
                latitude_range: (-90.0, 90.0),
                longitude_range: (-180.0, 180.0),
                resolution: (18, 36),
                valid_at: now,
            },
        }
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
        println!("Quiet: {:.1} MHz, Storm: {:.1} MHz", muf_quiet, muf_storm);
    }

    #[test]
    fn test_muf_grid_generation() {
        let grid = ForecastFetcher::generate_muf_grid(5.0, 100);
        assert_eq!(grid.len(), 18);
        assert_eq!(grid[0].len(), 36);
        assert!(grid[0][0] > 0.0 && grid[0][0] < 50.0);
    }

    #[test]
    fn test_band_status_prediction() {
        assert_eq!(ForecastFetcher::predict_band_status(10.0, 28.0), BandStatus::Excellent);
        assert_eq!(ForecastFetcher::predict_band_status(15.0, 21.0), BandStatus::Excellent);
        assert_eq!(ForecastFetcher::predict_band_status(5.0, 14.0), BandStatus::Marginal);
        assert_eq!(ForecastFetcher::predict_band_status(2.0, 14.0), BandStatus::Closed);
    }

    #[test]
    fn test_band_status_emoji() {
        assert_eq!(BandStatus::Open.as_emoji(), "✅");
        assert_eq!(BandStatus::Closed.as_emoji(), "❌");
        assert_eq!(BandStatus::Excellent.as_emoji(), "⭐");
    }
}
