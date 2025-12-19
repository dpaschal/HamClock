# Feature Overview - All 12 Implemented Features

HamClock implements 12 distinct features across Phase 8 (Core Alert System) and Phase 9 (Alert Extensions). This document provides an overview of each feature, its use case, and configuration.

---

## Phase 8: Core Alert System (8 Features)

Phase 8 implements the foundational alert infrastructure that detects radio-relevant events and notifies users with configurable thresholds and severity levels.

### 1. DX Band Monitoring

**Purpose**: Alert on new DX spots on your watched frequencies and modes

**How It Works**:
- Connects to DX Cluster network (e.g., ve7cc.net:23)
- Receives real-time DX spot stream
- Filters by watched band frequencies (e.g., 14.074 MHz FT8)
- Matches by mode (FT8, CW, SSB, etc.)
- Applies frequency range constraints (optional min/max)

**Use Case**: You're interested in 20m FT8 activity. DX Monitor alerts you when:
- New spot on 14.074 MHz
- Matching watched modes
- Within time window

**Alert Example**:
```
🎙️ NEW DX: 14.074 MHz FT8 K4ABC by W5XYZ
```

**Configuration**:
```toml
[alert_config]
dx_alerts_enabled = true
watched_bands = [14.074, 7.074, 3.573]    # 20m, 40m, 80m
watched_modes = ["FT8", "CW"]
dx_min_frequency = 14.070   # Optional range constraint
dx_max_frequency = 14.078
alert_duration_seconds = 30
```

**Severity**: `Notice` (yellow)

---

### 2. Satellite Pass Notifications

**Purpose**: Alert when your watched satellites reach visible elevation threshold

**How It Works**:
- Uses SGP4 orbital propagation (N2YO API or local calculations)
- Tracks altitude and azimuth for each satellite
- Detects elevation threshold crossings (e.g., 30° = "rise above horizon")
- Calculates time to peak elevation
- Generates countdown message

**Use Case**: ISS is rising above 30° in the west for a 12-minute pass. You get:
```
🛰️ ISS PASS: El 31° Az 260° (11 min to peak)
```

**Alert Example**:
- **With countdown**: "ISS PASS: El 31° Az 260° (11 min to peak)"
- **Without countdown**: "ISS PASS: El 31° Az 260° (1200km)"

**Configuration**:
```toml
[alert_config]
satellite_alerts_enabled = true
watched_satellites = ["ISS", "SO-50", "AO-91"]
satellite_elevation_threshold = 30.0    # degrees above horizon
satellite_countdown_enabled = true
alert_duration_seconds = 60  # Longer for passes
```

**Severity**: `Notice` (yellow)

**Data Source**: N2YO API (requires API key) or local SGP4 propagation

---

### 3. Kp Spike Alerts

**Purpose**: Alert on rapid increases in geomagnetic activity (Kp index)

**How It Works**:
- Monitors Kp index from NOAA SWPC
- Detects spike magnitude (threshold: 2.0 by default)
- Maps Kp value to severity:
  - Kp ≥ 8.0 → `Emergency` (magenta pulse)
  - Kp ≥ 6.0 → `Critical` (red)
  - Kp ≥ 5.0 → `Warning` (orange)
  - Kp < 5.0 → `Notice` (yellow)
- Displays current Kp value, delta, and status

**Use Case**: Kp jumps from 3.0 to 6.0 (spike of 3.0). You get:
```
⚠️ Kp SPIKE: 6.0 (+3.0) - ACTIVE
```

**Alert Examples**:
- Quiet: "Kp SPIKE: 4.0 (+1.0) - QUIET"
- Unsettled: "Kp SPIKE: 5.5 (+2.0) - UNSETTLED"
- Active: "Kp SPIKE: 6.5 (+3.0) - ACTIVE"
- Storm: "Kp SPIKE: 8.5 (+4.0) - STORM"

**Configuration**:
```toml
[alert_config]
space_weather_alerts_enabled = true
kp_spike_threshold = 2.0    # Minimum spike magnitude
kp_alert_threshold = 5.0    # Minimum Kp for aurora alert
```

**Severity**: Based on Kp value (Notice→Warning→Critical→Emergency)

---

### 4. X-Ray Flare Alerts

**Purpose**: Alert on solar X-ray flare events affecting HF propagation

**How It Works**:
- Monitors NOAA X-ray flux in SXY classification
- Detects class changes: A, B, C, M, X
- Filters by configured alert classes
- Maps class to severity:
  - X-class → `Critical` (red)
  - M-class → `Warning` (orange)
  - C-class and below → `Notice` (yellow)

**Use Case**: X-ray flux rises to M5 class (solar flare). You get:
```
☀️ SOLAR FLARE: M5 class
```

**Alert Examples**:
- A/B-class (background): No alert if not configured
- C-class: "SOLAR FLARE: C5 class"
- M-class: "SOLAR FLARE: M2 class"
- X-class: "SOLAR FLARE: X1 class"

**Configuration**:
```toml
[alert_config]
xray_alert_classes = ["M", "X"]  # Only alert on M and X class
```

**Severity**: M→Warning (orange), X→Critical (red)

**Data Source**: NOAA SWPC (real-time X-ray flux)

---

### 5. Aurora Visibility Alerts

**Purpose**: Alert when aurora is likely visible at your location

**How It Works**:
- Combines Kp index with aurora forecast model
- Calculates aurora visibility based on latitude and Kp
- Detects transitions to "visible" state
- Shows Kp value at time of alert

**Use Case**: You're at 45°N latitude, Kp reaches 6.0. Aurora is likely visible:
```
🌌 AURORA LIKELY: Kp 6.0
```

**Alert Examples**:
- Low activity: No aurora alert
- Moderate (Kp 5-6): "AURORA LIKELY: Kp 5.5"
- High (Kp 6-7): "AURORA VISIBLE: Kp 6.5"
- Extreme (Kp 8+): "STRONG AURORA: Kp 8.2"

**Configuration**:
```toml
[alert_config]
aurora_alert_level = "Moderate"  # Low, Moderate, High, Extreme
kp_alert_threshold = 5.0
```

**Severity**: Based on Kp (Notice→Warning→Critical)

**Data Source**: NOAA Kp index + Aurora forecast model

---

### 6. CME (Coronal Mass Ejection) Detection

**Purpose**: Alert on coronal mass ejections that affect propagation

**How It Works**:
- Monitors solar wind speed and density
- Detects sudden increases in solar activity
- Identifies characteristics of CME shockfronts
- Calculates severity based on rate of change
- Shows flux and AP index changes

**Use Case**: CME shockfront hits Earth, causing solar wind spike:
```
🌊 CME ALERT: Flux +250 SFU, AP +150 (possible coronal mass ejection)
```

**Alert Examples**:
- Minor: "CME ALERT: Flux +200 SFU, AP +80"
- Major: "CME ALERT: Flux +350 SFU, AP +120"
- Extreme: "CME ALERT: Flux +500 SFU, AP +250"

**Configuration**:
```toml
[alert_config]
cme_alerts_enabled = true
# Thresholds are hardcoded:
# - Flux change > 200 SFU or AP change > 100 → triggers alert
# - Flux > 350 or AP > 150 → Warning
# - Flux > 500 or AP > 200 → Critical
```

**Severity**: Notice→Warning→Critical (based on magnitude)

**Data Source**: NOAA Solar Wind Data

---

### 7. Audio Alerts

**Purpose**: Play audible notification on important alerts

**How It Works**:
- Generates sine wave tone (800 Hz, 500ms duration)
- Plays only for Critical and Emergency severity
- Uses system audio output
- Non-blocking (doesn't delay alert processing)

**Configuration**:
```toml
[alert_config]
audio_alerts_enabled = true  # Enable/disable audio
alert_duration_seconds = 30   # Visual alert duration
```

**Trigger Severity**: Critical, Emergency

**Technical**: Web Audio API (browser) or ALSA/PulseAudio (desktop)

---

### 8. Alert Acknowledgment

**Purpose**: User control to dismiss or acknowledge alerts

**How It Works**:
- Keyboard shortcuts to acknowledge alerts
- `SPACE` - Acknowledge most recent alert
- `ESC` - Acknowledge all alerts
- Acknowledged alerts remain visible but marked as read
- Time-based expiry (default: 30 seconds)

**UI Indication**:
- Active alerts: Color-coded box in top-right corner
- Expired alerts: Fade out and disappear
- Acknowledged: Visual indicator (strikethrough or reduced opacity)

**Configuration**:
```toml
[alert_config]
alert_duration_seconds = 30  # Auto-expire after 30s
alert_flash_enabled = true   # Background flash on critical
```

**Interaction**:
```
1. Alert appears: "NEW DX: 14.074 FT8 K4ABC"
2. User presses SPACE
3. Alert marked as acknowledged
4. After 30s: Alert auto-expires and disappears
```

---

## Phase 9: Alert Extensions (4 Features)

Phase 9 adds production-grade features for alert storage, local/remote notifications, and home automation integration.

### 9. SQLite Alert History

**Purpose**: Persistent storage of all alerts for audit trail and analytics

**How It Works**:
- Stores alerts to SQLite database in ~/.hamclock/alerts.db
- Captures: alert type, severity, message, timestamps, acknowledge status
- Automatic cleanup: deletes alerts older than retention period (default: 30 days)
- Enforces max entries (default: 10,000 alerts)
- Indexed on timestamp for fast queries

**Use Cases**:
- "When was the last M-class flare?" → Query alerts table
- "How many ISS passes this month?" → Count SatellitePass alerts
- Audit trail: "What alerts were missed while offline?"
- Analytics: "Peak activity times for DX?"

**Database Schema**:
```sql
CREATE TABLE alerts (
    id TEXT PRIMARY KEY,           -- "DxSpot-1734614400"
    alert_type TEXT NOT NULL,      -- "DxSpot", "SatellitePass", etc.
    severity TEXT NOT NULL,        -- "Notice", "Warning", etc.
    message TEXT NOT NULL,         -- Full alert message
    created_at INTEGER NOT NULL,   -- Unix timestamp
    expires_at INTEGER NOT NULL,   -- Expiration timestamp
    acknowledged INTEGER NOT NULL  -- 0 or 1
);
```

**Configuration**:
```toml
[phase9]
history_enabled = true
history_db_path = "~/.hamclock/alerts.db"
history_retention_days = 30      # Auto-delete after 30 days
history_max_entries = 10000      # Keep only newest 10000
```

**Features**:
- ✅ Automatic hourly cleanup
- ✅ Retention policy enforcement
- ✅ Max entry enforcement
- ✅ Query API for future use
- ✅ Zero performance impact on alerts

---

### 10. Desktop Notifications

**Purpose**: Native OS notifications that appear in system notification center

**How It Works**:
- Uses notify-rust (cross-platform library)
- Linux: XDG D-Bus notifications (system tray)
- macOS: NSNotificationCenter (Notification Center)
- Windows: Windows Toast notifications
- Maps alert severity to urgency level
- Configurable timeout per notification
- Filters by minimum severity (e.g., only Warning+)

**Use Cases**:
- Alert visible even when HamClock window is minimized
- System notification history (search past notifications)
- Integration with OS notification settings
- Do Not Disturb respects system settings

**Notification Examples**:

Linux (XDG):
```
+─────────────────────────────+
│ 🎙️ DX Spot                   │
│ NEW DX: 14.074 FT8 K4ABC    │
│ ← 9 seconds ago             │
+─────────────────────────────+
```

macOS:
```
Notification Center displays:
🛰️ Satellite Pass
ISS PASS: El 35° Az 270°
```

**Configuration**:
```toml
[phase9]
desktop_notifications_enabled = true
notification_timeout_seconds = 10          # Display time
notification_min_severity = "Warning"      # Only Warning and above
```

**Alert Type Emoji**:
- DxSpot → 🎙️
- SatellitePass → 🛰️
- KpSpike → ⚡
- XrayFlare → ☀️
- Aurora → 🌌
- CME → 🌊

**Severity Mapping**:
- Emergency/Critical → Urgent
- Warning → Normal
- Notice/Info → Low

---

### 11. MQTT Publishing

**Purpose**: Publish alerts to MQTT broker for home automation and monitoring systems

**How It Works**:
- Connects to MQTT broker (e.g., Mosquitto, HiveMQ)
- Publishes JSON messages for each alert
- Configurable topic structure: `{prefix}/{alert_type}`
- Supports QoS levels: 0 (Fire & Forget), 1 (At Least Once), 2 (Exactly Once)
- Auto-reconnection with exponential backoff
- Non-blocking: alert loss doesn't affect HamClock

**Use Cases**:
- Trigger Home Assistant automation: "If CME alert, activate backup power"
- Log alerts to InfluxDB for visualization
- Integration with Telegram/Discord via MQTT bridge
- Control smart home based on aurora forecast

**MQTT Topic Structure**:
```
hamclock/alerts/DxSpot          → DX spots
hamclock/alerts/SatellitePass   → Satellite passes
hamclock/alerts/KpSpike         → Kp events
hamclock/alerts/XrayFlare       → Solar flares
hamclock/alerts/Aurora          → Aurora alerts
hamclock/alerts/Cme             → CME events
```

**Message Payload** (JSON):
```json
{
  "id": "DxSpot-1734614400",
  "type": "DxSpot",
  "severity": "Notice",
  "message": "NEW DX: 14.074 MHz FT8 K4ABC by W5XYZ",
  "created_at": "2025-12-19T08:30:00Z",
  "expires_at": "2025-12-19T08:30:30Z"
}
```

**Configuration**:
```toml
[phase9]
mqtt_enabled = true
mqtt_broker_url = "mqtt://localhost:1883"      # mqtt:// or mqtts://
mqtt_client_id = "hamclock"
mqtt_topic_prefix = "hamclock/alerts"
mqtt_qos = 1        # 0=Fire&Forget, 1=AtLeastOnce, 2=Exactly
```

**Home Assistant Integration Example**:
```yaml
automation:
  - trigger:
      platform: mqtt
      topic: "hamclock/alerts/Cme"
    action:
      service: switch.turn_on
      entity_id: switch.generator_backup
```

---

### 12. Web Dashboard

**Purpose**: Remote real-time monitoring of alerts from any web browser

**How It Works**:
- HTTP server on configurable port (default: 8080)
- WebSocket for real-time alert streaming
- Embedded HTML/CSS/JavaScript dashboard
- Auto-reconnection if connection drops
- Terminal-inspired green-on-black theme
- Color-coded alerts by severity
- Responsive grid layout
- Auto-scrolling feed (newest first, max 100 alerts)
- Audio beep on critical alerts (Web Audio API)

**Access**: `http://localhost:8080` or `http://<your-ip>:8080`

**Dashboard Features**:
- Live connection status indicator
- Real-time alert count
- Search/filter (future enhancement)
- Severity color-coding:
  - Info: Blue (#0099ff)
  - Notice: Yellow (#ffff00)
  - Warning: Orange (#ffaa00)
  - Critical: Red (#ff0000)
  - Emergency: Magenta (#ff00ff) with pulse animation

**Alert Display**:
```
+─────────────────────────────────+
│ ⚡ HamClock Alert Dashboard    │
│ ✓ Connected                     │
├─────────────────────────────────+
│ ┌───────────────────────────────┐│
││ DxSpot                          ││
││ NEW DX: 14.074 FT8 K4ABC       ││
││ 14:32:15                        ││
│└───────────────────────────────┘│
│ ┌───────────────────────────────┐│
││ SatellitePass                   ││
││ ISS PASS: El 35° Az 270°       ││
││ 14:31:45                        ││
│└───────────────────────────────┘│
│ Showing 2 alerts                │
└─────────────────────────────────┘
```

**Configuration**:
```toml
[phase9]
web_dashboard_enabled = true
web_dashboard_host = "127.0.0.1"    # "0.0.0.0" for remote access
web_dashboard_port = 8080
```

**Security Note**:
- Default: `127.0.0.1:8080` (local only)
- Remote access: Change to `0.0.0.0:8080` (accessible from network)
- Recommended: Use VPN or reverse proxy for remote access
- Future: Add authentication layer

---

## Feature Interaction Matrix

| Feature | Requires | Enabled By | Used By |
|---------|----------|-----------|---------|
| DX Monitoring | DX Cluster | `dx_alerts_enabled` | Audit, Notifications, MQTT |
| Satellite Passes | N2YO/SGP4 | `satellite_alerts_enabled` | Audit, Notifications, MQTT |
| Kp Spikes | NOAA Kp | `space_weather_alerts_enabled` | Audit, Aurora, MQTT |
| X-Ray Flares | NOAA X-ray | `space_weather_alerts_enabled` | Audit, Notifications, MQTT |
| Aurora | NOAA Kp | `space_weather_alerts_enabled` | Audit, Notifications, MQTT |
| CME Detection | NOAA Wind | `cme_alerts_enabled` | Audit, Notifications, MQTT |
| Audio Alerts | Alert (any) | `audio_alerts_enabled` | User feedback |
| Acknowledgment | Alert (any) | User keyboard | UI state |
| SQLite History | Alert (any) | `history_enabled` | Audit trail |
| Desktop Notif | Alert (any) | `desktop_notifications_enabled` | User awareness |
| MQTT | Alert (any) | `mqtt_enabled` | Home automation |
| Web Dashboard | Alert (any) | `web_dashboard_enabled` | Remote monitoring |

---

## Configuration Quick Reference

```toml
# Minimal config (Phase 8 only)
[alert_config]
dx_alerts_enabled = true
watched_bands = [14.074, 7.074]
satellite_alerts_enabled = true
watched_satellites = ["ISS"]
space_weather_alerts_enabled = true

# Full config (Phase 8 + 9)
[alert_config]
dx_alerts_enabled = true
watched_bands = [14.074, 7.074, 3.573]
watched_modes = ["FT8", "CW"]
satellite_alerts_enabled = true
watched_satellites = ["ISS", "SO-50", "AO-91"]
satellite_elevation_threshold = 30.0
satellite_countdown_enabled = true
space_weather_alerts_enabled = true
kp_alert_threshold = 5.0
kp_spike_threshold = 2.0
xray_alert_classes = ["M", "X"]
aurora_alert_level = "Moderate"
cme_alerts_enabled = true
alert_duration_seconds = 30
alert_flash_enabled = true
audio_alerts_enabled = true

[phase9]
history_enabled = true
history_db_path = "~/.hamclock/alerts.db"
history_retention_days = 30
history_max_entries = 10000

desktop_notifications_enabled = true
notification_timeout_seconds = 10
notification_min_severity = "Warning"

mqtt_enabled = true
mqtt_broker_url = "mqtt://localhost:1883"
mqtt_client_id = "hamclock"
mqtt_topic_prefix = "hamclock/alerts"
mqtt_qos = 1

web_dashboard_enabled = true
web_dashboard_host = "127.0.0.1"
web_dashboard_port = 8080
```

---

## Summary

**12 features organized in 2 phases:**

**Phase 8 (Core)**: 6 alert types + 2 notifications = foundational alerting
**Phase 9 (Extensions)**: History + Desktop + MQTT + Dashboard = production-ready

All features are independent and can be enabled/disabled separately. They work together seamlessly through the alert distribution system.

---

**Last Updated**: 2025-12-19 | **Version**: 0.1.0-phase9
