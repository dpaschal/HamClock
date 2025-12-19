//! HamClock - Radio clock with ham radio features
//!
//! Main entry point - GPU-accelerated Rust rewrite with async data fetching

use hamclock::{Config, data::AppData};
use std::sync::Arc;
use tokio::sync::Mutex;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Initialize logging
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
        .init();

    log::info!("HamClock Rust Rewrite - Starting up");

    // Load configuration
    let config = Config::load()?;
    log::info!(
        "Configuration loaded: {}x{} @ {} FPS",
        config.resolution.0, config.resolution.1, config.target_fps
    );

    // Create shared data store
    let app_data = Arc::new(Mutex::new(AppData::new()));

    // Spawn background task for periodic data fetching
    let data_clone = Arc::clone(&app_data);
    let update_interval = config.data_update_interval;
    tokio::spawn(async move {
        loop {
            log::info!("Fetching data...");
            {
                let mut data = data_clone.lock().await;
                data.update_timestamp();
            }
            tokio::time::sleep(
                tokio::time::Duration::from_secs(update_interval)
            ).await;
        }
    });

    log::info!("Application initialized");
    log::info!("Phase 1: GPU rendering with wgpu (ready for implementation)");
    log::info!("Phase 2: Async data fetching with tokio (ready for implementation)");
    log::info!("Phase 3: Memory optimization (pending)");
    log::info!("Phase 4: Startup optimization (pending)");
    log::info!("Phase 5: CPU optimization (pending)");

    // Keep application running
    loop {
        tokio::time::sleep(tokio::time::Duration::from_secs(60)).await;
    }
}
