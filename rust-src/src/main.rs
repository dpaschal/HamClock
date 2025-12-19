//! HamClock - Radio clock with ham radio features
//!
//! Main entry point - GPU-accelerated Rust rewrite with async data fetching

use hamclock::{Config, data::AppData, render::gpu::GpuContext};
use std::sync::Arc;
use std::time::{Duration, Instant};
use tokio::sync::Mutex;
use winit::event::{Event, WindowEvent};
use winit::event_loop::EventLoop;
use winit::window::Window;

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
            log::debug!("Background data fetch tick");
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
    log::info!("Phase 1: GPU rendering with wgpu (implementing event loop)");
    log::info!("Phase 2: Async data fetching with tokio (ready for implementation)");
    log::info!("Phase 3: Memory optimization (pending)");
    log::info!("Phase 4: Startup optimization (pending)");
    log::info!("Phase 5: CPU optimization (pending)");

    // Create event loop
    let event_loop = EventLoop::new()?;
    let target_fps = config.target_fps as u32;
    let target_frame_duration = Duration::from_secs_f64(1.0 / target_fps as f64);

    // Shared state for event loop
    let mut window: Option<Arc<Window>> = None;
    let mut gpu: Option<GpuContext> = None;
    let mut last_frame_time = Instant::now();

    event_loop.run(move |event, target| {
        match event {
            Event::Resumed => {
                // Create window and GPU context on resume
                if window.is_none() {
                    log::info!("Creating window...");
                    match Window::new(target) {
                        Ok(w) => {
                            let w = Arc::new(w);

                            // Initialize GPU context
                            match pollster::block_on(GpuContext::new(Arc::clone(&w))) {
                                Ok(g) => {
                                    let (width, height) = g.dimensions();
                                    log::info!("GPU context initialized: {}x{}", width, height);
                                    window = Some(w.clone());
                                    gpu = Some(g);
                                    last_frame_time = Instant::now();
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
                                }
                            }
                            WindowEvent::RedrawRequested => {
                                if let Some(g) = &gpu {
                                    // Frame timing
                                    let elapsed = last_frame_time.elapsed();
                                    if elapsed < target_frame_duration {
                                        std::thread::sleep(target_frame_duration - elapsed);
                                    }
                                    last_frame_time = Instant::now();

                                    // Render frame
                                    if let Err(e) = g.render_frame() {
                                        log::error!("Frame render failed: {:?}", e);
                                        target.exit();
                                    }

                                    // Request next frame
                                    w.request_redraw();
                                }
                            }
                            _ => {}
                        }
                    }
                }
            }
            Event::AboutToWait => {
                if let Some(w) = &window {
                    w.request_redraw();
                }
            }
            _ => {}
        }
    })?;

    Ok(())
}
