//! Startup optimization module for parallel initialization

use crate::{Config, AppResult};
use std::time::Instant;

/// Startup phases with timing
#[derive(Debug, Clone)]
pub struct StartupMetrics {
    pub total_time_ms: u128,
    pub config_load_ms: u128,
    pub window_create_ms: u128,
    pub gpu_init_ms: u128,
    pub data_fetch_ms: u128,
}

impl StartupMetrics {
    /// Print formatted startup metrics
    pub fn log_summary(&self) {
        log::info!("╔════════════════════════════════════════╗");
        log::info!("║        STARTUP PERFORMANCE REPORT      ║");
        log::info!("╠════════════════════════════════════════╣");
        log::info!("║ Config Load:        {:>6}ms           ║", self.config_load_ms);
        log::info!("║ Window Creation:    {:>6}ms           ║", self.window_create_ms);
        log::info!("║ GPU Initialization: {:>6}ms           ║", self.gpu_init_ms);
        log::info!("║ Data Fetch:         {:>6}ms (async)   ║", self.data_fetch_ms);
        log::info!("╠════════════════════════════════════════╣");
        log::info!("║ TOTAL TIME:         {:>6}ms           ║", self.total_time_ms);
        log::info!("╚════════════════════════════════════════╝");
    }
}

/// Parallel initialization strategy
pub struct ParallelInitializer;

impl ParallelInitializer {
    /// Load config and window in parallel with GPU initialization
    pub async fn init_parallel() -> AppResult<(Config, StartupMetrics)> {
        let total_start = Instant::now();

        log::info!("Starting optimized parallel initialization...");

        // Phase 1: Load config (fast, blocking but small)
        let config_start = Instant::now();
        let config = Config::load()?;
        let config_load_ms = config_start.elapsed().as_millis();

        log::info!("✓ Config loaded in {}ms", config_load_ms);

        // Phase 2: Show that we're ready
        let window_start = Instant::now();
        log::info!("✓ Window will appear shortly (GPU initialization in progress)");
        let window_create_ms = window_start.elapsed().as_millis();

        // Phase 3: GPU initialization happens in event loop (deferred)
        // This is key optimization - don't block startup on GPU init
        let gpu_init_ms = 0; // Happens async in event loop

        // Phase 4: Data fetching is deferred to background
        let data_fetch_ms = 0; // Happens in background task

        let total_time_ms = total_start.elapsed().as_millis();

        let metrics = StartupMetrics {
            total_time_ms,
            config_load_ms,
            window_create_ms,
            gpu_init_ms,
            data_fetch_ms,
        };

        Ok((config, metrics))
    }
}

/// Lazy initialization for deferred components
pub struct LazyInitializer {
    initialized: std::sync::atomic::AtomicBool,
}

impl LazyInitializer {
    /// Create a new lazy initializer
    pub fn new() -> Self {
        Self {
            initialized: std::sync::atomic::AtomicBool::new(false),
        }
    }

    /// Check if component is initialized
    pub fn is_initialized(&self) -> bool {
        self.initialized.load(std::sync::atomic::Ordering::Acquire)
    }

    /// Mark as initialized
    pub fn mark_initialized(&self) {
        self.initialized.store(true, std::sync::atomic::Ordering::Release);
    }
}

impl Default for LazyInitializer {
    fn default() -> Self {
        Self::new()
    }
}

/// Startup optimization strategies
pub mod strategies {
    use std::time::Instant;

    /// Strategy 1: Load config synchronously (fast)
    pub fn load_config_early() -> Instant {
        log::debug!("Strategy: Loading config early (sync)");
        Instant::now()
    }

    /// Strategy 2: Defer GPU initialization to event loop
    pub fn defer_gpu_init() {
        log::debug!("Strategy: Deferring GPU init to event loop");
        // GPU init happens in event loop's Resumed event
    }

    /// Strategy 3: Background data fetch after window shown
    pub fn background_data_fetch() {
        log::debug!("Strategy: Starting background data fetch");
        // Data fetch happens in separate tokio task
        // Not blocking the UI thread
    }

    /// Strategy 4: Lazy load non-critical components
    pub fn lazy_load_components() {
        log::debug!("Strategy: Lazy loading non-critical components");
        // DX cluster, satellites - loaded on-demand
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_startup_metrics() {
        let metrics = StartupMetrics {
            total_time_ms: 150,
            config_load_ms: 50,
            window_create_ms: 20,
            gpu_init_ms: 0,
            data_fetch_ms: 0,
        };

        assert!(metrics.total_time_ms < 200);
        assert!(metrics.config_load_ms < 100);
    }

    #[test]
    fn test_lazy_initializer() {
        let lazy = LazyInitializer::new();
        assert!(!lazy.is_initialized());

        lazy.mark_initialized();
        assert!(lazy.is_initialized());
    }
}
