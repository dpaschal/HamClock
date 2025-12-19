//! Phase 9: Alert Extensions - History, Notifications, MQTT, Web Dashboard
//!
//! This module implements four complementary systems for the Phase 8 alert infrastructure:
//! 1. Alert History & Logging (SQLite) - Persistent alert database
//! 2. Desktop Notifications (OS Integration) - Native system notifications
//! 3. MQTT Publishing (Home Automation) - Real-time alert publishing
//! 4. Web Dashboard (Remote Monitoring) - HTTP/WebSocket server
//!
//! All features operate as independent background tasks connected via mpsc channels.

pub mod history;
pub mod notifications;
pub mod mqtt;
pub mod web_dashboard;

use tokio::sync::mpsc;
use crate::data::models::Alert;

/// AlertChannels distributes alerts to all Phase 9 features via mpsc channels.
/// Each feature runs independently and non-blocking.
#[derive(Clone)]
pub struct AlertChannels {
    pub history_tx: mpsc::Sender<Alert>,
    pub notification_tx: mpsc::Sender<Alert>,
    pub mqtt_tx: mpsc::Sender<Alert>,
    pub web_tx: mpsc::Sender<Alert>,
}

impl AlertChannels {
    /// Create new alert channels with specified capacity for each channel
    pub fn new(
        history_tx: mpsc::Sender<Alert>,
        notification_tx: mpsc::Sender<Alert>,
        mqtt_tx: mpsc::Sender<Alert>,
        web_tx: mpsc::Sender<Alert>,
    ) -> Self {
        Self {
            history_tx,
            notification_tx,
            mqtt_tx,
            web_tx,
        }
    }

    /// Distribute alert to all Phase 9 features (non-blocking)
    pub fn distribute(&self, alert: Alert) {
        let _ = self.history_tx.try_send(alert.clone());
        let _ = self.notification_tx.try_send(alert.clone());
        let _ = self.mqtt_tx.try_send(alert.clone());
        let _ = self.web_tx.try_send(alert);
    }
}
