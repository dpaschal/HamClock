//! GPU context initialization and management with wgpu

use wgpu::*;
use winit::window::Window;
use crate::AppResult;
use crate::error::AppError;
use crate::render::TextRenderer;
use crate::data::models::{AppData, AuroraLevel};
use std::sync::Arc;
use chrono::Timelike;

/// GPU rendering context with full window integration
pub struct GpuContext {
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub config: SurfaceConfiguration,
    pub surface: Arc<Surface<'static>>,
    pub format: TextureFormat,
    #[allow(dead_code)]
    text_renderer: TextRenderer, // Used in Phase C for text rendering
    _window: Arc<Window>, // Held to ensure window stays alive
}

/// OPTIMIZATION: Select GPU present mode for power efficiency
/// Prioritizes vsync (Fifo) for low idle CPU usage
fn select_present_mode(available: &[PresentMode]) -> PresentMode {
    use PresentMode::*;

    // Priority: Fifo (vsync, power efficient) > AutoVsync > FifoRelaxed > fallback
    if available.contains(&Fifo) {
        log::debug!("GPU: Using PresentMode::Fifo (vsync, power efficient)");
        Fifo
    } else if available.contains(&AutoVsync) {
        log::debug!("GPU: Using PresentMode::AutoVsync (platform vsync)");
        AutoVsync
    } else if available.contains(&FifoRelaxed) {
        log::debug!("GPU: Using PresentMode::FifoRelaxed (adaptive sync)");
        FifoRelaxed
    } else {
        log::warn!("GPU: Preferred present modes unavailable, using first available: {:?}", available[0]);
        available[0]
    }
}

impl GpuContext {
    /// Create GPU context and initialize surface with window
    pub async fn new(window: Arc<Window>) -> AppResult<Self> {
        log::info!("Initializing GPU context...");

        // Create instance with all backends
        let instance = Instance::new(InstanceDescriptor {
            backends: Backends::all(),
            ..Default::default()
        });

        // Create surface from window
        // SAFETY: std::mem::transmute converts &Window to &'static Window
        // This is safe because Arc is kept alive in _window field, guaranteeing the window's lifetime
        let window_static: &'static Window = unsafe {
            std::mem::transmute::<&Window, &'static Window>(&window)
        };

        let surface = Arc::new(
            instance.create_surface(window_static)
                .map_err(|e| AppError::GpuError(format!("Failed to create surface: {}", e)))?
        );

        // Request adapter with surface preference
        let adapter = instance
            .request_adapter(&RequestAdapterOptions {
                power_preference: PowerPreference::HighPerformance,
                compatible_surface: Some(&surface),
                ..Default::default()
            })
            .await
            .ok_or_else(|| AppError::GpuError("No suitable GPU adapter found".to_string()))?;

        let adapter_info = adapter.get_info();
        log::info!("GPU: {} (backend: {:?})", adapter_info.name, adapter_info.backend);

        // Create device and queue
        let (device, queue) = adapter
            .request_device(
                &DeviceDescriptor {
                    label: Some("HamClock Main Device"),
                    required_features: Features::empty(),
                    required_limits: Limits::default(),
                },
                None,
            )
            .await
            .map_err(|e| AppError::GpuError(format!("Failed to request device: {}", e)))?;

        // Get surface format
        let surface_caps = surface.get_capabilities(&adapter);
        let surface_format = surface_caps
            .formats
            .iter()
            .copied()
            .find(|f| f.is_srgb())
            .unwrap_or(surface_caps.formats[0]);

        log::info!("Surface format: {:?}", surface_format);

        // Get window size
        let size = window.inner_size();
        let width = size.width.max(1);
        let height = size.height.max(1);

        // Configure surface with explicit present mode for power efficiency
        let present_mode = select_present_mode(&surface_caps.present_modes);
        let config = SurfaceConfiguration {
            usage: TextureUsages::RENDER_ATTACHMENT,
            format: surface_format,
            width,
            height,
            present_mode,
            alpha_mode: surface_caps.alpha_modes[0],
            view_formats: vec![],
            desired_maximum_frame_latency: 2,
        };

        // Configure the surface
        surface.configure(&device, &config);

        // Initialize text renderer
        let device_arc = Arc::new(device);
        let queue_arc = Arc::new(queue);
        let text_renderer = TextRenderer::new(
            Arc::clone(&device_arc),
            Arc::clone(&queue_arc),
            surface_format,
            width,
            height,
        )
            .map_err(|e| AppError::GpuError(format!("TextRenderer init failed: {}", e)))?;

        log::info!(
            "GPU context ready: {}x{}, format {:?}, mode {:?}",
            width, height, surface_format, config.present_mode
        );

        Ok(Self {
            device: device_arc,
            queue: queue_arc,
            config,
            surface,
            format: surface_format,
            text_renderer,
            _window: window,
        })
    }

    /// Render a frame to the window with current AppData
    pub fn render_frame(&mut self, app_data: &AppData) -> AppResult<()> {
        // Get current frame texture
        let frame = self.surface
            .get_current_texture()
            .map_err(|e| AppError::GpuError(format!("Failed to get frame texture: {:?}", e)))?;

        let view = frame.texture.create_view(&TextureViewDescriptor::default());

        // Get current time and space weather data
        let now = chrono::Local::now();
        let time_str = now.format("%H:%M:%S").to_string();

        // Queue UI elements for rendering
        self.queue_ui_elements(app_data, self.config.width, self.config.height)?;

        // Log time and space weather
        log::debug!("Clock: {} | Kp: {:.1} | Flux: {} SFU",
                  time_str, app_data.space_weather.kp, app_data.space_weather.solar_flux);

        // Create command encoder
        let mut encoder = self.device.create_command_encoder(&CommandEncoderDescriptor {
            label: Some("Main Render Encoder"),
        });

        {
            // Begin render pass with dynamic color based on time (for visual feedback)
            let seconds = now.second() as f64 / 60.0;
            let color_variation = 0.05 + (seconds * 0.05);

            let mut render_pass = encoder.begin_render_pass(&RenderPassDescriptor {
                label: Some("Main Render Pass"),
                color_attachments: &[Some(RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    ops: Operations {
                        load: LoadOp::Clear(Color {
                            r: 0.05,
                            g: 0.05,
                            b: 0.1 + color_variation,
                            a: 1.0,
                        }),
                        store: StoreOp::Store,
                    },
                })],
                depth_stencil_attachment: None,
                occlusion_query_set: None,
                timestamp_writes: None,
            });

            // Render text
            self.text_renderer.render(
                &mut render_pass,
                (self.config.width, self.config.height),
            )?;

            drop(render_pass);
        }

        // Submit commands to GPU
        self.queue.submit(std::iter::once(encoder.finish()));

        // Present to screen
        frame.present();

        Ok(())
    }

    /// Resize the rendering surface
    pub fn resize(&mut self, new_size: (u32, u32)) -> AppResult<()> {
        if new_size.0 > 0 && new_size.1 > 0 {
            self.config.width = new_size.0;
            self.config.height = new_size.1;
            self.surface.configure(&self.device, &self.config);
            self.text_renderer.resize(new_size);
            log::info!("Resized surface to {}x{}", new_size.0, new_size.1);
        }
        Ok(())
    }

    /// Get current surface dimensions
    pub fn dimensions(&self) -> (u32, u32) {
        (self.config.width, self.config.height)
    }

    /// Get surface format
    pub fn format(&self) -> TextureFormat {
        self.format
    }

    /// Get surface format
    pub fn surface(&self) -> &Arc<Surface<'static>> {
        &self.surface
    }

    /// Queue UI elements for rendering (time, space weather, alerts)
    fn queue_ui_elements(&mut self, app_data: &AppData, _width: u32, _height: u32) -> AppResult<()> {
        let now = chrono::Local::now();
        let time_str = now.format("%H:%M:%S").to_string();

        // Queue time display (top center, large, white)
        self.text_renderer.queue_text(
            &time_str,
            [100.0, 50.0],  // Position
            72.0,           // Font size
            [1.0, 1.0, 1.0, 1.0],  // White
        );

        // Queue Kp-index (left, color-coded by intensity)
        let kp_str = format!("Kp: {:.1}", app_data.space_weather.kp);
        let kp_color = self.kp_color(app_data.space_weather.kp);
        self.text_renderer.queue_text(
            &kp_str,
            [50.0, 150.0],
            36.0,
            kp_color,
        );

        // Queue Solar Flux (right, cyan)
        let flux_str = format!("Flux: {} SFU", app_data.space_weather.solar_flux);
        self.text_renderer.queue_text(
            &flux_str,
            [400.0, 150.0],
            36.0,
            [0.0, 1.0, 1.0, 1.0],  // Cyan
        );

        // Queue CME alerts if present (red, high priority)
        if !app_data.space_weather.active_cmes.is_empty() {
            let cme = &app_data.space_weather.active_cmes[0];
            let alert_str = format!("⚠ CME ALERT: {} km/s", cme.speed as i32);
            self.text_renderer.queue_text(
                &alert_str,
                [50.0, 220.0],
                24.0,
                [1.0, 0.0, 0.0, 1.0],  // Red
            );
        }

        // Queue Aurora forecast (color-coded by intensity)
        let aurora_str = format!("Aurora: {:?}", app_data.space_weather.aurora_activity);
        let aurora_color = self.aurora_color(&app_data.space_weather.aurora_activity);
        self.text_renderer.queue_text(
            &aurora_str,
            [50.0, 280.0],
            20.0,
            aurora_color,
        );

        // Queue geomagnetic status (bottom, white)
        let status_str = format!("Status: {}", app_data.space_weather.geomag_status);
        self.text_renderer.queue_text(
            &status_str,
            [50.0, 400.0],
            16.0,
            [0.8, 0.8, 0.8, 1.0],  // Light gray
        );

        // Queue satellite passes (if available) - Phase 7
        if !app_data.satellites.is_empty() {
            // Find satellite with highest elevation (closest to overhead)
            let best_sat = app_data.satellites.iter()
                .max_by(|a, b| a.elevation.partial_cmp(&b.elevation).unwrap())
                .unwrap();

            let sat_str = format!("{}: El {:.0}° Az {:.0}° ({}km)",
                best_sat.name, best_sat.elevation, best_sat.azimuth, best_sat.range as i32);
            self.text_renderer.queue_text(
                &sat_str,
                [50.0, 320.0],
                20.0,
                [1.0, 1.0, 0.0, 1.0],  // Yellow
            );
        }

        // Queue DX spots (if available) - Phase 7
        if !app_data.dx_spots.is_empty() {
            let spot = &app_data.dx_spots[0];  // Most recent spot
            let dx_str = format!("DX: {:.1} MHz {} by {}",
                spot.frequency, spot.callsign, spot.spotter);
            self.text_renderer.queue_text(
                &dx_str,
                [50.0, 350.0],
                20.0,
                [0.0, 1.0, 0.0, 1.0],  // Green
            );
        }

        log::debug!("UI elements queued for rendering");
        Ok(())
    }

    /// Get color for Kp-index based on intensity
    fn kp_color(&self, kp: f32) -> [f32; 4] {
        match kp {
            k if k < 3.0 => [0.0, 1.0, 0.0, 1.0],      // Green (quiet)
            k if k < 5.0 => [1.0, 1.0, 0.0, 1.0],      // Yellow (unsettled)
            k if k < 7.0 => [1.0, 0.65, 0.0, 1.0],     // Orange (active)
            k if k <= 9.0 => [1.0, 0.0, 0.0, 1.0],     // Red (storm)
            _ => [0.5, 0.0, 0.5, 1.0],                 // Purple (extreme)
        }
    }

    /// Get color for aurora forecast based on intensity
    fn aurora_color(&self, level: &AuroraLevel) -> [f32; 4] {
        match level {
            AuroraLevel::None => [0.5, 0.5, 0.5, 1.0],       // Gray
            AuroraLevel::Low => [0.55, 0.0, 1.0, 1.0],       // Purple
            AuroraLevel::Moderate => [0.0, 1.0, 1.0, 1.0],   // Cyan
            AuroraLevel::High => [0.0, 1.0, 0.0, 1.0],       // Green
            AuroraLevel::Extreme => [1.0, 0.0, 0.0, 1.0],    // Red
        }
    }
}
