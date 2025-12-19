//! 2D shape rendering primitives

use bytemuck::{Pod, Zeroable};
use wgpu::*;

/// Vertex structure for GPU rendering
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
pub struct Vertex {
    pub position: [f32; 2],
    pub color: [f32; 4],
}

impl Vertex {
    pub fn desc() -> VertexBufferLayout<'static> {
        VertexBufferLayout {
            array_stride: std::mem::size_of::<Vertex>() as BufferAddress,
            step_mode: VertexStepMode::Vertex,
            attributes: &[
                VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: VertexFormat::Float32x2,
                },
                VertexAttribute {
                    offset: std::mem::size_of::<[f32; 2]>() as BufferAddress,
                    shader_location: 1,
                    format: VertexFormat::Float32x4,
                },
            ],
        }
    }
}

/// Color type
#[derive(Copy, Clone, Debug)]
pub struct Color {
    pub r: f32,
    pub g: f32,
    pub b: f32,
    pub a: f32,
}

impl Color {
    pub fn new(r: f32, g: f32, b: f32, a: f32) -> Self {
        Self { r, g, b, a }
    }

    pub fn white() -> Self {
        Self::new(1.0, 1.0, 1.0, 1.0)
    }

    pub fn black() -> Self {
        Self::new(0.0, 0.0, 0.0, 1.0)
    }

    pub fn red() -> Self {
        Self::new(1.0, 0.0, 0.0, 1.0)
    }

    pub fn green() -> Self {
        Self::new(0.0, 1.0, 0.0, 1.0)
    }

    pub fn blue() -> Self {
        Self::new(0.0, 0.0, 1.0, 1.0)
    }

    pub fn as_array(&self) -> [f32; 4] {
        [self.r, self.g, self.b, self.a]
    }
}

/// Buffer for storing shape vertices
pub struct ShapeBuffer {
    pub vertices: Vec<Vertex>,
    pub indices: Vec<u16>,
}

impl ShapeBuffer {
    pub fn new() -> Self {
        Self {
            vertices: Vec::new(),
            indices: Vec::new(),
        }
    }

    pub fn clear(&mut self) {
        self.vertices.clear();
        self.indices.clear();
    }

    /// Add a line segment
    pub fn add_line(&mut self, x1: f32, y1: f32, x2: f32, y2: f32, color: Color) {
        let idx = self.vertices.len() as u16;
        self.vertices.push(Vertex {
            position: [x1, y1],
            color: color.as_array(),
        });
        self.vertices.push(Vertex {
            position: [x2, y2],
            color: color.as_array(),
        });
        self.indices.push(idx);
        self.indices.push(idx + 1);
    }

    /// Add a rectangle
    pub fn add_rect(&mut self, x: f32, y: f32, w: f32, h: f32, color: Color) {
        let idx = self.vertices.len() as u16;
        // Top-left
        self.vertices.push(Vertex {
            position: [x, y],
            color: color.as_array(),
        });
        // Top-right
        self.vertices.push(Vertex {
            position: [x + w, y],
            color: color.as_array(),
        });
        // Bottom-right
        self.vertices.push(Vertex {
            position: [x + w, y + h],
            color: color.as_array(),
        });
        // Bottom-left
        self.vertices.push(Vertex {
            position: [x, y + h],
            color: color.as_array(),
        });

        // Two triangles
        self.indices.extend_from_slice(&[
            idx,     idx + 1, idx + 2,  // First triangle
            idx,     idx + 2, idx + 3,  // Second triangle
        ]);
    }

    /// Add a circle (approximated with triangles)
    pub fn add_circle(&mut self, cx: f32, cy: f32, r: f32, segments: u32, color: Color) {
        let idx = self.vertices.len() as u16;

        // Center point
        self.vertices.push(Vertex {
            position: [cx, cy],
            color: color.as_array(),
        });

        // Circle points
        for i in 0..=segments {
            let angle = (i as f32 / segments as f32) * std::f32::consts::TAU;
            let x = cx + r * angle.cos();
            let y = cy + r * angle.sin();
            self.vertices.push(Vertex {
                position: [x, y],
                color: color.as_array(),
            });
        }

        // Triangles from center to each point
        for i in 0..segments {
            let i_u16 = i as u16;
            self.indices.extend_from_slice(&[
                idx,
                idx + i_u16 + 1,
                idx + i_u16 + 2,
            ]);
        }
    }
}

impl Default for ShapeBuffer {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_color_creation() {
        let white = Color::white();
        assert_eq!(white.r, 1.0);
        assert_eq!(white.g, 1.0);
        assert_eq!(white.b, 1.0);
    }

    #[test]
    fn test_shape_buffer() {
        let mut buf = ShapeBuffer::new();
        buf.add_line(0.0, 0.0, 1.0, 1.0, Color::white());
        assert_eq!(buf.vertices.len(), 2);
        assert_eq!(buf.indices.len(), 2);
    }
}
