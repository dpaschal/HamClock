# Rust Rewrite - Performance Optimization Plan

**Branch:** `laptop-optimized`
**Target Improvement:** 2-3x faster than C++ version
**Measurement:** Continuous profiling & benchmarking

---

## Performance Goals

### Targets

| Metric | C++ Baseline | Rust Target | Improvement |
|--------|------------|------------|-------------|
| **Startup Time** | 3-5 seconds | 1-2 seconds | 60-80% |
| **FPS** | 30-60 | 60-120+ | 2-4x |
| **Data Fetch Latency** | 5-10s (sequential) | 1-2s (parallel) | 70-80% |
| **Memory Usage** | ~150MB | ~80MB | 45% |
| **CPU Usage** | 15-20% | 5-8% | 50-65% |

## Phase 1: GPU Rendering (Est. Weeks 2-3)

### Goal
Replace CPU-based X11 drawing with GPU acceleration.

### Expected Gains
- **Startup:** 30-40% faster (reduced initial draw calls)
- **FPS:** 2-3x increase (GPU handles rendering)
- **CPU:** 60% reduction (offloaded to GPU)

### Implementation

```rust
// Before (CPU-bound X11)
for point in map_points {
    x11_draw_pixel(point.x, point.y);  // 1000+ syscalls per frame
}

// After (GPU-optimized)
let mesh = vertices.into_gpu_buffer();
gpu.render_mesh(mesh);  // 1 draw call
```

### Benchmarking

```bash
# Profile frame rendering
cargo flamegraph --bin hamclock -- --profile-frames 100

# Check GPU utilization
glxinfo | grep "OpenGL version"
nvidia-smi  # if NVIDIA GPU
```

### Success Criteria
- [ ] FPS reaches 60+ on 1920x1200
- [ ] CPU usage drops below 10%
- [ ] No GPU errors in validation layer
- [ ] Frame time < 16.67ms (60 FPS)

## Phase 2: Async Data Fetching (Est. Weeks 1-2)

### Goal
Replace blocking HTTP requests with non-blocking async/await.

### Expected Gains
- **Data Fetch:** 50-80% faster (concurrent requests)
- **UI Responsiveness:** 100% improved (non-blocking)
- **Latency:** 70% reduction (parallel instead of serial)

### Implementation

```rust
// Before (Blocking, sequential)
let space_weather = fetch_space_weather();  // Wait...
let forecast = fetch_forecast();             // Wait...
let dx = fetch_dx_cluster();                 // Wait...
// Total: 5-10 seconds

// After (Non-blocking, concurrent)
let (space_weather, forecast, dx) = tokio::join!(
    fetch_space_weather(),
    fetch_forecast(),
    fetch_dx_cluster(),
);
// Total: 1-2 seconds (parallel)
```

### Benchmarking

```bash
# Measure data fetch time
time ./target/release/hamclock --benchmark-fetch

# Profile async tasks
cargo flamegraph --features async-profile
```

### Success Criteria
- [ ] Data fetches complete in < 2 seconds
- [ ] Multiple concurrent requests working
- [ ] No UI blocking during fetch
- [ ] Memory stable during concurrent operations

## Phase 3: Memory Optimization (Est. Week 4)

### Goal
Reduce runtime memory footprint from 150MB to 80MB.

### Current Allocation

```
C++ Version:
├─ Code/Binary: ~10MB
├─ Static Data: ~20MB
├─ Runtime Cache: ~70MB
├─ GPU Upload: ~30MB
└─ Misc: ~20MB
───────────────
Total: ~150MB
```

### Optimization Strategies

### Strategy 1: Reduce Cache Size
```rust
// Before: Store 30 days of weather history
let cache = Vec::with_capacity(30);

// After: Store only 7 days (still useful)
let cache = Vec::with_capacity(7);
// Saves ~20MB
```

### Strategy 2: Lazy Loading
```rust
// Before: Load all maps on startup
let maps = load_all_maps();  // 30MB

// After: Load on demand
let maps = Arc::new(LazyMap::new());  // 5MB + lazy load
// Saves ~25MB, adds <50ms to first access
```

### Strategy 3: Compression
```rust
// Before: Store uncompressed data
let data = serde_json::to_string(&weather);

// After: Compress in memory (zstd)
let compressed = zstd::encode_all(&data, 3)?;
// Saves ~15MB
```

### Expected Gains

| Strategy | Savings | Effort |
|----------|---------|--------|
| Reduce cache | 20MB | Low |
| Lazy loading | 25MB | Medium |
| Compression | 15MB | Medium |
| **Total** | **60MB** | **Medium** |

### Benchmarking

```bash
# Memory profiling
valgrind --tool=massif ./target/release/hamclock
ms_print massif.out.*

# Track allocations
RUST_LOG=debug ./target/release/hamclock 2>&1 | grep "alloc"
```

### Success Criteria
- [ ] Memory usage < 100MB
- [ ] Cache size configurable
- [ ] No performance regression
- [ ] Compression saves > 50MB total

## Phase 4: Startup Optimization (Est. Week 5)

### Goal
Reduce startup from 3-5 seconds to 1-2 seconds.

### Bottlenecks

```
Current startup timeline:
0.0s - Start process
0.3s - Initialize GUI (gtk-rs)
0.8s - Initialize GPU (wgpu)
1.2s - Load configuration
3.5s - Fetch initial data (blocking!)
5.0s - Render first frame
```

### Optimizations

### 1. Lazy Data Fetching
```rust
// Before: Block on startup
fn startup() {
    fetch_all_data();  // 3-5s wait
    render_window();
}

// After: Fetch in background
fn startup() {
    show_placeholder();
    tokio::spawn(async { fetch_all_data(); });
}
```
**Gain:** 3-4 seconds

### 2. Parallel Initialization
```rust
// Before: Sequential
init_gpu();
init_ui();
init_config();

// After: Parallel
tokio::join!(
    init_gpu(),
    init_ui(),
    init_config(),
);
```
**Gain:** 0.5-1 second

### 3. Binary Size Optimization
```bash
# Strip symbols
strip target/release/hamclock

# Use LTO
[profile.release]
lto = true
```
**Gain:** Faster loading from disk

### Expected Timeline After Optimization
```
0.0s - Start process
0.1s - Initialize GUI
0.2s - Initialize GPU (parallel)
0.3s - Initialize config (parallel)
0.5s - Show window (with placeholder)
0.8s - Start data fetch (background)
1.0s - Fetch completes, show real data
1.2s - Ready for user interaction
```

### Benchmarking

```bash
# Time startup
time ./target/release/hamclock --benchmark-startup

# Profile with flamegraph
cargo flamegraph --bin hamclock -- --benchmark-startup
```

### Success Criteria
- [ ] Window appears in < 500ms
- [ ] UI responsive in < 1s
- [ ] Data loaded in background
- [ ] Total startup < 2 seconds

## Phase 5: CPU Optimization (Est. Week 5)

### Goal
Reduce CPU usage from 15-20% to 5-8%.

### Profiling Strategy

```bash
# Identify CPU hotspots
cargo flamegraph --bin hamclock -- --duration 30

# Look for:
# - Excessive allocations
# - Inefficient loops
# - Blocking operations
# - Synchronous I/O
```

### Optimization Areas

### 1. Render Loop Optimization
```rust
// Before: Wasteful
loop {
    render_frame();        // Always
    sleep(1);              // Burn CPU waiting
}

// After: Event-driven
loop {
    wait_for_event();      // Sleep until needed
    if should_render() {
        render_frame();
    }
}
```

### 2. Data Update Cadence
```rust
// Before: Update every frame
fn on_frame() {
    parse_data();       // Expensive
    invalidate_cache();
    render();
}

// After: Update on data change only
async fn update_data() {
    loop {
        fetch_and_parse();
        update_cache();
        request_redraw();
        sleep(5).await;  // 5-second intervals
    }
}
```

### 3. Lock Contention
```rust
// Before: Locks on every access
let data = Arc::Mutex::new(data);
for _ in 0..1000 {
    let d = data.lock().unwrap();  // Expensive
}

// After: RwLock for read-heavy
let data = Arc::RwLock::new(data);
for _ in 0..1000 {
    let d = data.read().unwrap();  // Cheap
}
```

### Expected CPU Timeline

```
Before Optimization:
├─ Render loop: 8-10%
├─ Data parsing: 3-4%
├─ Network I/O: 2%
└─ GUI: 2-4%
───────────────
Total: 15-20%

After Optimization:
├─ Render loop: 2-3%
├─ Data parsing: 1%
├─ Network I/O: 1%
└─ GUI: 1-2%
───────────────
Total: 5-8%
```

### Benchmarking

```bash
# CPU profiling
perf record -g ./target/release/hamclock
perf report

# Or use flamegraph
cargo flamegraph
```

### Success Criteria
- [ ] CPU usage < 10% idle (just window displayed)
- [ ] CPU usage < 8% with data updating
- [ ] No CPU spikes
- [ ] Power consumption reduced

## Continuous Benchmarking

### Setup Benchmarks

```rust
// src/benches/rendering.rs
use criterion::{black_box, criterion_group, criterion_main, Criterion};

fn render_benchmark(c: &mut Criterion) {
    c.bench_function("render_frame", |b| {
        b.iter(|| render_frame(black_box(&data)));
    });
}

criterion_group!(benches, render_benchmark);
criterion_main!(benches);
```

### Run Benchmarks

```bash
# Run all benchmarks
cargo bench

# Compare against baseline
cargo bench -- --save-baseline my_baseline
cargo bench -- --baseline my_baseline
```

## Performance Tracking

### Weekly Metrics

Track after each optimization phase:

```
Week 1: Async Data Fetching
├─ Data fetch time: 2.1s (target: <2s)
├─ Memory: 145MB (target: <100MB)
└─ CPU: 18% (target: <8%)

Week 2: GPU Rendering
├─ FPS: 72 (target: >60)
├─ CPU: 8% (target: <8%) ✅
└─ Memory: 92MB (target: <100MB)

Week 3: Memory Optimization
├─ Memory: 78MB ✅ (target: <100MB)
├─ Startup: 1.8s ✅ (target: <2s)
└─ CPU: 6% ✅ (target: <8%)
```

### Regression Testing

```bash
# Automated performance tests
#[test]
fn perf_startup_time() {
    let start = Instant::now();
    startup_app();
    let elapsed = start.elapsed();
    assert!(elapsed < Duration::from_secs(2), "Startup too slow: {:?}", elapsed);
}
```

## Tools & Commands

### Essential Tools

```bash
# Install performance tools
cargo install flamegraph
cargo install criterion
cargo install cargo-flamegraph

# Profiling
cargo flamegraph                    # CPU profiling
valgrind --tool=massif            # Memory
cargo bench                         # Benchmarks
perf stat ./target/release/hamclock # Statistics
```

### Comparison Matrix

```bash
# Before vs After
echo "=== BEFORE ==="
time ./target_old/hamclock --benchmark

echo "=== AFTER ==="
time ./target/release/hamclock --benchmark
```

## Success Criteria (Overall)

- [ ] 60+ FPS on 1920x1200 display
- [ ] < 2 second startup
- [ ] < 100MB memory usage
- [ ] < 8% CPU at idle
- [ ] 50-80% faster data fetching
- [ ] No memory leaks
- [ ] No GPU errors
- [ ] All tests passing

---

**Status:** Performance plan ready
**Next:** Begin implementation with Phase 1 (GPU Rendering) and Phase 2 (Async Data) in parallel
**Review:** Weekly performance benchmarks during development
