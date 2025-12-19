//! Alert history logging to SQLite database
//!
//! This module manages persistent storage of alerts with automatic cleanup and retention policies.
//! Runs as an independent background task consuming from an mpsc channel.

use crate::data::models::Alert;
use crate::config::Phase9Config;
use rusqlite::{Connection, params, Result as SqliteResult};
use tokio::sync::mpsc;
use chrono::{Utc, Duration};
use log::{info, error};

/// AlertHistory manages SQLite database for persistent alert storage
pub struct AlertHistory {
    conn: Connection,
    config: Phase9Config,
}

impl AlertHistory {
    /// Initialize AlertHistory with database connection and schema
    pub fn new(config: Phase9Config) -> SqliteResult<Self> {
        // Expand ~ in path
        let db_path = shellexpand::tilde(&config.history_db_path.to_string_lossy()).to_string();

        // Ensure parent directory exists
        if let Some(parent) = std::path::Path::new(&db_path).parent() {
            std::fs::create_dir_all(parent).ok();
        }

        let conn = Connection::open(db_path)?;

        // Create schema
        conn.execute(
            "CREATE TABLE IF NOT EXISTS alerts (
                id TEXT PRIMARY KEY,
                alert_type TEXT NOT NULL,
                severity TEXT NOT NULL,
                message TEXT NOT NULL,
                created_at INTEGER NOT NULL,
                expires_at INTEGER NOT NULL,
                acknowledged INTEGER NOT NULL
            )",
            [],
        )?;

        // Create index for query performance
        conn.execute(
            "CREATE INDEX IF NOT EXISTS idx_created_at ON alerts(created_at DESC)",
            [],
        )?;

        Ok(Self { conn, config })
    }

    /// Insert a new alert into the database
    pub fn insert_alert(&self, alert: &Alert) -> SqliteResult<()> {
        self.conn.execute(
            "INSERT INTO alerts (id, alert_type, severity, message, created_at, expires_at, acknowledged)
             VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)",
            params![
                alert.id,
                format!("{:?}", alert.alert_type),
                format!("{:?}", alert.severity),
                alert.message,
                alert.created_at.timestamp(),
                alert.expires_at.timestamp(),
                alert.acknowledged as i32,
            ],
        )?;
        Ok(())
    }

    /// Remove alerts older than retention period
    pub fn cleanup_old_entries(&self) -> SqliteResult<usize> {
        let cutoff = Utc::now() - Duration::days(self.config.history_retention_days as i64);

        self.conn.execute(
            "DELETE FROM alerts WHERE created_at < ?1",
            params![cutoff.timestamp()],
        )
    }

    /// Enforce maximum entries limit by removing oldest alerts
    pub fn enforce_max_entries(&self) -> SqliteResult<()> {
        let count: usize = self.conn.query_row(
            "SELECT COUNT(*) FROM alerts",
            [],
            |row| row.get(0),
        )?;

        if count > self.config.history_max_entries {
            let to_delete = count - self.config.history_max_entries;
            self.conn.execute(
                "DELETE FROM alerts WHERE id IN (
                    SELECT id FROM alerts ORDER BY created_at ASC LIMIT ?1
                )",
                params![to_delete],
            )?;
        }

        Ok(())
    }

    /// Get total count of alerts in database
    pub fn get_count(&self) -> SqliteResult<usize> {
        self.conn.query_row(
            "SELECT COUNT(*) FROM alerts",
            [],
            |row| row.get(0),
        )
    }
}

/// Run the alert history logger background task
/// Receives alerts from channel and logs to SQLite
/// Performs periodic cleanup every hour
pub async fn run(mut rx: mpsc::Receiver<Alert>, config: Phase9Config) {
    if !config.history_enabled {
        info!("Alert history logging disabled");
        return;
    }

    let history = match AlertHistory::new(config.clone()) {
        Ok(h) => h,
        Err(e) => {
            error!("Failed to initialize alert history: {}", e);
            return;
        }
    };

    info!("Alert history logger started");

    // Cleanup task runs every hour
    let cleanup_interval = tokio::time::interval(tokio::time::Duration::from_secs(3600));
    tokio::pin!(cleanup_interval);

    loop {
        tokio::select! {
            Some(alert) = rx.recv() => {
                if let Err(e) = history.insert_alert(&alert) {
                    error!("Failed to log alert: {}", e);
                } else {
                    log::debug!("Logged alert to history: {}", alert.id);
                }
            }
            _ = cleanup_interval.tick() => {
                // Cleanup old entries based on retention policy
                if let Err(e) = history.cleanup_old_entries() {
                    error!("Failed to cleanup old alerts: {}", e);
                }

                // Enforce maximum entries limit
                if let Err(e) = history.enforce_max_entries() {
                    error!("Failed to enforce max entries: {}", e);
                }

                // Log statistics
                if let Ok(count) = history.get_count() {
                    info!("Alert history database: {} entries", count);
                }
            }
        }
    }
}
