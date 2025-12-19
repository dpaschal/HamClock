//! Data models for HamClock

use serde::{Deserialize, Serialize};
use chrono::{DateTime, Utc};
use std::collections::HashMap;

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

/// Alert severity levels (Phase 8)
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq, PartialOrd, Ord)]
pub enum AlertSeverity {
    Info,      // Light blue
    Notice,    // Yellow
    Warning,   // Orange
    Critical,  // Red
    Emergency, // Magenta
}

/// Alert type classification (Phase 8)
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub enum AlertType {
    DxSpot,
    SatellitePass,
    KpSpike,
    XrayFlare,
    Aurora,
    Cme,
}

/// Individual alert (Phase 8)
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Alert {
    pub id: String,
    pub alert_type: AlertType,
    pub severity: AlertSeverity,
    pub message: String,
    pub created_at: DateTime<Utc>,
    pub expires_at: DateTime<Utc>,
    pub acknowledged: bool,
}

impl Alert {
    pub fn new(
        alert_type: AlertType,
        severity: AlertSeverity,
        message: String,
        duration_seconds: u64,
    ) -> Self {
        let now = Utc::now();
        let id = format!("{:?}-{}", alert_type, now.timestamp());

        Self {
            id,
            alert_type,
            severity,
            message,
            created_at: now,
            expires_at: now + chrono::Duration::seconds(duration_seconds as i64),
            acknowledged: false,
        }
    }

    pub fn is_expired(&self) -> bool {
        Utc::now() > self.expires_at
    }

    pub fn is_active(&self) -> bool {
        !self.is_expired() && !self.acknowledged
    }
}

/// Alert system state tracking (Phase 8)
#[derive(Debug, Clone, Default)]
pub struct AlertState {
    pub active_alerts: Vec<Alert>,
    pub last_kp: f32,
    pub last_dx_spots: Vec<String>,
    pub last_satellite_elevations: HashMap<String, f32>,
    pub last_xray_class: String,
}

impl AlertState {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn cleanup_expired(&mut self) {
        self.active_alerts.retain(|a| a.is_active());
    }

    pub fn add_alert(&mut self, alert: Alert) {
        let is_duplicate = self.active_alerts.iter().any(|a| {
            a.alert_type == alert.alert_type &&
            a.is_active() &&
            (alert.created_at - a.created_at).num_minutes() < 5
        });

        if !is_duplicate {
            self.active_alerts.push(alert);
        }
    }
}

/// Combined application data state
#[derive(Debug, Clone, Default)]
pub struct AppData {
    pub space_weather: SpaceWeather,
    pub forecast: Vec<Forecast>,
    pub dx_spots: Vec<DxSpot>,
    pub satellites: Vec<SatelliteData>,
    pub last_update: DateTime<Utc>,
    pub alert_state: AlertState, // Phase 8
}

impl AppData {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn update_timestamp(&mut self) {
        self.last_update = Utc::now();
    }
}
