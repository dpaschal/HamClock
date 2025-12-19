//! GPU rendering engine using wgpu

pub mod gpu;
pub mod shapes;

pub use gpu::GpuContext;
pub use shapes::{Vertex, ShapeBuffer};
