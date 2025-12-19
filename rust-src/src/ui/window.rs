//! Main application window

use winit::window::Window;
use crate::AppResult;
use crate::render::GpuContext;
use crate::data::AppData;
use std::sync::Arc;
use tokio::sync::Mutex;

/// Main application window
pub struct MainWindow {
    pub window: Arc<Window>,
    pub gpu: GpuContext,
    pub data: Arc<Mutex<AppData>>,
}

impl MainWindow {
    /// Create a new main window
    pub async fn new(window: Arc<Window>, data: Arc<Mutex<AppData>>) -> AppResult<Self> {
        log::info!("Creating main window...");

        let gpu = GpuContext::new(Arc::clone(&window)).await?;

        Ok(Self {
            window,
            gpu,
            data,
        })
    }

    /// Handle window resize event
    pub async fn on_resize(&mut self, new_size: (u32, u32)) -> AppResult<()> {
        self.gpu.resize(new_size)
    }

    /// Render a frame
    pub async fn render_frame(&mut self) -> AppResult<()> {
        let data = self.data.lock().await;
        self.gpu.render_frame(&data)
    }

    /// Get window dimensions
    pub fn dimensions(&self) -> (u32, u32) {
        let size = self.window.inner_size();
        (size.width, size.height)
    }

    /// Request redraw
    pub fn request_redraw(&self) {
        self.window.request_redraw();
    }
}
