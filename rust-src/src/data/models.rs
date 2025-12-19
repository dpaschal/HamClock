//! Data models for HamClock

use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc};

/// Space weather data (solar activity)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SpaceWeather {
    pub kp: f32,           // Planetary K-index (0-9)
    pub a: i32,            // Planetary A-index
    pub ap: i32,           // Average Planetary A-index
    pub flux: i32,         // Solar flux
    pub updated: DateTime<Utc>,
}

impl Default for SpaceWeather {
    fn default() -> Self {
        Self {
            kp: 0.0,
            a: 0,
            ap: 0,
            flux: 0,
            updated: Utc::now(),
        }
    }
}

/// Weather forecast data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Forecast {
    pub date: String,
    pub temp_high: i32,
    pub temp_low: i32,
    pub conditions: String,
    pub humidity: i32,
}

/// DX spot from DX Cluster
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct DxSpot {
    pub frequency: f32,    // Frequency in MHz
    pub callsign: String,  // Spotted callsign
    pub spotter: String,   // Spotter callsign
    pub mode: String,      // Mode (SSB, CW, etc)
    pub time: DateTime<Utc>,
}

/// Satellite tracking data
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct SatelliteData {
    pub name: String,
    pub elevation: f32,    // Degrees above horizon
    pub azimuth: f32,      // Degrees from north
    pub range: f32,        // Distance in km
    pub doppler_shift: f32, // Doppler shift in Hz
}

/// Combined application data state
#[derive(Debug, Clone, Default)]
pub struct AppData {
    pub space_weather: SpaceWeather,
    pub forecast: Vec<Forecast>,
    pub dx_spots: Vec<DxSpot>,
    pub satellites: Vec<SatelliteData>,
    pub last_update: DateTime<Utc>,
}

impl AppData {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn update_timestamp(&mut self) {
        self.last_update = Utc::now();
    }
}
