# HamClock C - Build Instructions

**Last Updated:** 2025-12-22
**Platforms Supported:** Linux x86_64, ARM, ARM64
**Build System:** CMake 3.10+
**Language:** C11

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Linux Desktop](#linux-desktop)
3. [Raspberry Pi (Native Build)](#raspberry-pi-native-build)
4. [Raspberry Pi (Cross-Compile)](#raspberry-pi-cross-compile)
5. [Docker](#docker)
6. [Troubleshooting](#troubleshooting)

---

## Quick Start

### Minimal Build (Fedora/RHEL)

```bash
sudo dnf install gcc cmake SDL2-devel SDL2_ttf-devel sqlite-devel libcurl-devel
cd hamclock-c
mkdir build && cd build
cmake .. && make -j4
./hamclock
```

### Minimal Build (Ubuntu/Debian)

```bash
sudo apt update
sudo apt install gcc cmake libsdl2-dev libsdl2-ttf-dev libsqlite3-dev libcurl4-openssl-dev
cd hamclock-c
mkdir build && cd build
cmake .. && make -j4
./hamclock
```

---

## Linux Desktop

### Step 1: Install Dependencies

#### Fedora 43
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake pkg-config
sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel
sudo dnf install sqlite-devel libcurl-devel
```

#### Ubuntu 24.04 / Debian 12
```bash
sudo apt update
sudo apt install build-essential cmake pkg-config
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev
sudo apt install libsqlite3-dev libcurl4-openssl-dev
```

#### Alpine Linux (Minimal)
```bash
apk add alpine-sdk cmake pkgconfig
apk add sdl2-dev sdl2_ttf-dev sqlite-dev curl-dev
```

### Step 2: Clone Repository

```bash
git clone https://github.com/your-username/hamclock-c.git
cd hamclock-c
```

### Step 3: Configure Build

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

**Build Types:**
- `Release` - Optimized binary, no symbols (-O2 -flto)
- `Debug` - Debug symbols, address sanitizer (-O0 -g3)
- `MinSizeRel` - Size-optimized (-Os)

### Step 4: Build

```bash
make -j4            # Use 4 parallel jobs
# or
make -j$(nproc)     # Use all CPU cores
```

**Build Output:**
```
[ 42%] Built target hamclock
[100%] Built target test_timezone
[100%] Built target test_sun
[100%] Built target test_maidenhead
```

### Step 5: Run

```bash
./hamclock
```

**First Run:**
- Creates `hamclock.db` (SQLite database)
- Initializes configuration
- Opens SDL2 window
- Displays greyline map + clocks

### Step 6: (Optional) Install

```bash
sudo make install   # Installs to /usr/local/bin/hamclock
hamclock &          # Run from anywhere
```

---

## Raspberry Pi (Native Build)

### On Raspberry Pi 4 (bullseye or later)

#### Step 1: Update System

```bash
sudo apt update
sudo apt upgrade -y
```

#### Step 2: Install Build Tools

```bash
sudo apt install -y build-essential cmake git pkg-config
```

#### Step 3: Install Libraries

```bash
sudo apt install -y \
    libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
    libsqlite3-dev libcurl4-openssl-dev
```

#### Step 4: Clone & Build

```bash
git clone https://github.com/your-username/hamclock-c.git
cd hamclock-c
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

**Note:** Native ARM build is slow (5-10 minutes on RPi 4)

#### Step 5: Run

```bash
./hamclock
```

#### Step 6: Autostart (Optional)

Create `/home/pi/start-hamclock.sh`:
```bash
#!/bin/bash
cd /home/pi/hamclock-c/build
./hamclock
```

Make executable:
```bash
chmod +x /home/pi/start-hamclock.sh
```

Add to crontab (`crontab -e`):
```bash
@reboot /home/pi/start-hamclock.sh
```

---

## Raspberry Pi (Cross-Compile)

Build on **Linux x86_64** for **Raspberry Pi ARM**, much faster!

### Cross-Compile for Pi 4 (ARM64)

#### Step 1: Install Cross-Compiler (Ubuntu)

```bash
sudo apt install -y \
    gcc-aarch64-linux-gnu g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    aarch64-linux-gnu-gfortran
```

#### Fedora

```bash
sudo dnf install -y \
    gcc-c++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    glibc-devel-aarch64-linux-gnu
```

#### Step 2: Configure & Build

```bash
cd hamclock-c
mkdir build-arm64
cd build-arm64

cmake \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64-rpi.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    ..

make -j8    # Much faster on x86_64 (8-30 seconds)
```

#### Step 3: Verify Binary

```bash
file hamclock
# Should show: ELF 64-bit LSB executable, ARM aarch64, ...

ls -lh hamclock
# Should be ~44 KB
```

#### Step 4: Transfer to Pi

**Option A: SSH (Network)**
```bash
scp hamclock pi@raspberrypi.local:~/
ssh pi@raspberrypi.local
chmod +x hamclock
./hamclock
```

**Option B: USB Drive**
```bash
cp hamclock /media/usb/
# Transfer USB to Raspberry Pi
```

**Option C: Git on Pi**
```bash
# On Pi:
git clone https://github.com/your-username/hamclock-c.git
cd hamclock-c/build-arm64
file hamclock  # Verify ARM64 binary
./hamclock
```

### Cross-Compile for Pi 3 (ARMv7)

#### Step 1: Install Cross-Compiler (Ubuntu)

```bash
sudo apt install -y \
    gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf \
    binutils-arm-linux-gnueabihf
```

#### Fedora

```bash
sudo dnf install -y gcc-c++-arm-linux-gnu binutils-arm-linux-gnu
```

#### Step 2: Build

```bash
mkdir build-armv7
cd build-armv7
cmake \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-armv7-rpi.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    ..
make -j8
```

#### Step 3: Transfer & Run

```bash
scp hamclock pi@raspberrypi.local:~/
# Run on RPi 3
```

---

## Docker

### Build Docker Image

Create `Dockerfile`:
```dockerfile
FROM ubuntu:24.04

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential cmake pkg-config \
    libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev \
    libsqlite3-dev libcurl4-openssl-dev

COPY . .
RUN mkdir build && cd build && cmake .. && make -j4

CMD ["./build/hamclock"]
```

### Build & Run

```bash
docker build -t hamclock:latest .
docker run -it hamclock:latest
```

### Run in Container (Headless)

To use display output:
```bash
docker run -it \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    hamclock:latest
```

---

## Build Options

### CMake Configuration

```bash
cd build
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm64-rpi.cmake \
    -DCMAKE_INSTALL_PREFIX=/usr/local
```

### Custom Optimization

**For maximum performance:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
# Uses -O2 -flto (default)
```

**For minimum size:**
```bash
# Edit CMakeLists.txt, change O2 to Os:
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Os")
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4
```

**Result:** 41 KB binary (vs 44 KB)

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j4
gdb ./hamclock  # Debug with GDB
```

---

## Testing

### Build Tests

```bash
cd build
make test          # Build tests only
ctest              # Run tests
ctest -V           # Verbose output
ctest --output-on-failure  # Show failures
```

### Individual Tests

```bash
./tests/test_timezone
./tests/test_sun
./tests/test_maidenhead
```

---

## Cleaning

### Remove Build

```bash
rm -rf build build-arm64 build-armv7
```

### Clean Build Directory

```bash
cd build
make clean
```

### Deep Clean

```bash
cd build
cmake --build . --target clean
rm -f hamclock.db      # Remove database
```

---

## Installation

### System-Wide Install

```bash
cd build
sudo make install     # Installs to /usr/local/bin/hamclock
which hamclock        # Verify
hamclock              # Run from anywhere
```

### Local Install

```bash
make install DESTDIR=$HOME/.local PREFIX=
~/.local/bin/hamclock
```

### Portable Install

Just use the binary:
```bash
./build/hamclock      # Run from project directory
```

---

## Troubleshooting

### Error: "CMake not found"

**Solution:** Install CMake
```bash
# Ubuntu
sudo apt install cmake

# Fedora
sudo dnf install cmake

# macOS
brew install cmake
```

### Error: "SDL2 not found"

**Solution:** Install SDL2 development libraries
```bash
# Ubuntu
sudo apt install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev

# Fedora
sudo dnf install SDL2-devel SDL2_ttf-devel SDL2_image-devel
```

### Error: "gcc: command not found"

**Solution:** Install build tools
```bash
# Ubuntu
sudo apt install build-essential

# Fedora
sudo dnf groupinstall "Development Tools"
```

### Error: "undefined reference to SDL_Init"

**Problem:** SDL2 not properly linked

**Solution:**
```bash
cd build
rm CMakeCache.txt    # Clear CMake cache
cmake ..             # Reconfigure
make -j4
```

### Error: "pkg-config: command not found"

**Solution:** Install pkg-config
```bash
# Ubuntu
sudo apt install pkg-config

# Fedora
sudo dnf install pkgconfig
```

### Warning: "10 warnings" during compile

**Status:** Non-critical, safe to ignore

**To fix:** Edit source files with unused variables

**Example:**
```bash
# In src/main.c, change:
int main(int argc, char *argv[]) {
# To:
int main(int argc __attribute__((unused)), char *argv[] __attribute__((unused))) {
```

### Build is Very Slow

**Problem:** Using single-threaded build

**Solution:**
```bash
make -j$(nproc)     # Use all CPU cores
# or
make -j8            # Use 8 parallel jobs
```

### Binary Size > 50KB

**Cause:** Unstripped binary (with debug symbols)

**Solution:**
```bash
aarch64-linux-gnu-strip hamclock    # For ARM64
strip hamclock                       # For x86_64
ls -lh hamclock
# Should be ~44 KB now
```

---

## Advanced Topics

### Static Linking

To create standalone binary:
```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_STATIC=ON ..
make -j4
```

**Result:** Larger binary (~100+ KB) but no runtime dependencies

### Link-Time Optimization (LTO)

Already enabled by default (-flto flag)

To verify:
```bash
grep -i "flto" CMakeLists.txt
```

### Profile-Guided Optimization (PGO)

```bash
# Step 1: Compile with profiling
cmake -DCMAKE_CXX_FLAGS="-fprofile-generate" ..
make

# Step 2: Run with typical workload
./hamclock &
sleep 30
killall hamclock

# Step 3: Recompile with profiling data
cmake -DCMAKE_CXX_FLAGS="-fprofile-use -fprofile-correction" ..
make -j4
```

---

## Build Matrix

| Platform | Compiler | Build Time | Binary Size | Status |
|----------|----------|-----------|-------------|--------|
| Linux x86_64 | GCC 15 | ~30s | 44 KB | ✅ Verified |
| Linux ARMv7 | ARM GCC | ~45s | 44 KB | ⚙️ Cross-compile |
| Linux ARM64 | ARM64 GCC | ~40s | 44 KB | ⚙️ Cross-compile |
| Raspberry Pi 3 | ARM GCC | ~5min | 44 KB | ⚠️ Slow native |
| Raspberry Pi 4 | ARM64 GCC | ~2min | 44 KB | ✅ Native build |

---

## See Also

- **CROSS_COMPILE.md** - Detailed cross-compilation guide
- **README.md** - Project overview
- **TESTING.md** - Test suite documentation
- **CMakeLists.txt** - Build configuration

