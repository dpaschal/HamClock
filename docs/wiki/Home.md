# HamClock Documentation Wiki

Welcome to the HamClock Wiki! This documentation covers development, optimization, and deployment information.

## Quick Links

- **[Debian Trixie Compatibility](Debian-Trixie-Compatibility)** - libgpiod v2 support for modern Linux systems
- **[GPIO Optimization](GPIO-Optimization)** - Performance improvements and benchmarking
- **[Building HamClock](Building-HamClock)** - Build instructions for various platforms
- **[Architecture](Architecture)** - System design and GPIO implementation details

## Latest Updates

### Debian Trixie Support (2025-12-18)
Added full libgpiod v2 support for Debian Trixie compatibility on the main branch.

### GPIO Optimizations (2025-12-18)
Implemented comprehensive GPIO optimizations on the `optimize/gpio-batching` branch:
- 80% reduction in GPIO initialization syscalls
- Request batching and lazy configuration
- Software pin state caching
- Read-write lock for better concurrency

See [GPIO Optimization](GPIO-Optimization) for details.

## Getting Started

### For Users
1. [Building HamClock](Building-HamClock)
2. [Supported Platforms](Supported-Platforms)
3. [Installation Guide](Installation-Guide)

### For Developers
1. [Architecture](Architecture)
2. [GPIO Implementation](GPIO-Implementation)
3. [Building from Source](Building-from-Source)
4. [Contributing](Contributing)

### For Performance Optimization
1. [GPIO Optimization](GPIO-Optimization) - Current optimizations
2. [Benchmarking](Benchmarking) - How to measure performance

## Current Branches

- **main** - Production release with Debian Trixie support
- **optimize/gpio-batching** - Experimental branch with GPIO optimizations (80% faster syscalls)

## Support

For issues, questions, or contributions:
- GitHub Issues: https://github.com/dpaschal/HamClock/issues
- Pull Requests: https://github.com/dpaschal/HamClock/pulls

---

**Last Updated:** 2025-12-18
