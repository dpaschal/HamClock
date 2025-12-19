//! Configuration for HamClock

use serde::{Deserialize, Serialize};
use crate::AppResult;

/// HamClock application configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    /// Display resolution (width x height)
    pub resolution: (u32, u32),

    /// Target frames per second
    pub target_fps: u32,

    /// Data update interval in seconds
    pub data_update_interval: u64,

    /// Default location (latitude, longitude)
    pub location: (f32, f32),

    /// Enable GPIO support
    pub gpio_enabled: bool,

    /// GPU backend (vulkan, gl, metal, dx12)
    pub gpu_backend: String,

    /// Enable logging
    pub logging: bool,
}

impl Default for Config {
    fn default() -> Self {
        Self {
            resolution: (1920, 1200),
            target_fps: 60,
            data_update_interval: 5,
            location: (0.0, 0.0),
            gpio_enabled: false,
            gpu_backend: "vulkan".to_string(),
            logging: false,
        }
    }
}

impl Config {
    /// Load configuration from file or use defaults
    pub fn load() -> AppResult<Self> {
        // For now, return defaults
        // In future: load from config file
        Ok(Self::default())
    }

    /// Save configuration to file
    pub fn save(&self) -> AppResult<()> {
        // TODO: Implement file persistence
        Ok(())
    }
}
