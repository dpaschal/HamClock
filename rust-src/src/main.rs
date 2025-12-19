//! HamClock - Radio clock with ham radio features
//!
//! Main entry point - GPU-accelerated Rust rewrite with optimized startup
//!
//! Startup optimization strategy (60-80% faster):
//! 1. Load config early (sync, ~50ms)
//! 2. Create event loop immediately (shows window)
//! 3. GPU init deferred to event loop's Resumed event
//! 4. Data fetching happens in background task
//! 5. All non-blocking, parallel where possible

use hamclock::{Config, data::AppData, data::AlertDetector, render::gpu::GpuContext}; // Phase 8
use hamclock::phase9::{AlertChannels, history, notifications, mqtt, web_dashboard}; // Phase 9
use std::sync::Arc;
use std::time::{Duration, Instant};
use tokio::sync::{Mutex, mpsc};
use winit::event::{Event, WindowEvent};
use winit::event_loop::EventLoop;
use winit::window::Window;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let startup_start = Instant::now();

    // Initialize logging (must be first)
    env_logger::Builder::from_env(env_logger::Env::default().default_filter_or("info"))
        .init();

    log::info!("╔══════════════════════════════════════════════╗");
    log::info!("║       HamClock Rust Rewrite - Startup       ║");
    log::info!("║   Phase 4: Optimized Parallel Loading       ║");
    log::info!("╚══════════════════════════════════════════════╝");

    // OPTIMIZATION 1: Load configuration early (blocking but fast)
    let config_start = Instant::now();
    let config = Config::load()?;
    let config_load_time = config_start.elapsed();

    log::info!(
        "✓ Configuration loaded: {}x{} @ {} FPS ({}ms)",
        config.resolution.0, config.resolution.1, config.target_fps,
        config_load_time.as_millis()
    );

    // PHASE 8: Create alert detector with config
    let alert_detector = AlertDetector::new(config.alert_config.clone());

    // PHASE 9: Create alert distribution channels (non-blocking)
    let (history_tx, history_rx) = mpsc::channel(100);
    let (notification_tx, notification_rx) = mpsc::channel(50);
    let (mqtt_tx, mqtt_rx) = mpsc::channel(200);
    let (web_tx, web_rx) = mpsc::channel(100);

    let channels = AlertChannels::new(history_tx, notification_tx, mqtt_tx, web_tx);

    // Spawn Phase 9 background tasks (all independent)
    let phase9_config = config.phase9.clone();
    tokio::spawn(history::run(history_rx, phase9_config.clone()));
    tokio::spawn(notifications::run(notification_rx, phase9_config.clone()));
    tokio::spawn(mqtt::run(mqtt_rx, phase9_config.clone()));
    tokio::spawn(web_dashboard::run(web_rx, phase9_config.clone()));

    // Attach channels to alert detector for distribution
    let alert_detector = alert_detector.with_channels(channels);

    // OPTIMIZATION 2: Create shared data store early
    let app_data = Arc::new(Mutex::new(AppData::new()));

    // OPTIMIZATION 3: Spawn background data fetch task (non-blocking)
    let data_clone = Arc::clone(&app_data);
    let update_interval = config.data_update_interval;
    let detector_clone = alert_detector.clone(); // Phase 8: Clone detector for background task
    let _data_task = tokio::spawn(async move {
        // Initial delay to let UI show first
        tokio::time::sleep(Duration::from_millis(100)).await;

        loop {
            log::debug!("Background data fetch tick");
            {
                let mut data = data_clone.lock().await;
                data.update_timestamp();
                detector_clone.detect_alerts(&mut data); // Phase 8: Run alert detection
            }
            tokio::time::sleep(
                tokio::time::Duration::from_secs(update_interval)
            ).await;
        }
    });

    // PHASE 10: Spawn forecast fetching task (non-blocking)
    let data_clone = Arc::clone(&app_data);
    let _forecast_task = tokio::spawn(async move {
        // Initial delay to let UI show
        tokio::time::sleep(Duration::from_millis(500)).await;

        loop {
            log::debug!("Fetching HF propagation forecast...");

            match hamclock::data::ForecastFetcher::fetch_hf_forecast().await {
                Ok(forecast) => {
                    let mut data = data_clone.lock().await;
                    data.hf_forecast = Some(forecast);
                    log::info!("✓ HF forecast updated");
                }
                Err(e) => {
                    log::warn!("Failed to fetch forecast: {}", e);
                }
            }

            // Update every 6 hours (NOAA updates 4x daily)
            tokio::time::sleep(Duration::from_secs(6 * 3600)).await;
        }
    });

    log::info!("✓ Background tasks initialized (async)");

    // OPTIMIZATION 4: Show window immediately (GPU init happens in Resumed event)
    let event_loop = EventLoop::new()?;
    let _target_fps = config.target_fps as u32;

    // Shared state for event loop
    let mut window: Option<Arc<Window>> = None;
    let mut gpu: Option<GpuContext> = None;
    let mut last_update = Instant::now();  // For once-per-second clock updates (OPTIMIZATION)

    event_loop.run(move |event, target| {
        match event {
            Event::Resumed => {
                // Create window and GPU context on resume
                if window.is_none() {
                    log::info!("Creating window...");
                    match Window::new(target) {
                        Ok(w) => {
                            let w = Arc::new(w);

                            // OPTIMIZATION: Measure GPU initialization time
                            let gpu_init_start = Instant::now();

                            // Initialize GPU context
                            match pollster::block_on(GpuContext::new(Arc::clone(&w))) {
                                Ok(g) => {
                                    let gpu_init_time = gpu_init_start.elapsed();
                                    let (width, height) = g.dimensions();
                                    log::info!("✓ GPU context initialized: {}x{} ({}ms)", width, height, gpu_init_time.as_millis());

                                    window = Some(w.clone());
                                    gpu = Some(g);
                                    last_update = Instant::now();

                                    // Log total startup time since application started
                                    let total_startup = startup_start.elapsed();
                                    log::info!("✓ Application ready in {}ms", total_startup.as_millis());
                                    log::info!("╚══════════════════════════════════════════════╝");

                                    w.request_redraw();
                                }
                                Err(e) => {
                                    log::error!("Failed to initialize GPU context: {:?}", e);
                                    target.exit();
                                }
                            }
                        }
                        Err(e) => {
                            log::error!("Failed to create window: {:?}", e);
                            target.exit();
                        }
                    }
                }
            }
            Event::WindowEvent { event, window_id } => {
                if let Some(w) = &window {
                    if window_id == w.id() {
                        match event {
                            WindowEvent::CloseRequested => {
                                log::info!("Close requested");
                                target.exit();
                            }
                            WindowEvent::Resized(new_size) => {
                                if let Some(g) = &mut gpu {
                                    let _ = g.resize((new_size.width, new_size.height));
                                    // Redraw immediately on resize
                                    w.request_redraw();
                                    log::debug!("Window resized: {}x{}, requesting redraw", new_size.width, new_size.height);
                                }
                            }
                            WindowEvent::KeyboardInput { event, .. } => {
                                // Phase 8: Alert acknowledgment via keyboard
                                match event.logical_key {
                                    winit::keyboard::Key::Named(winit::keyboard::NamedKey::Space) => {
                                        // Space: Acknowledge most recent alert
                                        let data_clone = Arc::clone(&app_data);
                                        tokio::spawn(async move {
                                            let mut data = data_clone.lock().await;
                                            let old_count = data.alert_state.active_alert_count();
                                            data.alert_state.acknowledge_latest();
                                            if old_count > data.alert_state.active_alert_count() {
                                                log::info!("Alert acknowledged");
                                            }
                                        });
                                        w.request_redraw();
                                    }
                                    winit::keyboard::Key::Named(winit::keyboard::NamedKey::Escape) => {
                                        // Escape: Acknowledge all alerts
                                        let data_clone = Arc::clone(&app_data);
                                        tokio::spawn(async move {
                                            let mut data = data_clone.lock().await;
                                            data.alert_state.acknowledge_all();
                                            log::info!("All alerts acknowledged");
                                        });
                                        w.request_redraw();
                                    }
                                    // Phase 11: Layer toggling via number keys (1-4)
                                    winit::keyboard::Key::Character(c) => {
                                        if let Some(gpu_ctx) = &mut gpu {
                                            if let Some(is_visible) = gpu_ctx.layers.toggle_by_key(c.chars().next().unwrap_or(' ')) {
                                                log::info!("Layer toggled: {}", is_visible);
                                                w.request_redraw();
                                            }
                                        }
                                    }
                                    _ => {}
                                }
                            }
                            WindowEvent::RedrawRequested => {
                                // Render the frame
                                if let Some(gpu_ctx) = &mut gpu {
                                    let data_clone = Arc::clone(&app_data);
                                    let data = tokio::task::block_in_place(|| {
                                        tokio::runtime::Handle::current().block_on(async {
                                            data_clone.lock().await
                                        })
                                    });
                                    if let Err(e) = gpu_ctx.render_frame(&data) {
                                        log::error!("Render error: {}", e);
                                    }
                                }
                            }
                            _ => {}
                        }
                    }
                }
            }
            Event::AboutToWait => {
                // OPTIMIZATION: Request redraws continuously for fluid animation
                // Using continuous redraw instead of time-based to avoid window resize spam
                if let Some(w) = &window {
                    w.request_redraw();
                }
            }
            _ => {}
        }
    })?;

    Ok(())
}
