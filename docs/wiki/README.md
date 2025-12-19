# HamClock Documentation Wiki

This directory contains comprehensive documentation for HamClock development, optimization, and deployment.

## Quick Navigation

- **[Home](Home.md)** - Overview and quick links
- **[Debian Trixie Compatibility](Debian-Trixie-Compatibility.md)** - libgpiod v2 support
- **[GPIO Optimization](GPIO-Optimization.md)** - Performance improvements (80% faster)
- **[Building HamClock](Building-HamClock.md)** - Build instructions for all platforms

## Latest Updates

### 2025-12-18: GPIO Optimization
Comprehensive performance optimizations completed on `optimize/gpio-batching` branch:
- 80% reduction in GPIO initialization syscalls
- Request batching and lazy configuration
- Software pin state caching
- Read-write lock for better concurrency

See [GPIO Optimization](GPIO-Optimization.md) for details.

### 2025-12-18: Debian Trixie Support
Full libgpiod v2 support merged to main branch:
- Automatic detection of libgpiod
- 100% backward compatible
- Graceful fallback to older methods
- Production ready

See [Debian Trixie Compatibility](Debian-Trixie-Compatibility.md) for details.

## Documentation Structure

```
docs/wiki/
├── README.md                           (This file)
├── Home.md                             (Main entry point)
├── Debian-Trixie-Compatibility.md     (libgpiod v2 implementation)
├── GPIO-Optimization.md                (Performance improvements)
└── Building-HamClock.md                (Build instructions)
```

## Getting Started

### For Users
1. Start with [Home.md](Home.md)
2. Choose your platform in [Building HamClock](Building-HamClock.md)
3. Follow build instructions

### For Developers
1. Read [Home.md](Home.md) overview
2. Check [Debian Trixie Compatibility](Debian-Trixie-Compatibility.md) for GPIO details
3. Review [GPIO Optimization](GPIO-Optimization.md) for performance insights

### For Performance Work
1. Review [GPIO Optimization](GPIO-Optimization.md)
2. Check `optimize/gpio-batching` branch for implementation
3. Follow benchmarking guide

## Current Branches

- **main** - Production release with Debian Trixie support
- **optimize/gpio-batching** - Performance-optimized GPIO (80% faster syscalls)

## Key Features

✅ Debian Trixie (v13) support via libgpiod v2
✅ 80% performance improvement available (optimize/gpio-batching)
✅ 100% backward compatible
✅ Multiple platform support (Linux, RPi, FreeBSD, macOS)
✅ Comprehensive error handling
✅ Thread-safe GPIO operations

## Repository

**GitHub:** https://github.com/dpaschal/HamClock
**Wiki:** This documentation
**Issues:** https://github.com/dpaschal/HamClock/issues

---

**Last Updated:** 2025-12-18
**Status:** Production Ready ✅
