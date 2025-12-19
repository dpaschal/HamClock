//! Web dashboard for remote alert monitoring
//!
//! This module runs an HTTP/WebSocket server for real-time remote alert monitoring.
//! Provides an interactive dashboard accessible from any web browser.
//! Runs as an independent background task consuming from an mpsc channel.

use crate::data::models::Alert;
use crate::config::Phase9Config;
use axum::{
    Router,
    extract::{State, ws::{WebSocket, WebSocketUpgrade, Message}},
    response::{Html, IntoResponse},
    routing::get,
};
use tokio::sync::{mpsc, broadcast};
use std::sync::Arc;
use log::{info, error};

/// Application state containing broadcast channel for WebSocket clients
struct AppState {
    alert_broadcast: broadcast::Sender<Alert>,
}

/// Serve the dashboard HTML page
async fn serve_index() -> Html<String> {
    let html = r#"<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>HamClock Alert Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Courier New', monospace; background: #000; color: #0f0; padding: 20px; min-height: 100vh; }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 20px; font-size: 2em; text-shadow: 0 0 10px #0f0; }
        .status { text-align: center; margin-bottom: 20px; font-size: 0.9em; }
        .status.connected { color: #0f0; }
        .status.disconnected { color: #f00; }
        #alerts-container { display: grid; grid-template-columns: repeat(auto-fill, minmax(350px, 1fr)); gap: 10px; margin-top: 20px; }
        .alert { border: 2px solid #0f0; padding: 12px; border-radius: 4px; animation: slideIn 0.3s ease-out; }
        @keyframes slideIn { from { opacity: 0; transform: translateY(-20px); } to { opacity: 1; transform: translateY(0); } }
        .alert.info { border-color: #0099ff; color: #0099ff; }
        .alert.notice { border-color: #ffff00; color: #ffff00; }
        .alert.warning { border-color: #ffaa00; color: #ffaa00; }
        .alert.critical { border-color: #ff0000; color: #ff0000; }
        .alert.emergency { border-color: #ff00ff; color: #ff00ff; animation: pulse 1s infinite; }
        @keyframes pulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.7; } }
        .alert-type { font-weight: bold; font-size: 1.1em; margin-bottom: 5px; }
        .alert-message { font-size: 0.95em; margin-bottom: 5px; word-wrap: break-word; }
        .alert-time { font-size: 0.8em; opacity: 0.7; }
        .alert-count { text-align: center; margin-top: 20px; font-size: 0.9em; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>⚡ HamClock Alert Dashboard</h1>
        <div class="status disconnected" id="status">Connecting...</div>
        <div id="alerts-container"></div>
        <div class="alert-count" id="alert-count">No alerts yet</div>
    </div>

    <script>
        const alertsContainer = document.getElementById('alerts-container');
        const statusDiv = document.getElementById('status');
        const alertCountDiv = document.getElementById('alert-count');
        let alertCount = 0;

        const protocol = window.location.protocol === 'https:' ? 'wss' : 'ws';
        const wsUrl = `${protocol}://${window.location.host}/ws`;

        const ws = new WebSocket(wsUrl);

        ws.onopen = () => {
            statusDiv.className = 'status connected';
            statusDiv.textContent = '✓ Connected';
        };

        ws.onclose = () => {
            statusDiv.className = 'status disconnected';
            statusDiv.textContent = '✗ Disconnected - Attempting to reconnect...';
            setTimeout(() => { window.location.reload(); }, 3000);
        };

        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            statusDiv.className = 'status disconnected';
            statusDiv.textContent = '✗ Connection Error';
        };

        ws.onmessage = (event) => {
            try {
                const alert = JSON.parse(event.data);
                displayAlert(alert);
            } catch (e) {
                console.error('Failed to parse alert:', e);
            }
        };

        function displayAlert(alert) {
            alertCount++;
            const alertDiv = document.createElement('div');
            alertDiv.className = `alert ${alert.severity.toLowerCase()}`;
            const createdTime = new Date(alert.created_at);
            const timeStr = createdTime.toLocaleTimeString();
            alertDiv.innerHTML = `
                <div class="alert-type">${alert.type}</div>
                <div class="alert-message">${escapeHtml(alert.message)}</div>
                <div class="alert-time">${timeStr}</div>
            `;
            alertsContainer.insertBefore(alertDiv, alertsContainer.firstChild);
            while (alertsContainer.children.length > 100) {
                alertsContainer.removeChild(alertsContainer.lastChild);
            }
            updateAlertCount();
            if (alert.severity === 'Critical' || alert.severity === 'Emergency') {
                playAlert();
            }
        }

        function updateAlertCount() {
            const count = alertsContainer.children.length;
            alertCountDiv.textContent = `Showing ${count} alert${count !== 1 ? 's' : ''}`;
        }

        function escapeHtml(text) {
            const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
            return text.replace(/[&<>"']/g, m => map[m]);
        }

        function playAlert() {
            try {
                const audioContext = new (window.AudioContext || window.webkitAudioContext)();
                const oscillator = audioContext.createOscillator();
                const gainNode = audioContext.createGain();
                oscillator.connect(gainNode);
                gainNode.connect(audioContext.destination);
                oscillator.frequency.value = 800;
                oscillator.type = 'sine';
                gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
                gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);
                oscillator.start(audioContext.currentTime);
                oscillator.stop(audioContext.currentTime + 0.5);
            } catch (e) {
                console.log('Audio not available');
            }
        }
    </script>
</body>
</html>"#;
    Html(html.to_string())
}

/// Handle WebSocket upgrade requests
async fn websocket_handler(
    ws: WebSocketUpgrade,
    State(state): State<Arc<AppState>>,
) -> impl IntoResponse {
    ws.on_upgrade(|socket| handle_socket(socket, state))
}

/// Handle WebSocket connections
/// Subscribes to alert broadcast and sends updates to client
async fn handle_socket(mut socket: WebSocket, state: Arc<AppState>) {
    let mut rx = state.alert_broadcast.subscribe();

    while let Ok(alert) = rx.recv().await {
        // Create JSON message
        let json = serde_json::json!({
            "type": format!("{:?}", alert.alert_type),
            "severity": format!("{:?}", alert.severity),
            "message": alert.message,
            "created_at": alert.created_at.to_rfc3339(),
        });

        // Send to WebSocket client
        if socket.send(Message::Text(json.to_string())).await.is_err() {
            break; // Client disconnected
        }
    }
}

/// Run the web dashboard background task
/// Starts HTTP server and manages WebSocket connections
pub async fn run(mut rx: mpsc::Receiver<Alert>, config: Phase9Config) {
    if !config.web_dashboard_enabled {
        info!("Web dashboard disabled");
        return;
    }

    // Create broadcast channel for alerts
    let (broadcast_tx, _) = broadcast::channel(100);
    let state = Arc::new(AppState {
        alert_broadcast: broadcast_tx.clone(),
    });

    // Spawn task to distribute alerts from mpsc to broadcast channel
    tokio::spawn(async move {
        while let Some(alert) = rx.recv().await {
            let _ = broadcast_tx.send(alert);
        }
    });

    // Build axum router
    let app = Router::new()
        .route("/", get(serve_index))
        .route("/ws", get(websocket_handler))
        .with_state(state);

    // Create TCP listener
    let addr = format!("{}:{}", config.web_dashboard_host, config.web_dashboard_port);
    let listener = match tokio::net::TcpListener::bind(&addr).await {
        Ok(l) => l,
        Err(e) => {
            error!("Failed to bind web dashboard on {}: {}", addr, e);
            return;
        }
    };

    info!("Web dashboard started on http://{}", addr);

    // Run the server
    if let Err(e) = axum::serve(listener, app).await {
        error!("Web dashboard server error: {}", e);
    }
}
