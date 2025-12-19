# Building HamClock

Quick build instructions for various platforms.

## Prerequisites

### Required Packages

#### Debian/Ubuntu (including Trixie)
```bash
sudo apt-get update
sudo apt-get install build-essential git libx11-dev
```

#### With GPIO Support (Trixie, Bookworm+)
```bash
sudo apt-get install libgpiod-dev
```

#### Raspberry Pi OS
```bash
sudo apt-get install build-essential git
# GPIO headers included in Raspberry Pi OS
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install X11 (XQuartz)
# Download from https://www.xquartz.org/
```

#### FreeBSD
```bash
pkg install gcc gmake x11-libs libgpio
```

## Building

### Basic Build (Desktop X11)

```bash
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-800x480
```

Output: `./hamclock-800x480`

### All Available Targets

```bash
# X11 Desktop Versions (any Linux/macOS with X11)
make hamclock-800x480       # 800x480 (default, small screens)
make hamclock-1600x960      # 1600x960 (medium displays)
make hamclock-2400x1440     # 2400x1440 (large displays)
make hamclock-3200x1920     # 3200x1920 (huge displays)

# Raspberry Pi Framebuffer (direct /dev/fb0)
make hamclock-fb0-800x480   # 800x480 (small screens)
make hamclock-fb0-1600x960  # 1600x960 (medium displays)
make hamclock-fb0-2400x1440 # 2400x1440 (large displays)
make hamclock-fb0-3200x1920 # 3200x1920 (huge displays)
```

### Clean Build

```bash
make clean
make hamclock-800x480
```

## Platform-Specific Instructions

### Debian Trixie

```bash
# Install dependencies
sudo apt-get install build-essential git libx11-dev libgpiod-dev

# Build
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-800x480

# Run
./hamclock-800x480
```

### Ubuntu 24.04+

```bash
# Same as Debian Trixie
sudo apt-get install build-essential git libx11-dev libgpiod-dev
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-800x480
```

### Raspberry Pi OS (Bookworm)

```bash
# Install dependencies
sudo apt-get install build-essential git libgpiod-dev

# Build framebuffer version (no X11 needed)
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-fb0

# Run
./hamclock-fb0
```

### Older Debian/Ubuntu (pre-Trixie)

```bash
# Install dependencies
sudo apt-get install build-essential git libx11-dev

# Build (uses bcm_host for GPIO)
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-800x480
```

### macOS with XQuartz

```bash
# Install XQuartz first: https://www.xquartz.org/

# Install dependencies
brew install git

# Clone and build
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
make hamclock-800x480
```

### FreeBSD

```bash
# Install dependencies
pkg install gcc gmake x11-libs libgpio

# Clone and build
git clone https://github.com/dpaschal/HamClock.git
cd HamClock
gmake hamclock-800x480  # Use gmake instead of make
```

## Installation (Optional)

### System-Wide Installation

```bash
# Requires built binary (run make first)
sudo make install

# Installs to /usr/local/bin/hamclock
# Sets SUID bit for GPIO access
```

### Uninstall

```bash
sudo rm /usr/local/bin/hamclock
```

## Troubleshooting

### "command not found: make"

**Linux:**
```bash
sudo apt-get install build-essential
```

**macOS:**
```bash
xcode-select --install
```

### "X11 not found"

**Linux:** Should work by default, check `libx11-dev` installed
```bash
dpkg -l | grep libx11-dev
```

**macOS:** Install XQuartz
```bash
# https://www.xquartz.org/
```

### "gpiod.h: No such file or directory"

**Solution:** Install libgpiod-dev (Debian Trixie/Bookworm+)
```bash
sudo apt-get install libgpiod-dev
make clean && make hamclock-800x480
```

**Note:** GPIO is optional, HamClock works without it

### Compilation errors

**Try clean rebuild:**
```bash
make clean
make hamclock-800x480
```

**Check dependencies:**
```bash
# Debian/Ubuntu
sudo apt-get install build-essential git libx11-dev libgpiod-dev

# Then retry
make clean hamclock-800x480
```

## Development Branches

### Current Main
```bash
# Latest stable with Trixie support
git checkout main
make hamclock-800x480
```

### Optimization Branch
```bash
# Performance-optimized GPIO (not yet merged)
git checkout optimize/gpio-batching
make hamclock-800x480
```

## Build Customization

### 16-bit Framebuffer (non-standard)

```bash
# Edit Makefile or pass flag
CXXFLAGS="-D_16BIT_FB" make hamclock-fb0
```

### Custom Resolution

Modify `Makefile` target or adjust at runtime.

## Performance Tips

### Building Faster

```bash
# Use multiple cores
make -j4 hamclock-800x480  # Use 4 cores

# Or parallel build
make -j$(nproc) hamclock-800x480  # All cores
```

### Optimization Branch

For 80% faster GPIO operations on Raspberry Pi:
```bash
git checkout optimize/gpio-batching
make hamclock-800x480
```

## Testing After Build

```bash
# Quick functionality test
./hamclock-800x480 --help

# Should display: Version and usage info
```

## Related Documentation

- [Debian Trixie Compatibility](Debian-Trixie-Compatibility) - Trixie-specific info
- [GPIO Optimization](GPIO-Optimization) - Performance improvements
- [Architecture](Architecture) - Technical design

---

**Last Updated:** 2025-12-18
