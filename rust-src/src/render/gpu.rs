//! GPU context initialization and management with wgpu

use wgpu::*;
use winit::window::Window;
use crate::AppResult;
use crate::error::AppError;
use std::sync::Arc;
use chrono::Timelike;

/// GPU rendering context with full window integration
pub struct GpuContext {
    pub device: Arc<Device>,
    pub queue: Arc<Queue>,
    pub config: SurfaceConfiguration,
    pub surface: Arc<Surface<'static>>,
    pub format: TextureFormat,
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

        log::info!(
            "GPU context ready: {}x{}, format {:?}, mode {:?}",
            width, height, surface_format, config.present_mode
        );

        Ok(Self {
            device: Arc::new(device),
            queue: Arc::new(queue),
            config,
            surface,
            format: surface_format,
            _window: window,
        })
    }

    /// Render a frame to the window
    pub fn render_frame(&self) -> AppResult<()> {
        // Get current frame texture
        let frame = self.surface
            .get_current_texture()
            .map_err(|e| AppError::GpuError(format!("Failed to get frame texture: {:?}", e)))?;

        let view = frame.texture.create_view(&TextureViewDescriptor::default());

        // Get current time and space weather data
        let now = chrono::Local::now();
        let time_str = now.format("%H:%M:%S").to_string();

        // Log time and space weather placeholder
        log::debug!("Clock: {} | Kp: -- | Flux: --", time_str);

        // Create command encoder
        let mut encoder = self.device.create_command_encoder(&CommandEncoderDescriptor {
            label: Some("Main Render Encoder"),
        });

        {
            // Begin render pass with dynamic color based on time (for visual feedback)
            let seconds = now.second() as f64 / 60.0;
            let color_variation = 0.05 + (seconds * 0.05);

            let _render_pass = encoder.begin_render_pass(&RenderPassDescriptor {
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

            // Render commands placeholder for Phase 6.1
            // Text rendering will be added with wgpu-glyph in next iteration
            log::debug!("Render pass: {}x{} cleared", self.config.width, self.config.height);
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
}
