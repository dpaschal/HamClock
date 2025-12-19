//! MQTT alert publishing for home automation integration
//!
//! This module publishes alerts to an MQTT broker for integration with home automation systems.
//! Supports configurable QoS levels and topic structure.
//! Runs as an independent background task consuming from an mpsc channel.

use crate::data::models::Alert;
use crate::config::Phase9Config;
use rumqttc::{MqttOptions, AsyncClient, QoS, Event, Packet};
use tokio::sync::mpsc;
use serde_json;
use log::{info, error};
use std::time::Duration;

/// MqttPublisher manages MQTT client for alert publishing
pub struct MqttPublisher {
    client: AsyncClient,
    config: Phase9Config,
}

impl MqttPublisher {
    /// Create new MQTT publisher with connection to broker
    pub async fn new(config: Phase9Config) -> Result<(Self, rumqttc::EventLoop), Box<dyn std::error::Error>> {
        // Parse MQTT URL: mqtt://host:port or mqtt://localhost:1883
        let url = &config.mqtt_broker_url;
        let (host, port) = if let Some(after_scheme) = url.strip_prefix("mqtt://") {
            if let Some((h, p)) = after_scheme.rsplit_once(':') {
                let port = p.parse::<u16>()
                    .map_err(|_| "Invalid port number")?;
                (h.to_string(), port)
            } else {
                (after_scheme.to_string(), 1883)  // Default MQTT port
            }
        } else if let Some(after_scheme) = url.strip_prefix("mqtts://") {
            if let Some((h, p)) = after_scheme.rsplit_once(':') {
                let port = p.parse::<u16>()
                    .map_err(|_| "Invalid port number")?;
                (h.to_string(), port)
            } else {
                (after_scheme.to_string(), 8883)  // Default MQTTS port
            }
        } else {
            return Err("MQTT broker URL must start with mqtt:// or mqtts://".into());
        };

        let mut mqtt_options = MqttOptions::new(config.mqtt_client_id.clone(), host, port);

        // Configure client
        mqtt_options.set_keep_alive(Duration::from_secs(30));

        // Create MQTT client
        let (client, eventloop) = AsyncClient::new(mqtt_options, 10);

        Ok((Self { client, config }, eventloop))
    }

    /// Publish an alert to MQTT broker
    pub async fn publish_alert(&self, alert: &Alert) -> Result<(), rumqttc::ClientError> {
        // Build topic with alert type
        let topic = format!("{}/{:?}", self.config.mqtt_topic_prefix, alert.alert_type);

        // Create JSON payload
        let payload = serde_json::json!({
            "id": alert.id,
            "type": format!("{:?}", alert.alert_type),
            "severity": format!("{:?}", alert.severity),
            "message": alert.message,
            "created_at": alert.created_at.to_rfc3339(),
            "expires_at": alert.expires_at.to_rfc3339(),
        });

        // Map QoS configuration
        let qos = match self.config.mqtt_qos {
            0 => QoS::AtMostOnce,
            1 => QoS::AtLeastOnce,
            2 => QoS::ExactlyOnce,
            _ => QoS::AtLeastOnce,
        };

        // Publish message
        self.client.publish(
            topic,
            qos,
            false, // retain=false
            payload.to_string().into_bytes(),
        ).await?;

        Ok(())
    }
}

/// Run the MQTT publisher background task
/// Maintains connection to broker and publishes alerts
pub async fn run(mut rx: mpsc::Receiver<Alert>, config: Phase9Config) {
    if !config.mqtt_enabled {
        info!("MQTT publishing disabled");
        return;
    }

    // Initialize MQTT publisher
    let (publisher, mut eventloop) = match MqttPublisher::new(config).await {
        Ok(p) => p,
        Err(e) => {
            error!("Failed to initialize MQTT client: {:?}", e);
            return;
        }
    };

    info!("MQTT publisher started");

    // Spawn eventloop handler task
    tokio::spawn(async move {
        loop {
            match eventloop.poll().await {
                Ok(Event::Incoming(Packet::ConnAck(_))) => {
                    info!("MQTT connected to broker");
                }
                Ok(Event::Outgoing(_)) => {
                    // Expected for QoS 1/2
                    log::debug!("MQTT outgoing packet");
                }
                Err(e) => {
                    error!("MQTT connection error: {}", e);
                    tokio::time::sleep(Duration::from_secs(5)).await;
                }
                _ => {}
            }
        }
    });

    // Alert publishing loop
    while let Some(alert) = rx.recv().await {
        match publisher.publish_alert(&alert).await {
            Ok(()) => {
                info!("Published alert to MQTT: {}", alert.id);
            }
            Err(e) => {
                error!("Failed to publish alert to MQTT: {}", e);
            }
        }
    }
}
