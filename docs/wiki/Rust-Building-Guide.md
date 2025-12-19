# Building HamClock Rust Rewrite

**Branch:** `laptop-optimized`
**Language:** Rust 1.70+
**Platform:** Linux (Fedora, Ubuntu, Debian)

---

## Prerequisites

### Rust Toolchain

```bash
# Install Rust (if not already installed)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Activate Rust
source $HOME/.cargo/env

# Verify installation
rustc --version
cargo --version
```

**Should show:** Rust 1.70.0 or newer

### Fedora 43 Dependencies

```bash
# Graphics and display
sudo dnf install gtk3-devel libxcb-devel libxrender-devel

# Build essentials
sudo dnf install gcc g++ pkg-config

# Optional: for GPU debugging
sudo dnf install vulkan-tools mesa-vulkan-devel
```

### Ubuntu 24.04+ Dependencies

```bash
# Graphics and display
sudo apt-get install libgtk-3-dev libxcb-render0-dev libxcb-shape0-dev

# Build essentials
sudo apt-get install build-essential pkg-config

# Optional: for GPU debugging
sudo apt-get install vulkan-tools mesa-vulkan-drivers
```

### Debian Trixie Dependencies

```bash
# Graphics and display
sudo apt-get install libgtk-3-dev libxcb-render0-dev libxcb-shape0-dev

# Build essentials
sudo apt-get install build-essential pkg-config libgpiod-dev

# Optional: for GPU debugging
sudo apt-get install vulkan-tools
```

## Cloning the Repository

```bash
# Clone the repository
git clone https://github.com/dpaschal/HamClock.git
cd HamClock

# Switch to Rust rewrite branch
git checkout laptop-optimized
```

## Building

### Development Build (Fast, with debugging)

```bash
# Build in debug mode (faster compilation)
cargo build

# Run with debugging info
./target/debug/hamclock
```

**Compilation time:** ~2-5 minutes (first build)
**Binary size:** ~40-50MB (debug symbols included)
**Performance:** Good for testing, not optimized

### Release Build (Slow, optimized)

```bash
# Build in release mode (optimized)
cargo build --release

# Run optimized binary
./target/release/hamclock
```

**Compilation time:** ~5-10 minutes (LTO enabled)
**Binary size:** ~3-4MB (stripped)
**Performance:** Maximum speed, recommended for production

### Development with Hot Reload (Optional)

```bash
# Install cargo-watch for automatic recompilation
cargo install cargo-watch

# Watch for changes and rebuild
cargo watch -x run
```

## Running

### From Repository

```bash
# Debug version
./target/debug/hamclock

# Release version
./target/release/hamclock
```

### From Anywhere (After Installation)

```bash
# Run installed binary
hamclock

# Run in background
hamclock &
```

## Installation

### User Installation (Recommended)

```bash
# Copy binary to user PATH
cargo install --path .

# Verify installation
which hamclock
hamclock
```

**Location:** `~/.cargo/bin/hamclock`

### System Installation

```bash
# Build release version
cargo build --release

# Copy to system location (requires sudo)
sudo cp target/release/hamclock /usr/local/bin/

# Verify
which hamclock
hamclock
```

## Testing

### Run All Tests

```bash
# Run all unit and integration tests
cargo test

# Run with output
cargo test -- --nocapture
```

### Run Specific Test

```bash
# Test data parsing
cargo test data::parsers

# Test rendering
cargo test render::gpu
```

### Performance Benchmarks

```bash
# Install benchmark tool
cargo install criterion

# Run benchmarks
cargo bench
```

## Development Tools

### Code Formatting

```bash
# Check formatting
cargo fmt --check

# Fix formatting
cargo fmt
```

### Linting (Clippy)

```bash
# Check for common mistakes
cargo clippy

# Fix warnings
cargo clippy --fix
```

### Documentation

```bash
# Build and open documentation
cargo doc --open
```

### Type Checking (Without Building)

```bash
# Quick type check
cargo check
```

## Troubleshooting

### "Could not find gtk3-devel"

**Fedora:**
```bash
sudo dnf search gtk3-devel
sudo dnf install gtk3-devel
```

**Ubuntu:**
```bash
sudo apt-get install libgtk-3-dev
```

### "error: failed to run custom build command"

**Likely cause:** Missing system dependencies

```bash
# Fedora
sudo dnf install libxcb-devel libxrender-devel pkg-config

# Ubuntu
sudo apt-get install libxcb-render0-dev pkg-config
```

### "rustc: command not found"

```bash
# Install Rust
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source $HOME/.cargo/env
```

### Compilation Hangs or Out of Memory

```bash
# Use fewer threads (reduce memory usage)
cargo build -j 2

# Or use release mode with slower LTO
cargo build --release -j 2
```

### GPU Errors at Runtime

```bash
# Check available GPUs
glxinfo | grep "OpenGL version"

# Try software rendering (slow)
WGPU_BACKEND=gl ./target/debug/hamclock

# Or force Vulkan
WGPU_BACKEND=vulkan ./target/debug/hamclock
```

## Performance Optimization

### Profiling CPU

```bash
# Install flamegraph
cargo install flamegraph

# Profile and generate flame graph
cargo flamegraph

# View result (opens in browser)
firefox flamegraph.svg
```

### Profiling Memory

```bash
# Using valgrind
valgrind --tool=massif ./target/release/hamclock

# View result
ms_print massif.out.*
```

### GPU Profiling

```bash
# Using apitrace (for OpenGL)
apitrace trace ./target/release/hamclock

# Analyze
qapitrace hamclock.trace
```

## Continuous Integration

### Local Pre-commit Checks

```bash
#!/bin/bash
# Save as .git/hooks/pre-commit

cargo fmt --check
cargo clippy -- -D warnings
cargo test
```

## Advanced Building

### Cross-Compilation

```bash
# Install cross
cargo install cross

# Compile for other architectures
cross build --target aarch64-unknown-linux-gnu
```

### Static Build

```bash
# Build static binary (all libs linked)
cargo build --release --target x86_64-unknown-linux-musl
```

## Environment Variables

### Rust Build

```bash
# Use all CPU cores
CARGO_BUILD_JOBS=4 cargo build --release

# Enable LLVM optimizations
RUSTFLAGS="-C target-cpu=native" cargo build --release
```

### Runtime

```bash
# GPU backend selection
WGPU_BACKEND=vulkan ./target/release/hamclock

# Logging (debug)
RUST_LOG=debug ./target/release/hamclock

# GPU debugging
WGPU_TRACE=<path> ./target/release/hamclock
```

## CI/CD Integration

### GitHub Actions (Future)

```yaml
name: Build

on: [push]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - uses: actions-rs/toolchain@v1
        with:
          toolchain: stable
      - run: cargo test --release
      - run: cargo build --release
```

## Build Artifacts

### What Gets Generated

```
target/
├── debug/
│   ├── hamclock           (40-50MB, debug symbols)
│   ├── deps/              (intermediate files)
│   └── ...
└── release/
    ├── hamclock           (3-4MB, optimized, stripped)
    ├── deps/
    └── ...
```

## Clean Build

```bash
# Remove all build artifacts
cargo clean

# Then rebuild from scratch
cargo build --release
```

**Use when:**
- Build seems corrupted
- Switching between release/debug
- Changing dependencies significantly

## Dependency Management

### Update Dependencies

```bash
# Check for updates
cargo outdated

# Update all dependencies
cargo update

# Update specific crate
cargo update -p wgpu
```

### Add New Dependency

```bash
# Add to Cargo.toml
cargo add serde

# Or manually edit Cargo.toml
[dependencies]
serde = "1.0"
```

## Documentation

### Generate and View

```bash
# Build documentation for all dependencies
cargo doc --document-private-items

# Open in browser
cargo doc --open
```

### Write Documentation

```rust
/// Calculate radio propagation MUF
///
/// # Arguments
/// * `frequency` - Frequency in MHz
///
/// # Returns
/// Maximum Usable Frequency
pub fn calculate_muf(frequency: f32) -> f32 { /* ... */ }
```

## Performance Tips

### Build Faster

```bash
# Use mold linker (faster than lld)
cargo install mold
RUSTFLAGS="-C link-arg=-fuse-ld=mold" cargo build

# Or incremental compilation
CARGO_INCREMENTAL=1 cargo build
```

### Runtime Faster

```bash
# Native CPU optimizations
RUSTFLAGS="-C target-cpu=native -C llvm-args=-mcpu=native" cargo build --release
```

## Getting Help

```bash
# Rust documentation
rustup doc

# Cargo documentation
cargo --help

# Build help
cargo build --help

# Check Clippy warnings
cargo clippy --help
```

---

**Next:** Read [Rust-Performance-Plan.md](Rust-Performance-Plan.md) for optimization details
**Status:** Build system ready, implementation can begin
