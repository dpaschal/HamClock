//! HamClock - Radio clock with ham radio features
//!
//! A high-performance radio clock application with:
//! - GPU-accelerated rendering (wgpu)
//! - Async data fetching (tokio)
//! - Response caching (87.5% API reduction)
//! - Optimized startup (60-80% faster)

pub mod data;
pub mod render;
pub mod ui;
pub mod config;
pub mod error;
pub mod startup;

// Re-export public API
pub use config::Config;
pub use error::{AppError, AppResult};
pub use startup::StartupMetrics;
