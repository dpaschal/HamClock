//! Desktop notification system using notify-rust
//!
//! This module sends native desktop notifications for alerts.
//! Supports Linux (XDG), macOS (NSNotificationCenter), and Windows (toast).
//! Runs as an independent background task consuming from an mpsc channel.

use crate::data::models::{Alert, AlertSeverity, AlertType};
use crate::config::Phase9Config;
use notify_rust::{Notification, Timeout, Urgency};
use tokio::sync::mpsc;
use log::{info, error};

/// NotificationSender manages desktop notifications via notify-rust
pub struct NotificationSender {
    config: Phase9Config,
}

impl NotificationSender {
    /// Create new notification sender
    pub fn new(config: Phase9Config) -> Self {
        Self { config }
    }

    /// Send desktop notification for an alert
    /// Filters by minimum severity if configured
    pub fn send_alert(&self, alert: &Alert) -> Result<(), notify_rust::error::Error> {
        // Filter by minimum severity
        if alert.severity < self.config.notification_min_severity {
            return Ok(());
        }

        // Map severity to urgency level
        let urgency = match alert.severity {
            AlertSeverity::Emergency | AlertSeverity::Critical => Urgency::Critical,
            AlertSeverity::Warning => Urgency::Normal,
            AlertSeverity::Notice => Urgency::Low,
            AlertSeverity::Info => Urgency::Low,
        };

        // Set timeout based on config
        let timeout = Timeout::Milliseconds(self.config.notification_timeout_seconds * 1000);

        // Create summary based on alert type with emoji
        let summary = match alert.alert_type {
            AlertType::DxSpot => "🎙️ DX Spot",
            AlertType::SatellitePass => "🛰️ Satellite Pass",
            AlertType::KpSpike => "⚡ Geomagnetic Storm",
            AlertType::XrayFlare => "☀️ Solar Flare",
            AlertType::Aurora => "🌌 Aurora Alert",
            AlertType::Cme => "🌊 CME Detected",
        };

        // Send the notification
        Notification::new()
            .summary(summary)
            .body(&alert.message)
            .timeout(timeout)
            .urgency(urgency)
            .show()?;

        Ok(())
    }
}

/// Run the desktop notification background task
/// Receives alerts from channel and sends OS notifications
pub async fn run(mut rx: mpsc::Receiver<Alert>, config: Phase9Config) {
    if !config.desktop_notifications_enabled {
        info!("Desktop notifications disabled");
        return;
    }

    let sender = NotificationSender::new(config);
    info!("Desktop notification sender started");

    while let Some(alert) = rx.recv().await {
        match sender.send_alert(&alert) {
            Ok(()) => {
                info!("Sent desktop notification for alert: {}", alert.id);
            }
            Err(e) => {
                error!("Failed to send desktop notification: {}", e);
            }
        }
    }
}
