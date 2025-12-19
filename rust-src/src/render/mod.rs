//! GPU rendering engine using wgpu

pub mod gpu;
pub mod shapes;
pub mod text;
pub mod layers;    // Phase 11
pub mod heatmap;   // Phase 12
pub mod aurora;    // Phase 12
pub mod grid;      // Phase 12

pub use gpu::GpuContext;
pub use shapes::{Vertex, ShapeBuffer};
pub use text::TextRenderer;
pub use layers::{LayerManager, LayerType, Layer, BlendMode};
pub use heatmap::MufColorPalette;
pub use aurora::AuroraOvalRenderer;
pub use grid::MaidenheadGridRenderer;
