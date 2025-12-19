# HamClock Rust Rewrite - Laptop Optimized

**Status:** 🚀 Planned & Ready to Start
**Language:** Rust
**Branch:** `laptop-optimized`
**Target:** Fedora Laptops (and other Linux desktops)
**Performance Goal:** 2-3x faster than C++ version

---

## Executive Summary

A complete rewrite of HamClock in Rust optimized for modern laptop displays (1920x1200+).

### Why Rust?

✅ **Speed** - Native compiled code, no garbage collector
✅ **Safety** - Memory safety without runtime overhead
✅ **Async** - Native async/await for concurrent data fetching
✅ **Modern Ecosystem** - Excellent GPU libraries, async runtimes
✅ **Long-term Maintainability** - Compiler catches errors at compile time
✅ **Future-proof** - Rust is the future of systems programming

### Performance Targets

| Metric | C++ Version | Rust Target | Improvement |
|--------|------------|-------------|------------|
| Startup Time | ~3-5s | ~1-2s | 60-80% faster |
| Rendering FPS | 30-60 FPS | 60-120 FPS | 2-4x faster |
| Data Fetch Latency | Sequential | Parallel async | 50% faster |
| Memory Usage | ~150MB | ~80MB | 50% reduction |
| CPU Usage | 15-20% | 5-8% | 60% reduction |

## Architecture Changes

### C++ Version (Current)

```
┌─────────────────────────────────────┐
│      HamClock Application           │
├─────────────────────────────────────┤
│  Arduino Library (Abstraction)      │
├─────────────────────────────────────┤
│  Display Logic (X11 Drawing)        │
├─────────────────────────────────────┤
│  Data Fetching (Blocking HTTP)      │
├─────────────────────────────────────┤
│  OS-Specific GPIO Abstraction       │
└─────────────────────────────────────┘
```

**Problems:**
- X11 drawing is slow (CPU-bound)
- Blocking HTTP requests (waits for responses)
- Heavy abstraction layers (Arduino lib)
- GPIO code for laptops (unnecessary overhead)

### Rust Version (Proposed)

```
┌──────────────────────────────────────────┐
│   HamClock Rust (Modern, Optimized)      │
├──────────────────────────────────────────┤
│  GPU Rendering (wgpu: OpenGL/Vulkan)    │
│  ↓                                       │
│  Async Data Fetching (tokio runtime)    │
│  ↓                                       │
│  Modern UI (gtk-rs or egui)             │
│  ↓                                       │
│  Direct Hardware Access (no Arduino)    │
└──────────────────────────────────────────┘
```

**Benefits:**
- GPU acceleration (off-loads CPU)
- Async/concurrent data fetching (non-blocking)
- Minimal abstraction layers
- No unnecessary GPIO code
- Modern Rust patterns

## Feature Parity

### HamClock Features Retained

✅ Radio propagation maps (MUF, FOE, etc)
✅ Real-time space weather data
✅ Sunrise/sunset calculations
✅ Moon phase and position
✅ Multiple time displays
✅ DX Cluster integration
✅ Satellite tracking
✅ Weather forecasts
✅ All display modes

### New Rust Benefits

✅ GPU-accelerated rendering
✅ Concurrent data fetching
✅ Touch/mouse interaction improvements
✅ Better error handling
✅ Memory-safe operations
✅ Faster startup (lazy loading)

## Development Timeline

### Phase 1: Core Data Engine (Weeks 1-2)
```
- Rust project setup
- Data structure definitions
- Network fetching (tokio + reqwest)
- Parser implementations (space weather, forecasts)
- Unit tests
```

### Phase 2: Rendering Engine (Weeks 2-3)
```
- GPU setup (wgpu)
- 2D graphics primitives
- Map rendering
- Text rendering
- Widget system (buttons, etc)
```

### Phase 3: UI & Integration (Weeks 3-4)
```
- Main window creation
- Tab/view switching
- Mouse/keyboard interaction
- Settings/preferences
- Data caching
```

### Phase 4: Optimization & Polish (Weeks 4-5)
```
- Performance profiling
- Memory optimization
- Async refinement
- Error handling
- Testing on multiple resolutions
```

### Phase 5: Documentation & Release (Week 5-6)
```
- Build instructions
- API documentation
- User guide updates
- Performance benchmarks
- Release packaging
```

## Technology Stack

### Core Dependencies

| Component | Library | Why |
|-----------|---------|-----|
| Async Runtime | tokio | Best-in-class async/await |
| HTTP Client | reqwest | Easy async HTTP |
| JSON Parser | serde_json | Fast, zero-copy |
| Graphics | wgpu | Cross-platform GPU access |
| UI Framework | gtk-rs or egui | Modern GUI |
| DateTime | chrono | Timezone handling |
| Math | nalgebra | Linear algebra |

### Build System

- **Language:** Rust 1.70+
- **Build Tool:** cargo
- **Package Manager:** crates.io
- **Testing:** cargo test
- **Documentation:** cargo doc

## Target Platforms

### Primary
- **Fedora 43** - Full optimization
- **Ubuntu 24.04+** - Full support
- **Debian Trixie** - Full support

### Secondary
- **Arch Linux** - Full support
- **Fedora 42** - Full support
- **CentOS Stream** - Full support

### System Requirements

- **CPU:** Any 64-bit processor
- **RAM:** 512MB minimum (target: <100MB usage)
- **GPU:** Any GPU with OpenGL 4.6+ or Vulkan support
- **Display:** 1366x768 minimum, 1920x1200 target

## Build Instructions

### Prerequisites

```bash
# Install Rust toolchain
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env

# Install system dependencies (Fedora)
sudo dnf install gtk3-devel libxcb-devel libxrender-devel
```

### Build

```bash
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
git checkout laptop-optimized

cargo build --release
./target/release/hamclock
```

## Performance Expectations

### Startup

```
C++ Version:   3-5 seconds
Rust Version:  1-2 seconds
Improvement:   60-80% faster
```

### Runtime

```
C++ Version:   15-20% CPU, 150MB RAM, 30-60 FPS
Rust Version:   5-8% CPU, 80MB RAM, 60-120 FPS
Improvement:   2-3x overall performance
```

### Data Fetching

```
C++ Version:   Sequential - waits for each request (5-10 seconds total)
Rust Version:  Parallel async - concurrent requests (1-2 seconds total)
Improvement:   50-80% faster data loading
```

## Repository Structure

```
HamClock/
├── Cargo.toml                # Rust project config
├── src/
│   ├── main.rs              # Application entry
│   ├── lib.rs               # Library exports
│   ├── data/
│   │   ├── fetcher.rs       # Data fetching (async)
│   │   ├── parsers.rs       # Data parsing
│   │   └── models.rs        # Data structures
│   ├── render/
│   │   ├── gpu.rs           # GPU initialization (wgpu)
│   │   ├── shapes.rs        # 2D primitives
│   │   └── ui.rs            # UI components
│   ├── ui/
│   │   ├── window.rs        # Main window
│   │   ├── tabs.rs          # Tab views
│   │   └── controls.rs      # Buttons, etc
│   └── config.rs            # Settings
├── docs/wiki/               # Documentation
└── README.md
```

## Getting Started (for Contributors)

1. **Read:** [Rust-Architecture.md](Rust-Architecture.md)
2. **Build:** [Rust-Building-Guide.md](Rust-Building-Guide.md)
3. **Contribute:** Follow performance plan in [Rust-Performance-Plan.md](Rust-Performance-Plan.md)

## Performance Monitoring

### Profiling Tools

```bash
# CPU profiling
cargo flamegraph

# Memory profiling
valgrind --tool=massif ./target/release/hamclock

# GPU profiling
apitrace trace hamclock  # For OpenGL
```

## Known Limitations (To Address)

- GPIO is disabled (laptops don't have pins)
- Single GPU device support (most laptops have one GPU)
- X11 focus (Wayland support for future)

## Future Enhancements

- [ ] Wayland support
- [ ] Mobile app (iOS/Android)
- [ ] Web UI (wasm + web technologies)
- [ ] Multi-GPU rendering
- [ ] AR mode for radio propagation
- [ ] Touch screen optimization

## Success Criteria

✅ Builds without warnings on Fedora 43
✅ Runs 2-3x faster than C++ version
✅ Uses <100MB RAM at runtime
✅ Feature parity with C++ HamClock
✅ Compiles in <2 minutes (release build)
✅ All tests pass
✅ Documentation complete
✅ Benchmarks published

## References

- [Rust Book](https://doc.rust-lang.org/book/)
- [wgpu Documentation](https://docs.rs/wgpu/)
- [tokio Tutorial](https://tokio.rs/)
- [gtk-rs Guide](https://gtk-rs.org/)

---

**Branch:** `laptop-optimized`
**Status:** Ready for implementation
**Created:** 2025-12-18
