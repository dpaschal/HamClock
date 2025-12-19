//! Configuration for HamClock

use serde::{Deserialize, Serialize};
use crate::AppResult;
use std::path::PathBuf;
use crate::data::models::AlertSeverity;

/// Phase 9: Alert Extensions configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Phase9Config {
    // Alert History (SQLite)
    pub history_enabled: bool,
    pub history_db_path: PathBuf,
    pub history_retention_days: u32,
    pub history_max_entries: usize,

    // Desktop Notifications
    pub desktop_notifications_enabled: bool,
    pub notification_timeout_seconds: u32,
    pub notification_min_severity: AlertSeverity,

    // MQTT Publishing
    pub mqtt_enabled: bool,
    pub mqtt_broker_url: String,
    pub mqtt_client_id: String,
    pub mqtt_topic_prefix: String,
    pub mqtt_qos: u8,

    // Web Dashboard
    pub web_dashboard_enabled: bool,
    pub web_dashboard_port: u16,
    pub web_dashboard_host: String,
}

impl Default for Phase9Config {
    fn default() -> Self {
        Self {
            history_enabled: true,
            history_db_path: PathBuf::from("~/.hamclock/alerts.db"),
            history_retention_days: 30,
            history_max_entries: 10000,

            desktop_notifications_enabled: false,
            notification_timeout_seconds: 10,
            notification_min_severity: AlertSeverity::Warning,

            mqtt_enabled: false,
            mqtt_broker_url: "mqtt://localhost:1883".to_string(),
            mqtt_client_id: "hamclock".to_string(),
            mqtt_topic_prefix: "hamclock/alerts".to_string(),
            mqtt_qos: 1,

            web_dashboard_enabled: false,
            web_dashboard_port: 8080,
            web_dashboard_host: "127.0.0.1".to_string(),
        }
    }
}

/// Alert configuration for Phase 8
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct AlertConfig {
    // DX Band Monitoring
    pub dx_alerts_enabled: bool,
    pub watched_bands: Vec<f32>,        // e.g., [14.074, 7.074, 3.573]
    pub watched_modes: Vec<String>,     // e.g., ["FT8", "CW", "SSB"]
    pub dx_min_frequency: Option<f32>,
    pub dx_max_frequency: Option<f32>,

    // Satellite Pass Notifications
    pub satellite_alerts_enabled: bool,
    pub satellite_elevation_threshold: f32,  // degrees
    pub watched_satellites: Vec<String>,
    pub satellite_countdown_enabled: bool,

    // Space Weather Alerts
    pub space_weather_alerts_enabled: bool,
    pub kp_alert_threshold: f32,
    pub kp_spike_threshold: f32,
    pub xray_alert_classes: Vec<String>,
    pub aurora_alert_level: String,
    pub cme_alerts_enabled: bool,

    // Visual settings
    pub alert_duration_seconds: u64,
    pub alert_flash_enabled: bool,
    pub audio_alerts_enabled: bool,
}

impl Default for AlertConfig {
    fn default() -> Self {
        Self {
            dx_alerts_enabled: true,
            watched_bands: vec![14.074, 7.074, 3.573], // FT8 frequencies
            watched_modes: vec!["FT8".to_string(), "CW".to_string()],
            dx_min_frequency: None,
            dx_max_frequency: None,

            satellite_alerts_enabled: true,
            satellite_elevation_threshold: 30.0,
            watched_satellites: vec!["ISS".to_string(), "SO-50".to_string()],
            satellite_countdown_enabled: true,

            space_weather_alerts_enabled: true,
            kp_alert_threshold: 5.0,
            kp_spike_threshold: 2.0,
            xray_alert_classes: vec!["M".to_string(), "X".to_string()],
            aurora_alert_level: "Moderate".to_string(),
            cme_alerts_enabled: true,

            alert_duration_seconds: 30,
            alert_flash_enabled: true,
            audio_alerts_enabled: false,
        }
    }
}

/// HamClock application configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    /// Display resolution (width x height)
    pub resolution: (u32, u32),

    /// Target frames per second
    pub target_fps: u32,

    /// Data update interval in seconds
    pub data_update_interval: u64,

    /// Default location (latitude, longitude)
    pub location: (f32, f32),

    /// Enable GPIO support
    pub gpio_enabled: bool,

    /// GPU backend (vulkan, gl, metal, dx12)
    pub gpu_backend: String,

    /// Enable logging
    pub logging: bool,

    /// Phase 7: N2YO API key for satellite tracking
    pub n2yo_api_key: Option<String>,

    /// Phase 7: DX Cluster host
    pub dx_cluster_host: String,

    /// Phase 7: DX Cluster port
    pub dx_cluster_port: u16,

    /// Phase 8: Alert configuration
    pub alert_config: AlertConfig,

    /// Phase 9: Alert extensions configuration
    pub phase9: Phase9Config,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            resolution: (1920, 1200),
            target_fps: 60,
            data_update_interval: 5,
            location: (0.0, 0.0),
            gpio_enabled: false,
            gpu_backend: "vulkan".to_string(),
            logging: false,
            n2yo_api_key: None,
            dx_cluster_host: "dxc.ve7cc.net".to_string(),
            dx_cluster_port: 23,
            alert_config: AlertConfig::default(),
            phase9: Phase9Config::default(),
        }
    }
}

impl Config {
    /// Load configuration from file or use defaults
    pub fn load() -> AppResult<Self> {
        // For now, return defaults
        // In future: load from config file
        Ok(Self::default())
    }

    /// Save configuration to file
    pub fn save(&self) -> AppResult<()> {
        // TODO: Implement file persistence
        Ok(())
    }
}
