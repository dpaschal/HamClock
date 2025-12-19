//! Layer management and rendering system for HamClock
//!
//! Provides multi-layer visualization system for:
//! - Base maps (geographic, topographic, political)
//! - Overlays (greyline, MUF heatmap, aurora oval, satellites)
//! - Dynamic layer toggling and opacity control

use serde::{Deserialize, Serialize};

/// Supported visualization layers
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq, Eq, Hash)]
pub enum LayerType {
    /// Base geographic/topographic map
    BaseMap,
    /// Day/night terminator (greyline)
    Greyline,
    /// Maximum Usable Frequency heat map
    MufHeatmap,
    /// Aurora forecast oval overlay
    AuroraOval,
    /// Satellite footprint zones
    SatelliteFootprint,
    /// Maidenhead grid squares
    GridSquares,
    /// ITU regions boundary
    ItuRegions,
    /// Call sign prefix boundaries
    CallsignPrefix,
}

impl LayerType {
    pub fn name(&self) -> &'static str {
        match self {
            LayerType::BaseMap => "Base Map",
            LayerType::Greyline => "Greyline",
            LayerType::MufHeatmap => "MUF Heat Map",
            LayerType::AuroraOval => "Aurora Oval",
            LayerType::SatelliteFootprint => "Satellites",
            LayerType::GridSquares => "Grid",
            LayerType::ItuRegions => "ITU Regions",
            LayerType::CallsignPrefix => "Callsign Prefix",
        }
    }

    pub fn keyboard_shortcut(&self) -> char {
        match self {
            LayerType::MufHeatmap => '1',
            LayerType::AuroraOval => '2',
            LayerType::SatelliteFootprint => '3',
            LayerType::GridSquares => '4',
            LayerType::ItuRegions => '5',
            LayerType::CallsignPrefix => '6',
            _ => ' ',
        }
    }
}

/// Blend modes for layer compositing
#[derive(Debug, Clone, Copy, Serialize, Deserialize, PartialEq)]
pub enum BlendMode {
    /// No transparency, fully opaque
    Opaque,
    /// Standard alpha blending
    Blend,
    /// Additive blending (brightens)
    Add,
    /// Screen blending (lighten)
    Screen,
    /// Overlay blending
    Overlay,
    /// Multiply blending
    Multiply,
}

/// Single layer configuration
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Layer {
    pub layer_type: LayerType,
    pub visible: bool,
    pub opacity: f32,       // 0.0-1.0
    pub blend_mode: BlendMode,
}

impl Layer {
    pub fn new(layer_type: LayerType) -> Self {
        let (visible, opacity, blend_mode): (bool, f32, BlendMode) = match layer_type {
            LayerType::BaseMap => (true, 1.0, BlendMode::Opaque),
            LayerType::Greyline => (true, 0.8, BlendMode::Blend),
            LayerType::MufHeatmap => (false, 0.6, BlendMode::Add),
            LayerType::AuroraOval => (false, 0.5, BlendMode::Blend),
            LayerType::SatelliteFootprint => (true, 0.4, BlendMode::Blend),
            LayerType::GridSquares => (false, 0.3, BlendMode::Blend),
            LayerType::ItuRegions => (false, 0.25, BlendMode::Blend),
            LayerType::CallsignPrefix => (false, 0.2, BlendMode::Blend),
        };

        Self {
            layer_type,
            visible,
            opacity: opacity.clamp(0.0, 1.0),
            blend_mode,
        }
    }

    pub fn toggle(&mut self) {
        self.visible = !self.visible;
    }

    pub fn set_opacity(&mut self, opacity: f32) {
        self.opacity = opacity.clamp(0.0, 1.0);
    }
}

/// Layer manager for multi-layer visualization
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct LayerManager {
    pub layers: Vec<Layer>,
    pub active_layer: LayerType,
}

impl Default for LayerManager {
    fn default() -> Self {
        Self {
            layers: vec![
                Layer::new(LayerType::BaseMap),
                Layer::new(LayerType::Greyline),
                Layer::new(LayerType::MufHeatmap),
                Layer::new(LayerType::AuroraOval),
                Layer::new(LayerType::SatelliteFootprint),
                Layer::new(LayerType::GridSquares),
                Layer::new(LayerType::ItuRegions),
                Layer::new(LayerType::CallsignPrefix),
            ],
            active_layer: LayerType::BaseMap,
        }
    }
}

impl LayerManager {
    pub fn new() -> Self {
        Self::default()
    }

    /// Toggle layer visibility
    pub fn toggle_layer(&mut self, layer_type: LayerType) -> bool {
        if let Some(layer) = self.layers.iter_mut().find(|l| l.layer_type == layer_type) {
            layer.toggle();
            log::info!("Layer '{}' toggled: {}", layer_type.name(), layer.visible);
            return layer.visible;
        }
        false
    }

    /// Set layer opacity (0.0-1.0)
    pub fn set_opacity(&mut self, layer_type: LayerType, opacity: f32) {
        if let Some(layer) = self.layers.iter_mut().find(|l| l.layer_type == layer_type) {
            layer.set_opacity(opacity);
            log::debug!("Layer '{}' opacity set to {:.2}", layer_type.name(), layer.opacity);
        }
    }

    /// Get layer configuration
    pub fn get_layer(&self, layer_type: LayerType) -> Option<&Layer> {
        self.layers.iter().find(|l| l.layer_type == layer_type)
    }

    /// Get mutable layer configuration
    pub fn get_layer_mut(&mut self, layer_type: LayerType) -> Option<&mut Layer> {
        self.layers.iter_mut().find(|l| l.layer_type == layer_type)
    }

    /// Get all visible layers in render order
    pub fn visible_layers(&self) -> Vec<&Layer> {
        self.layers.iter()
            .filter(|l| l.visible)
            .collect()
    }

    /// Set active layer for editing
    pub fn set_active(&mut self, layer_type: LayerType) {
        self.active_layer = layer_type;
        log::debug!("Active layer set to: {}", layer_type.name());
    }

    /// Get rendering order (back to front)
    pub fn rendering_order(&self) -> Vec<LayerType> {
        vec![
            LayerType::BaseMap,           // Back layer
            LayerType::Greyline,
            LayerType::MufHeatmap,
            LayerType::ItuRegions,
            LayerType::CallsignPrefix,
            LayerType::GridSquares,
            LayerType::SatelliteFootprint,
            LayerType::AuroraOval,        // Front layer
        ]
    }

    /// Toggle layer by keyboard shortcut
    pub fn toggle_by_key(&mut self, key: char) -> Option<bool> {
        for layer_type in &[
            LayerType::MufHeatmap,
            LayerType::AuroraOval,
            LayerType::SatelliteFootprint,
            LayerType::GridSquares,
            LayerType::ItuRegions,
            LayerType::CallsignPrefix,
        ] {
            if layer_type.keyboard_shortcut() == key {
                return Some(self.toggle_layer(*layer_type));
            }
        }
        None
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_layer_creation() {
        let layer = Layer::new(LayerType::MufHeatmap);
        assert_eq!(layer.layer_type, LayerType::MufHeatmap);
        assert!(!layer.visible);
        assert_eq!(layer.opacity, 0.6);
    }

    #[test]
    fn test_layer_toggle() {
        let mut layer = Layer::new(LayerType::Greyline);
        assert!(layer.visible);
        layer.toggle();
        assert!(!layer.visible);
        layer.toggle();
        assert!(layer.visible);
    }

    #[test]
    fn test_layer_opacity_clamping() {
        let mut layer = Layer::new(LayerType::BaseMap);
        layer.set_opacity(1.5);
        assert_eq!(layer.opacity, 1.0);
        layer.set_opacity(-0.5);
        assert_eq!(layer.opacity, 0.0);
    }

    #[test]
    fn test_layer_manager_default() {
        let manager = LayerManager::new();
        assert_eq!(manager.layers.len(), 8);
        assert!(manager.get_layer(LayerType::BaseMap).unwrap().visible);
        assert!(!manager.get_layer(LayerType::MufHeatmap).unwrap().visible);
    }

    #[test]
    fn test_layer_manager_toggle() {
        let mut manager = LayerManager::new();
        let was_visible = manager.get_layer(LayerType::MufHeatmap).unwrap().visible;
        let is_visible = manager.toggle_layer(LayerType::MufHeatmap);
        assert_eq!(is_visible, !was_visible);
    }

    #[test]
    fn test_keyboard_shortcuts() {
        assert_eq!(LayerType::MufHeatmap.keyboard_shortcut(), '1');
        assert_eq!(LayerType::AuroraOval.keyboard_shortcut(), '2');
        assert_eq!(LayerType::SatelliteFootprint.keyboard_shortcut(), '3');
        assert_eq!(LayerType::GridSquares.keyboard_shortcut(), '4');
    }

    #[test]
    fn test_toggle_by_key() {
        let mut manager = LayerManager::new();
        let result = manager.toggle_by_key('1');
        assert!(result.is_some());
        assert_eq!(result.unwrap(), true);
        assert!(manager.get_layer(LayerType::MufHeatmap).unwrap().visible);
    }

    #[test]
    fn test_visible_layers() {
        let mut manager = LayerManager::new();
        manager.toggle_layer(LayerType::MufHeatmap);
        let visible = manager.visible_layers();
        assert_eq!(visible.len(), 4); // BaseMap, Greyline, MufHeatmap, SatelliteFootprint
    }
}
