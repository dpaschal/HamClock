# HamClock Documentation Wiki

Welcome to the HamClock Wiki! This documentation covers development, optimization, and deployment information.

## Quick Links

- **[Debian Trixie Compatibility](Debian-Trixie-Compatibility)** - libgpiod v2 support for modern Linux systems
- **[GPIO Optimization](GPIO-Optimization)** - Performance improvements available on optimize/gpio-batching branch
- **[Building HamClock](Building-HamClock)** - Build instructions for various platforms

## Latest Updates

### Debian Trixie Support (2025-12-18) - MAIN BRANCH
Added full libgpiod v2 support for Debian Trixie compatibility on the main branch.

### GPIO Optimizations (2025-12-18) - EXPERIMENTAL BRANCH
Experimental branch `optimize/gpio-batching` now available with:
- 80% reduction in GPIO initialization syscalls
- Request batching and lazy configuration
- Software pin state caching
- Read-write lock for better concurrency

See [GPIO Optimization](GPIO-Optimization) for details.

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

- **main** - Production release with Debian Trixie support
- **optimize/gpio-batching** - Experimental branch with GPIO optimizations (80% faster syscalls)

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
