# HamClock Documentation Wiki

Welcome to the HamClock Wiki! This documentation covers development, optimization, and deployment information.

## Quick Links

### Current C++ Versions
- **[Debian Trixie Compatibility](Debian-Trixie-Compatibility)** - libgpiod v2 support (main branch)
- **[GPIO Optimization](GPIO-Optimization)** - C++ optimizations (optimize/gpio-batching branch)
- **[Building HamClock](Building-HamClock)** - Build instructions for C++ version

### 🚀 NEW: Rust Rewrite (Laptop Optimized - 2-3x faster)
- **[Rust Rewrite Overview](Rust-Rewrite-Overview)** - Complete rewrite in Rust for maximum performance
- **[Rust Architecture](Rust-Architecture)** - Technical design with GPU rendering and async I/O
- **[Rust Building Guide](Rust-Building-Guide)** - How to build and develop the Rust version
- **[Rust Performance Plan](Rust-Performance-Plan)** - Detailed performance optimization roadmap

## Latest Updates

### 🎉 Rust Rewrite Planned (2025-12-18) - LAPTOP-OPTIMIZED BRANCH
A complete Rust rewrite is in planning phase targeting 2-3x performance improvement:
- GPU acceleration (wgpu: OpenGL/Vulkan)
- Async data fetching (tokio runtime)
- Optimized for 1920x1200+ displays
- 60+ FPS rendering target
- < 100MB memory usage
- < 8% CPU at idle

See [Rust Rewrite Overview](Rust-Rewrite-Overview) for details.

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

**Last Updated:** 2025-12-18
