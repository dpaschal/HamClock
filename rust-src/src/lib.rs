//! HamClock - Radio clock with ham radio features
//!
//! A high-performance radio clock application with:
//! - GPU-accelerated rendering (wgpu)
//! - Async data fetching (tokio)
//! - Modern UI (gtk-rs)

pub mod data;
pub mod render;
pub mod ui;
pub mod config;
pub mod error;

// Re-export public API
pub use config::Config;
pub use error::{AppError, AppResult};
