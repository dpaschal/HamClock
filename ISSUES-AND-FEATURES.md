# HamClock Issues & Feature Requests Tracker

**Last Updated:** 2025-12-19
**Status:** Active Development - Phase 12 Visual Rendering

---

## 📋 Overview

This document tracks all known bugs, feature requests, and enhancement proposals for HamClock. Issues are prioritized by severity and impact on user experience.

### How to Report Issues
1. Go to [GitHub Issues](https://github.com/dpaschal/HamClock/issues)
2. Click "New Issue"
3. Use the appropriate template (Bug Report, Feature Request, or Enhancement)
4. Provide detailed information and reproduction steps

---

## 🐛 Known Bugs & Fixes

### Recently Fixed ✅

#### Bug #1: Rendering Not Working - Text Invisible
**Status:** ✅ FIXED (Commit: 752aa3e)
**Severity:** CRITICAL
**Description:** Window displays blank purple screen with no UI text after Phase 11-12 integration
**Root Causes:**
1. GridLine field initialization error (duplicate `lat1` fields)
2. Font loading failure (invalid Roboto-Regular.ttf)
3. Disk space exhaustion (5GB build artifacts)
**Fix Applied:**
- Corrected GridLine struct field assignments
- Replaced font with valid Liberation Sans
- Cleaned build cache
**Testing:** ✅ Text rendering verified working

---

### Active / Reported Bugs 🔴

*Currently none reported*

**To Report a Bug:**
- Describe the issue clearly
- Include reproduction steps
- Provide system information (OS, GPU, resolution)
- Attach screenshot or logs if applicable
- Link to related code if known

---

## ✨ Feature Requests

### Tier 1: High Priority (Next Phase)

#### Feature #1: Layer Rendering Pipeline Optimization
**Status:** PLANNED (Phase 12.2)
**Priority:** HIGH
**Category:** Performance
**Description:** Optimize GPU rendering pipeline for multi-layer compositing
**Requirements:**
- Efficient texture binding and updates
- Minimal render pass overhead
- Support for opacity and blend modes
- <5ms per frame render time
**Estimated LOC:** 150-200
**Files:** `src/render/gpu.rs`, `src/render/layers.rs`

#### Feature #2: MUF Heatmap Rendering
**Status:** IN DEVELOPMENT (Phase 12.2)
**Priority:** HIGH
**Category:** Visualization
**Description:** GPU-rendered MUF heatmap overlay with real-time updates
**Requirements:**
- 5-color gradient texture (Blue→Green→Yellow→Red)
- Dynamic texture updates from forecast data
- Configurable opacity
- Proper coordinate mapping (lat/lon → screen)
**Estimated LOC:** 200-250
**Files:** `src/render/heatmap.rs`, `src/render/gpu.rs`
**Dependencies:** Phase 10 (HF Forecasting) ✅

#### Feature #3: Aurora Oval Rendering
**Status:** IN DEVELOPMENT (Phase 12.3)
**Priority:** HIGH
**Category:** Visualization
**Description:** Real-time aurora oval contour rendering with Kp-based intensity
**Requirements:**
- 360-point contour generation
- Kp-based color mapping
- Distorted oval shape for realism
- Real-time updates from space weather data
**Estimated LOC:** 150-200
**Files:** `src/render/aurora.rs`, `src/render/gpu.rs`
**Dependencies:** Phase 5 (Space Weather) ✅

#### Feature #4: Maidenhead Grid Rendering
**Status:** COMPLETED (Phase 12.4)
**Priority:** HIGH
**Category:** Visualization
**Description:** Grid lines and labels for Maidenhead locator system
**Status:** ✅ Module complete, awaiting GPU integration
**Files:** `src/render/grid.rs` ✅

---

### Tier 2: Medium Priority (Future Phases)

#### Feature #5: Web Dashboard Remote Monitoring
**Status:** PLANNED (Phase 9.4)
**Priority:** MEDIUM
**Category:** User Interface
**Description:** HTTP/WebSocket server for real-time remote alert monitoring
**Requirements:**
- Lightweight HTTP server (axum)
- WebSocket for real-time updates
- JSON alert feed
- Browser-based dashboard HTML/CSS
- Multi-client support
**Estimated LOC:** 200-300
**Dependencies:** Phase 8 (Alerts) ✅, Phase 9 (MQTT) ✅

#### Feature #6: MQTT Alert Publishing
**Status:** PLANNED (Phase 9.3)
**Priority:** MEDIUM
**Category:** Integration
**Description:** Publish alerts to MQTT broker for home automation integration
**Requirements:**
- MQTT client connection management
- QoS level support (0, 1, 2)
- Topic-based alert routing
- Auto-reconnection handling
**Estimated LOC:** 150-200
**Dependencies:** Phase 8 (Alerts) ✅

#### Feature #7: Desktop Native Notifications
**Status:** PLANNED (Phase 9.2)
**Priority:** MEDIUM
**Category:** User Experience
**Description:** System tray notifications for critical alerts (Linux/macOS/Windows)
**Requirements:**
- notify-rust integration
- Severity-based urgency levels
- Custom timeout configuration
- Cross-platform support
**Estimated LOC:** 100-150
**Dependencies:** Phase 8 (Alerts) ✅

#### Feature #8: SQLite Alert History & Logging
**Status:** PLANNED (Phase 9.1)
**Priority:** MEDIUM
**Category:** Data Management
**Description:** Persistent alert database with retention policies and query API
**Requirements:**
- SQLite database schema
- Retention policy enforcement (30-day default)
- Query interface for alert retrieval
- Automatic cleanup tasks
**Estimated LOC:** 200-250
**Dependencies:** Phase 8 (Alerts) ✅

---

### Tier 3: Lower Priority (Post-Phase 12)

#### Feature #9: Audio Alert System
**Status:** PLANNED (Post-Phase 12)
**Priority:** LOW
**Category:** User Experience
**Description:** Configurable audio alerts for critical space weather events
**Requirements:**
- Audio file synthesis (sine wave for different tones)
- Severity-based frequency mapping
- System audio integration
- Mute/volume control
**Estimated LOC:** 100-150
**Dependencies:** Phase 8 (Alerts) ✅

#### Feature #10: Geomagnetic Coordinate Rendering
**Status:** PROPOSED
**Priority:** LOW
**Category:** Visualization
**Description:** Display geomagnetic coordinates alongside geographic coordinates
**Requirements:**
- IGRF model implementation or library
- Real-time coordinate conversion
- Overlay toggle
**Estimated LOC:** 150-200

#### Feature #11: Historical Data Graphing
**Status:** PROPOSED
**Priority:** LOW
**Category:** Analysis
**Description:** Time-series graphs of Kp, Solar Flux, and alert frequency
**Requirements:**
- Historical data aggregation
- UI graphing library
- Configurable time ranges
- Export to CSV/PNG
**Estimated LOC:** 250-350
**Dependencies:** Feature #8 (SQLite History) ✅

#### Feature #12: CLI Alert Configuration
**Status:** PROPOSED
**Priority:** LOW
**Category:** User Interface
**Description:** Command-line interface for alert configuration without editing TOML
**Requirements:**
- CLI argument parsing
- Interactive configuration wizard
- Config validation
- Preset templates
**Estimated LOC:** 150-200

---

## 📊 Current Project Status

### Completed Phases ✅
- **Phase 1-4:** Core Infrastructure, GPU Rendering, Startup Optimization
- **Phase 5:** Space Weather Data Integration
- **Phase 6:** Satellite Pass Predictions
- **Phase 7:** DX Spots & Propagation Alerts
- **Phase 8:** Alert System Implementation
- **Phase 10:** HF Propagation Forecasting
- **Phase 11:** Advanced Visualization Layer System

### In Progress 🔄
- **Phase 12:** Visual Layer Rendering (Heatmap, Aurora, Grid)
  - ✅ Heatmap module (150 LOC)
  - ✅ Aurora module (200 LOC)
  - ✅ Grid module (180 LOC)
  - ⏳ GPU texture integration
  - ⏳ Layer rendering pipeline

### Planned Phases 📅
- **Phase 9:** Alert Extensions (History, Notifications, MQTT, Web Dashboard)
- **Phase 13:** Performance Optimization & Refactoring

---

## 🎯 Feature Request Template

**Title:** [Brief description]
**Priority:** [HIGH/MEDIUM/LOW]
**Category:** [Visualization/Performance/Integration/UI/Data]
**Description:** [Detailed explanation]
**Requirements:**
- [ ] Requirement 1
- [ ] Requirement 2
- [ ] Requirement 3
**Estimated LOC:** [Range]
**Dependencies:** [Other features or phases]
**Use Case:** [Why this feature matters]

---

## 🐛 Bug Report Template

**Title:** [Brief description]
**Severity:** [CRITICAL/HIGH/MEDIUM/LOW]
**System:** [OS, GPU, Resolution]
**Reproduction Steps:**
1. Step 1
2. Step 2
3. Step 3
**Expected Behavior:** [What should happen]
**Actual Behavior:** [What actually happened]
**Logs/Screenshots:** [Attach if available]
**Additional Context:** [Any other relevant info]

---

## 📈 Metrics & Goals

### Performance Targets
- **Startup Time:** < 100ms ✅ (Currently 75ms)
- **Frame Rate:** 60 FPS
- **Idle CPU:** < 3%
- **Memory:** < 200MB
- **Render Pass:** < 5ms per frame

### Quality Targets
- **Code Coverage:** > 80%
- **Warnings:** 0 (Currently 6, non-critical)
- **Test Suite:** Comprehensive unit + integration tests
- **Documentation:** Full API docs + user guide

### Competitive Goals (vs Geochron 4K)
- ✅ Free & Open Source
- ✅ GPU Accelerated Rendering
- ✅ Multi-layer Visualization
- ✅ Real-time Space Weather Integration
- ⏳ Heat Maps & Aurora Overlay
- ⏳ Web Dashboard
- ⏳ Mobile Companion App (Future)

---

## 🤝 Contributing

To contribute fixes or features:
1. Check this document to avoid duplicate work
2. Create a GitHub issue with your proposal
3. Fork the repository
4. Create a feature branch (`feature/description` or `fix/issue-number`)
5. Implement with tests
6. Submit a pull request
7. Reference the issue in your PR description

---

## 📞 Contact

- **Issues:** Report via GitHub Issues
- **Discussions:** Start a GitHub Discussion
- **Code Review:** Submit pull requests
- **Questions:** Check the [Wiki](https://github.com/dpaschal/HamClock/wiki)

---

## 📝 Notes

- This document is maintained alongside code commits
- Issues are triaged and prioritized monthly
- Phases may be adjusted based on user feedback
- Feature requests are welcome anytime
- Security issues should be reported privately

**Last synchronized with codebase:** Commit 752aa3e
