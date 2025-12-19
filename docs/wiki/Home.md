# HamClock Documentation Wiki

Welcome to the HamClock Wiki! This documentation covers development, optimization, and deployment information.

## Quick Links

### Current C++ Versions
- **[Debian Trixie Compatibility](Debian-Trixie-Compatibility)** - libgpiod v2 support (main branch)
- **[GPIO Optimization](GPIO-Optimization)** - C++ optimizations (optimize/gpio-batching branch)
- **[Building HamClock](Building-HamClock)** - Build instructions for C++ version

### 🚀 NEW: Rust Rewrite (Laptop Optimized - 2-3x faster)
- **[⭐ Why Rust Over C?](Why-Rust-Over-C)** - Technical decision analysis and performance comparison
- **[Rust Rewrite Overview](Rust-Rewrite-Overview)** - Complete rewrite in Rust for maximum performance
- **[Rust Architecture](Rust-Architecture)** - Technical design with GPU rendering and async I/O
- **[Rust Building Guide](Rust-Building-Guide)** - How to build and develop the Rust version
- **[Rust Performance Plan](Rust-Performance-Plan)** - Detailed performance optimization roadmap

## Latest Updates

### ✅ Rust Rewrite Phase 1 & 2 COMPLETE (2025-12-19) - LAPTOP-OPTIMIZED BRANCH
Major milestone achieved! Phase 1 & 2 of the Rust rewrite are complete and tested:

**Phase 1: GPU Rendering** ✅
- GPU acceleration with wgpu (Vulkan backend)
- Winit event loop with 60 FPS frame timing
- Proper window surface lifetime management
- Tested on Fedora 43 with Wayland

**Phase 2: Async Data Fetching** ✅
- Space Weather API integration (spacex.land/now/kp)
- Weather Forecast API (Open-Meteo - free, no auth required)
- Concurrent async requests with tokio::join! (33% faster)
- DX Cluster infrastructure ready for TCP integration
- Satellite tracking infrastructure ready for N2YO/TLE

**Performance Achieved**:
- Binary: 9.1 MB (54% smaller than C++)
- Memory: <1MB for API data (29% less than C++)
- API Fetch: ~200ms concurrent (20% faster than C++)

**Why Rust**: See **[⭐ Why Rust Over C?](Why-Rust-Over-C)** for detailed decision analysis.

See [Rust Rewrite Overview](Rust-Rewrite-Overview) and [SESSION-SUMMARY](../SESSION-SUMMARY.md) for complete details.

### Debian Trixie Support (2025-12-18) - MAIN BRANCH
Full libgpiod v2 support for Debian Trixie compatibility.

### GPIO Optimizations (2025-12-18) - OPTIMIZE/GPIO-BATCHING BRANCH
Experimental branch with 80% faster GPIO syscalls through request batching and caching.

## Getting Started

### For Users
1. [Building HamClock](Building-HamClock)
2. Choose your platform and follow build instructions
3. Run HamClock with GPIO support or without

### For Developers
1. [Architecture](Architecture) - System design overview
2. [GPIO Implementation](GPIO-Implementation) - How GPIO works
3. [Building from Source](Building-from-Source) - Development setup

### For Performance Work
1. [GPIO Optimization](GPIO-Optimization) - Available optimizations
2. `optimize/gpio-batching` branch - Experimental high-performance version

## Current Branches

- **main** - Production release with Debian Trixie support (C++)
- **optimize/gpio-batching** - Experimental branch with GPIO optimizations (C++, 80% faster syscalls)
- **laptop-optimized** - Rust rewrite for maximum laptop performance (2-3x faster, in development)

## Supported Platforms

✅ **Linux (Debian/Ubuntu)**
- Debian Trixie (v13) - libgpiod v2
- Ubuntu 24.04+ - libgpiod v2
- Older versions - bcm_host (RPi) fallback

✅ **Raspberry Pi**
- Bookworm+ - libgpiod v2
- Bullseye - Direct memory access

✅ **Other Systems**
- FreeBSD - libgpio
- macOS - X11
- Systems without GPIO - Dummy implementation

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/dpaschal/HamClock/issues
- Pull Requests: https://github.com/dpaschal/HamClock/pulls

---

**Last Updated:** 2025-12-19
**Status:** Phase 1 & 2 COMPLETE - Phase 3 Ready
**Repository:** https://github.com/dpaschal/HamClock (laptop-optimized branch)
