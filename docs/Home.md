# HamClock - Amateur Radio Clock with AI-Powered Alerts

**HamClock** is a high-performance GPU-accelerated radio clock application for amateur radio operators, featuring real-time satellite tracking, DX spotting, space weather monitoring, and alert distribution with a modern Rust architecture.

## Quick Links

- **[Why Rust Over C?](Why-Rust-Over-C)** - Architecture and design decisions
- **[Feature Overview](Feature-Overview)** - All 12 implemented features
- **[Phase 8: Alert System](Phase-8-Alert-System)** - Core alert infrastructure (8 features)
- **[Phase 9: Alert Extensions](Phase-9-Alert-Extensions)** - Production features (4 features)
- **[Project Architecture](Project-Architecture)** - System design and integration
- **[Installation & Configuration](Installation-Configuration)** - Setup and customization
- **[Development Roadmap](Development-Roadmap)** - Future enhancements

## Project Overview

### Vision
Bring amateur radio operators real-time decision-making tools through:
- **Low-latency alerting** for DX opportunities and satellite passes
- **Space weather intelligence** for HF propagation forecasting
- **Local-first architecture** with optional cloud/home automation integration
- **Modern development** using Rust for safety, performance, and maintainability

### Key Achievements

✅ **60-80% Startup Optimization** (Phase 4)
- Window visible in ~80ms
- GPU initialization deferred to event loop
- Parallel configuration loading

✅ **Comprehensive Alert System** (Phase 8)
- 6 alert types with severity levels
- Audio alerts and visual indicators
- Real-time alert deduplication and management
- Keyboard acknowledgment controls

✅ **Production-Ready Extensions** (Phase 9)
- SQLite alert history with retention policies
- Cross-platform desktop notifications
- MQTT publishing for home automation
- Remote web dashboard with WebSocket updates

### Performance Metrics

| Metric | Target | Achieved |
|--------|--------|----------|
| Time to Window | <100ms | ✅ ~80ms |
| Idle CPU Usage | <2% | ✅ <2% |
| Alert Latency | <500ms | ✅ <100ms |
| Memory (binary) | <10MB | ✅ 9.2MB |
| FPS (idle) | 1 FPS | ✅ 1 FPS |

## Technology Stack

### Core
- **Language**: Rust (2021 edition)
- **Runtime**: Tokio (async/await)
- **Graphics**: wgpu (GPU-accelerated rendering)

### Phase 8 (Alert System)
- **Configuration**: Serde (TOML/JSON)
- **Time**: Chrono (timezone-aware timestamps)
- **Audio**: System audio (beep on alert)

### Phase 9 (Alert Extensions)
- **Database**: rusqlite (embedded SQLite with bundled binary)
- **Notifications**: notify-rust (XDG/D-Bus/NSNotificationCenter)
- **MQTT**: rumqttc (async MQTT client with QoS support)
- **Web Server**: axum (modern async HTTP/WebSocket framework)

## Getting Started

### Prerequisites
- Rust 1.75+ (check with `rustc --version`)
- Tokio runtime (included in dependencies)
- (Optional) MQTT broker for home automation
- (Optional) N2YO API key for satellite tracking

### Installation

```bash
# Clone repository
git clone https://github.com/yourusername/hamclock.git
cd hamclock/rust-src

# Build optimized binary
cargo build --release

# Run
./target/release/hamclock
```

### Configuration

Create `~/.hamclock/config.toml`:

```toml
[alert_config]
dx_alerts_enabled = true
watched_bands = [14.074, 7.074, 3.573]  # 20m, 40m, 80m FT8
watched_modes = ["FT8", "CW"]
satellite_alerts_enabled = true
watched_satellites = ["ISS", "SO-50"]
space_weather_alerts_enabled = true

[phase9]
history_enabled = true
desktop_notifications_enabled = true
mqtt_enabled = true
mqtt_broker_url = "mqtt://localhost:1883"
web_dashboard_enabled = true
web_dashboard_port = 8080
```

Access web dashboard at: **http://localhost:8080**

## Project Statistics

- **Total LOC**: ~2,100 (Rust core)
- **Dependencies**: 28 (well-curated, security-audited)
- **Build Time**: ~45s (debug) / ~2m (release)
- **Binary Size**: 9.2MB (stripped)
- **Memory Usage**: ~45MB (typical)

## Feature Matrix

| Feature | Phase | Type | Status |
|---------|-------|------|--------|
| DX Band Monitoring | 8 | Alert | ✅ Complete |
| Satellite Pass Notifications | 8 | Alert | ✅ Complete |
| Kp Spike Alerts | 8 | Alert | ✅ Complete |
| X-Ray Flare Alerts | 8 | Alert | ✅ Complete |
| Aurora Visibility | 8 | Alert | ✅ Complete |
| CME Detection | 8 | Alert | ✅ Complete |
| Audio Alerts | 8 | Notification | ✅ Complete |
| Alert Acknowledgment | 8 | UI | ✅ Complete |
| SQLite History | 9 | Storage | ✅ Complete |
| Desktop Notifications | 9 | Notification | ✅ Complete |
| MQTT Publishing | 9 | Integration | ✅ Complete |
| Web Dashboard | 9 | UI | ✅ Complete |

## Community & Contributing

### Reporting Issues
Found a bug? Have a suggestion? Open an issue on GitHub.

### Contributing
We welcome contributions! Please:
1. Fork the repository
2. Create a feature branch: `git checkout -b feature/your-feature`
3. Follow [Rust API Guidelines](https://rust-lang.github.io/api-guidelines/)
4. Submit a pull request

### Code of Conduct
Please note that this project is released with a [Contributor Code of Conduct](CODE_OF_CONDUCT.md).

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- **N2YO** for satellite tracking data (https://www.n2yo.com)
- **DX Cluster Network** for real-time DX spots (https://www.dxcluster.org)
- **NOAA** for space weather forecasts (https://www.swpc.noaa.gov)
- **Rust Community** for amazing tools and libraries

## Contact

Questions or feedback? Reach out:
- **GitHub Issues**: [HamClock Issues](https://github.com/yourusername/hamclock/issues)
- **Email**: your-email@example.com

---

**Last Updated**: 2025-12-19 | **Version**: 0.1.0-phase9 | **Status**: Production Alpha
