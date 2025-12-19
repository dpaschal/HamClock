//! Alert detection and generation logic (Phase 8)

use crate::data::models::{
    AppData, Alert, AlertType, AlertSeverity,
};
use crate::config::AlertConfig;
use chrono::Utc;

#[derive(Clone)]
pub struct AlertDetector {
    config: AlertConfig,
}

impl AlertDetector {
    pub fn new(config: AlertConfig) -> Self {
        Self { config }
    }

    /// Main detection method - called periodically from background task
    pub fn detect_alerts(&self, app_data: &mut AppData) {
        app_data.alert_state.cleanup_expired();

        if self.config.dx_alerts_enabled {
            self.check_dx_alerts(app_data);
        }

        if self.config.satellite_alerts_enabled {
            self.check_satellite_alerts(app_data);
        }

        if self.config.space_weather_alerts_enabled {
            self.check_space_weather_alerts(app_data);
        }
    }

    /// Check for new DX spots matching watched bands/modes
    fn check_dx_alerts(&self, app_data: &mut AppData) {
        for spot in &app_data.dx_spots {
            // Skip if already seen
            if app_data.alert_state.last_dx_spots.contains(&spot.callsign) {
                continue;
            }

            // Check if band is watched
            let is_watched_band = self.config.watched_bands.iter()
                .any(|&freq| (spot.frequency - freq).abs() < 0.01);

            // Check if mode is watched
            let is_watched_mode = self.config.watched_modes.iter()
                .any(|mode| spot.mode.to_uppercase().contains(&mode.to_uppercase()));

            // Check frequency range constraints
            let in_range = match (self.config.dx_min_frequency, self.config.dx_max_frequency) {
                (Some(min), Some(max)) => spot.frequency >= min && spot.frequency <= max,
                (Some(min), None) => spot.frequency >= min,
                (None, Some(max)) => spot.frequency <= max,
                (None, None) => true,
            };

            // Create alert if conditions match
            if (is_watched_band || is_watched_mode) && in_range {
                let message = format!(
                    "NEW DX: {:.3} MHz {} {} by {}",
                    spot.frequency, spot.mode, spot.callsign, spot.spotter
                );

                let alert = Alert::new(
                    AlertType::DxSpot,
                    AlertSeverity::Notice,
                    message,
                    self.config.alert_duration_seconds,
                );

                app_data.alert_state.add_alert(alert);
                app_data.alert_state.last_dx_spots.push(spot.callsign.clone());
            }
        }

        // Trim history to prevent unbounded growth
        if app_data.alert_state.last_dx_spots.len() > 100 {
            app_data.alert_state.last_dx_spots.drain(0..50);
        }
    }

    /// Check for satellite passes reaching elevation threshold
    fn check_satellite_alerts(&self, app_data: &mut AppData) {
        for sat in &app_data.satellites {
            // Check if satellite is watched
            let is_watched = self.config.watched_satellites.is_empty() ||
                self.config.watched_satellites.iter()
                    .any(|name| sat.name.to_uppercase().contains(&name.to_uppercase()));

            if !is_watched {
                continue;
            }

            // Get previous elevation
            let prev_elevation = app_data.alert_state.last_satellite_elevations
                .get(&sat.name).copied().unwrap_or(0.0);

            // Update elevation tracking
            app_data.alert_state.last_satellite_elevations
                .insert(sat.name.clone(), sat.elevation);

            // Alert on elevation threshold crossing
            if sat.elevation >= self.config.satellite_elevation_threshold &&
               prev_elevation < self.config.satellite_elevation_threshold {

                let message = if self.config.satellite_countdown_enabled {
                    let degrees_to_peak = 90.0 - sat.elevation;
                    let minutes_to_peak = (degrees_to_peak / 10.0).max(1.0);

                    format!(
                        "{} PASS: El {:.0}° Az {:.0}° ({:.0} min to peak)",
                        sat.name, sat.elevation, sat.azimuth, minutes_to_peak
                    )
                } else {
                    format!(
                        "{} PASS: El {:.0}° Az {:.0}° ({}km)",
                        sat.name, sat.elevation, sat.azimuth, sat.range as i32
                    )
                };

                let alert = Alert::new(
                    AlertType::SatellitePass,
                    AlertSeverity::Notice,
                    message,
                    self.config.alert_duration_seconds * 2,
                );

                app_data.alert_state.add_alert(alert);
            }
        }
    }

    /// Check for space weather events (Kp, flares, aurora)
    fn check_space_weather_alerts(&self, app_data: &mut AppData) {
        let sw = &app_data.space_weather;

        // Kp spike detection
        let kp_change = sw.kp - app_data.alert_state.last_kp;
        if kp_change >= self.config.kp_spike_threshold {
            let severity = match sw.kp {
                k if k >= 8.0 => AlertSeverity::Emergency,
                k if k >= 6.0 => AlertSeverity::Critical,
                k if k >= 5.0 => AlertSeverity::Warning,
                _ => AlertSeverity::Notice,
            };

            let status = match sw.kp {
                k if k >= 8.0 => "STORM",
                k if k >= 6.0 => "ACTIVE",
                k if k >= 5.0 => "UNSETTLED",
                _ => "QUIET",
            };

            let message = format!("⚠ Kp SPIKE: {:.1} (+{:.1}) - {}", sw.kp, kp_change, status);

            let alert = Alert::new(
                AlertType::KpSpike,
                severity,
                message,
                self.config.alert_duration_seconds,
            );

            app_data.alert_state.add_alert(alert);
        }
        app_data.alert_state.last_kp = sw.kp;

        // X-ray flare detection
        if sw.flux != 0 && sw.flux as i32 > 50 {
            // Simple X-ray detection based on flux threshold
            let xray_class = if sw.flux > 1000 {
                "X"
            } else if sw.flux > 100 {
                "M"
            } else if sw.flux > 10 {
                "C"
            } else {
                "B"
            };

            if sw.flux as i32 != app_data.alert_state.last_xray_class.parse::<i32>().unwrap_or(0) {
                if self.config.xray_alert_classes.iter().any(|c| c == xray_class) {
                    let severity = match xray_class {
                        "X" => AlertSeverity::Critical,
                        "M" => AlertSeverity::Warning,
                        _ => AlertSeverity::Notice,
                    };

                    let message = format!("☀ SOLAR FLARE: {} class", xray_class);

                    let alert = Alert::new(
                        AlertType::XrayFlare,
                        severity,
                        message,
                        self.config.alert_duration_seconds,
                    );

                    app_data.alert_state.add_alert(alert);
                }
            }
            app_data.alert_state.last_xray_class = xray_class.to_string();
        }

        // Aurora alerts based on Kp level
        if sw.kp >= self.config.kp_alert_threshold {
            let aurora_severity = match sw.kp {
                k if k >= 8.0 => AlertSeverity::Critical,
                k if k >= 6.0 => AlertSeverity::Warning,
                k if k >= 5.0 => AlertSeverity::Notice,
                _ => AlertSeverity::Info,
            };

            let message = format!("🌌 AURORA LIKELY: Kp {:.1}", sw.kp);

            let alert = Alert::new(
                AlertType::Aurora,
                aurora_severity,
                message,
                self.config.alert_duration_seconds,
            );

            app_data.alert_state.add_alert(alert);
        }
    }
}
