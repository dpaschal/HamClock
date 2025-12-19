//! Application views and view management

use crate::render::GpuContext;
use crate::data::AppData;

/// Application view/screen
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum View {
    Clock,
    PropagationMap,
    Forecast,
    Moon,
    Satellite,
    DxCluster,
}

impl View {
    /// Get list of all available views
    pub fn all() -> &'static [View] {
        &[
            View::Clock,
            View::PropagationMap,
            View::Forecast,
            View::Moon,
            View::Satellite,
            View::DxCluster,
        ]
    }

    /// Get human-readable name for view
    pub fn name(&self) -> &'static str {
        match self {
            View::Clock => "Clock",
            View::PropagationMap => "Propagation Map",
            View::Forecast => "Forecast",
            View::Moon => "Moon",
            View::Satellite => "Satellite",
            View::DxCluster => "DX Cluster",
        }
    }

    /// Render this view
    pub fn render(&self, _gpu: &GpuContext, _data: &AppData) -> crate::AppResult<()> {
        log::debug!("Rendering view: {}", self.name());
        // TODO: Implement view rendering
        Ok(())
    }
}

impl Default for View {
    fn default() -> Self {
        View::Clock
    }
}
