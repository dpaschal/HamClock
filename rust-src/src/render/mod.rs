//! GPU rendering engine using wgpu

pub mod gpu;
pub mod shapes;
pub mod text;
pub mod layers; // Phase 11

pub use gpu::GpuContext;
pub use shapes::{Vertex, ShapeBuffer};
pub use text::TextRenderer;
pub use layers::{LayerManager, LayerType, Layer, BlendMode}; // Phase 11
