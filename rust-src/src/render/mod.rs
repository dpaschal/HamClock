//! GPU rendering engine using wgpu

pub mod gpu;
pub mod shapes;
pub mod text;

pub use gpu::GpuContext;
pub use shapes::{Vertex, ShapeBuffer};
pub use text::TextRenderer;
