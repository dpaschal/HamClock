# Rust Rewrite - Technical Architecture

**Branch:** `laptop-optimized`
**Language:** Rust 1.70+
**Status:** Architecture planned, ready for implementation

---

## System Architecture

### High-Level Component Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    HamClock Application                      │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  ┌──────────────────┐         ┌─────────────────────┐      │
│  │   UI Layer       │ ◄────────┤  Event Handler      │      │
│  │  (gtk-rs/egui)   │          │  (Mouse, Keyboard)  │      │
│  └──────────┬───────┘          └─────────────────────┘      │
│             │                                                │
│             ▼                                                │
│  ┌─────────────────────────────────────────────────┐       │
│  │        Rendering Engine (wgpu)                  │       │
│  │  ┌──────────────────┐  ┌──────────────────┐   │       │
│  │  │  GPU Rendering   │  │  2D Primitives   │   │       │
│  │  │  (OpenGL/Vulkan) │  │  (Shapes, Text)  │   │       │
│  │  └──────────────────┘  └──────────────────┘   │       │
│  └────────────┬────────────────────────────────────┘       │
│               │                                             │
│               ▼                                             │
│  ┌──────────────────────────────────────────────┐         │
│  │      Data & State Management                │         │
│  │  ┌──────────────┐  ┌──────────────────┐   │         │
│  │  │  Cache       │  │  Live Data       │   │         │
│  │  │  (In-Memory) │  │  (Current State) │   │         │
│  │  └──────────────┘  └──────────────────┘   │         │
│  └────────────┬────────────────────────────────┘         │
│               │                                            │
│               ▼                                            │
│  ┌──────────────────────────────────────────────┐        │
│  │  Async Data Fetching (tokio runtime)        │        │
│  │  ┌──────────────────────────────────────┐  │        │
│  │  │  Concurrent HTTP Requests            │  │        │
│  │  │  - Space Weather                     │  │        │
│  │  │  - Forecasts                         │  │        │
│  │  │  - DX Cluster                        │  │        │
│  │  │  - Satellite Data                    │  │        │
│  │  └──────────────────────────────────────┘  │        │
│  └──────────────────────────────────────────────┘        │
│                                                            │
│  ┌──────────────────────────────────────────────┐        │
│  │  Network & Parsing                          │        │
│  │  ┌──────────────────────────────────────┐  │        │
│  │  │  Async HTTP (reqwest)                │  │        │
│  │  │  JSON Parsing (serde)                │  │        │
│  │  │  Data Models (structs)               │  │        │
│  │  │  Error Handling (Result<T, E>)       │  │        │
│  │  └──────────────────────────────────────┘  │        │
│  └──────────────────────────────────────────────┘        │
│                                                            │
└─────────────────────────────────────────────────────────────┘
```

## Module Organization

### Core Modules

#### `main.rs` - Application Entry Point
```rust
fn main() {
    // Initialize Rust runtime
    // Load configuration
    // Create application window
    // Start event loop
    // Run tokio executor alongside UI
}
```

#### `lib.rs` - Library Interface
```rust
pub mod data;      // Data fetching and models
pub mod render;    // GPU rendering
pub mod ui;        // User interface
pub mod config;    // Configuration
pub mod error;     // Error types
```

### Data Module (`src/data/`)

#### `fetcher.rs` - Async Data Fetching
```rust
pub struct DataFetcher {
    client: reqwest::Client,
}

impl DataFetcher {
    // Fetch space weather (concurrent)
    async fn fetch_space_weather() -> Result<SpaceWeather>

    // Fetch forecasts (concurrent)
    async fn fetch_forecasts() -> Result<Forecast>

    // Fetch DX Cluster (concurrent)
    async fn fetch_dx_cluster() -> Result<Vec<DxSpot>>
}
```

**Performance:** All requests run in parallel using tokio tasks

#### `parsers.rs` - Data Parsing
```rust
pub struct JsonParser;

impl JsonParser {
    fn parse_space_weather(json: &str) -> Result<SpaceWeather>
    fn parse_forecast(json: &str) -> Result<Forecast>
    fn parse_dx_cluster(json: &str) -> Result<Vec<DxSpot>>
}
```

**Performance:** Uses serde for zero-copy parsing

#### `models.rs` - Data Structures
```rust
#[derive(Serialize, Deserialize)]
pub struct SpaceWeather {
    pub kp: f32,
    pub a: i32,
    pub ap: i32,
    pub flux: i32,
}

#[derive(Serialize, Deserialize)]
pub struct Forecast {
    pub date: Date,
    pub high_temp: i32,
    pub conditions: String,
}
```

### Rendering Module (`src/render/`)

#### `gpu.rs` - GPU Initialization (wgpu)
```rust
pub struct GpuContext {
    device: Device,
    queue: Queue,
    surface: Surface,
    config: SurfaceConfiguration,
}

impl GpuContext {
    fn new(window: &Window) -> Self { /* ... */ }
    fn render_frame(&self, data: &AppData) { /* ... */ }
}
```

**Performance:** Direct GPU access via wgpu (OpenGL 4.6+ or Vulkan)

#### `shapes.rs` - 2D Primitives
```rust
pub fn draw_rect(x: f32, y: f32, w: f32, h: f32, color: Color)
pub fn draw_circle(x: f32, y: f32, r: f32, color: Color)
pub fn draw_text(x: f32, y: f32, text: &str, size: f32)
pub fn draw_line(x1: f32, y1: f32, x2: f32, y2: f32, color: Color)
```

**Performance:** Uses instancing for batch rendering

#### `ui.rs` - UI Components
```rust
pub struct Button {
    rect: Rect,
    label: String,
    on_click: Box<dyn Fn()>,
}

pub struct Map {
    bounds: Rect,
    zoom: f32,
    data: MapData,
}
```

### UI Module (`src/ui/`)

#### `window.rs` - Main Window
```rust
pub struct MainWindow {
    current_view: View,
    data: Arc<Mutex<AppData>>,
    event_sender: mpsc::Sender<Event>,
}
```

#### `tabs.rs` - View Management
```rust
pub enum View {
    Clock,
    PropagationMap,
    Forecast,
    Moon,
    Satellite,
    DxCluster,
}

impl View {
    fn render(&self, gpu: &GpuContext, data: &AppData)
}
```

#### `controls.rs` - Interactive Controls
```rust
pub struct TimeDisplay { /* ... */ }
pub struct MapControl { /* ... */ }
pub struct MenuButton { /* ... */ }
```

## Data Flow

### Startup Sequence

```
1. Load Config
   ↓
2. Initialize GPU (wgpu)
   ↓
3. Create UI Window (gtk-rs/egui)
   ↓
4. Start tokio Runtime
   ↓
5. Spawn Async Tasks (data fetching)
   ↓
6. Enter Event Loop
   ├─ Poll async tasks (data updates)
   ├─ Handle UI events (mouse, keyboard)
   ├─ Render frame (GPU)
   └─ Update display
```

### Data Update Cycle

```
Every 5 seconds:
┌─────────────────────────────────────┐
│ Fetch Data (Async, Non-blocking)    │
├─────────────────────────────────────┤
│ ┌─────────────────────────────────┐ │
│ │ Parallel Requests:              │ │
│ │ - Space Weather (concurrent)    │ │
│ │ - Forecast (concurrent)         │ │
│ │ - DX Cluster (concurrent)       │ │
│ │ - Satellite Data (concurrent)   │ │
│ └─────────────────────────────────┘ │
├─────────────────────────────────────┤
│ Parse Results (serde)               │
├─────────────────────────────────────┤
│ Update Cache (Arc<Mutex<>>)         │
├─────────────────────────────────────┤
│ Wake UI (render next frame)         │
└─────────────────────────────────────┘
```

## Concurrency Model

### Tokio Runtime

```rust
#[tokio::main]
async fn main() {
    // Multiple async tasks running concurrently

    tokio::spawn(fetch_space_weather());
    tokio::spawn(fetch_forecasts());
    tokio::spawn(fetch_dx_cluster());
    tokio::spawn(fetch_satellite_data());

    // UI runs on main thread
    // Tasks complete without blocking UI
}
```

### Thread Safety

```rust
// Shared data between threads
pub struct AppData {
    pub space_weather: Arc<Mutex<SpaceWeather>>,
    pub forecasts: Arc<Mutex<Vec<Forecast>>>,
    pub dx_cluster: Arc<Mutex<Vec<DxSpot>>>,
}
```

**Benefits:**
- Compile-time guarantees of thread safety
- No data races possible
- Rust compiler enforces correct synchronization

## Performance Characteristics

### GPU Rendering

**wgpu Pipeline:**
```
Data → Vertex Shader → Rasterization → Fragment Shader → Display
```

**Optimizations:**
- Batching (multiple primitives per draw call)
- Instancing (repeated geometry)
- Off-screen rendering (compositing)
- GPU memory management

### Memory Management

**Rust Advantages:**
- No garbage collector pauses
- Stack allocation for small objects
- Automatic cleanup (RAII pattern)
- No memory leaks (compiler enforced)

**Estimated Memory Usage:**
```
Binary:         ~8-10MB
Runtime (Code): ~20-30MB
Data Cache:     ~40-50MB
GPU Memory:     ~20-40MB
───────────────────────
Total:          ~80-100MB
```

### Async I/O

**Benefits:**
- Concurrent requests without threads
- Minimal memory per task (vs threads)
- Non-blocking UI
- Automatic task scheduling

**Example:**
```rust
// Three requests in parallel
let weather = fetch_space_weather();
let forecast = fetch_forecasts();
let dx_cluster = fetch_dx_cluster();

// Wait for all to complete
let (w, f, d) = tokio::join!(weather, forecast, dx_cluster);
```

## Error Handling

### Result Type Pattern

```rust
type AppResult<T> = Result<T, AppError>;

pub enum AppError {
    NetworkError(String),
    ParseError(String),
    RenderError(String),
    ConfigError(String),
}
```

**Benefits:**
- Explicit error handling
- Compile-time error checking
- No panics in production code

## Testing Strategy

### Unit Tests

```rust
#[cfg(test)]
mod tests {
    #[test]
    fn test_parser() {
        let json = r#"{"kp": 5}"#;
        let result = parse_space_weather(json);
        assert!(result.is_ok());
    }
}
```

### Integration Tests

```bash
cargo test --all
```

### Performance Tests

```bash
cargo bench
```

## Dependencies Overview

| Dependency | Purpose | Why |
|-----------|---------|-----|
| tokio | Async runtime | Best async ecosystem |
| reqwest | HTTP client | Easy async requests |
| serde | Serialization | Fast zero-copy parsing |
| wgpu | GPU graphics | Cross-platform GPU |
| gtk-rs | UI framework | Native desktop UI |
| chrono | DateTime | Timezone support |
| nalgebra | Math | Linear algebra |
| log | Logging | Structured logging |

## Build Artifacts

### Release Build

```bash
cargo build --release
./target/release/hamclock
```

**Optimizations:**
- Link-Time Optimization (LTO)
- Code stripping
- Release profile settings (opt-level=3)

### Debug Build

```bash
cargo build
./target/debug/hamclock
```

**For Development:**
- Fast compilation
- Debugging symbols
- Runtime assertions

---

**Status:** Architecture complete, ready for implementation
**Next:** See [Rust-Building-Guide.md](Rust-Building-Guide.md)
