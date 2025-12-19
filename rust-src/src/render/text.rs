//! Text rendering using glyph_brush
//!
//! This module provides text rendering capabilities for displaying:
//! - Time (HH:MM:SS)
//! - Space weather data (Kp-index, solar flux, aurora forecasts, CME alerts)
//! - Metrics and status information
//!
//! Uses glyph_brush 0.7 (compatible with wgpu 0.19) for glyph rasterization

use crate::error::{AppResult, AppError};
use glyph_brush::{GlyphBrush, Section, Text, GlyphBrushBuilder, ab_glyph::FontArc, BrushAction};
use bytemuck::{Pod, Zeroable};
use std::sync::Arc;
use wgpu::{Device, Queue, RenderPipeline, BindGroup, Texture, TextureView, Buffer};
use wgpu::util::DeviceExt;

/// GPU vertex data for a single text quad
#[repr(C)]
#[derive(Copy, Clone, Debug, Pod, Zeroable)]
struct TextVertex {
    position: [f32; 2],      // Clip space [-1, 1]
    tex_coords: [f32; 2],    // UV in atlas [0, 1]
    color: [f32; 4],         // RGBA
}

impl TextVertex {
    fn desc() -> wgpu::VertexBufferLayout<'static> {
        wgpu::VertexBufferLayout {
            array_stride: std::mem::size_of::<TextVertex>() as wgpu::BufferAddress,
            step_mode: wgpu::VertexStepMode::Vertex,
            attributes: &[
                wgpu::VertexAttribute {
                    offset: 0,
                    shader_location: 0,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: 8,
                    shader_location: 1,
                    format: wgpu::VertexFormat::Float32x2,
                },
                wgpu::VertexAttribute {
                    offset: 16,
                    shader_location: 2,
                    format: wgpu::VertexFormat::Float32x4,
                },
            ],
        }
    }
}

/// Intermediate quad representation from glyph_brush
#[derive(Copy, Clone, Debug)]
struct GlyphQuad {
    pixel_min: [f32; 2],
    pixel_max: [f32; 2],
    tex_min: [f32; 2],
    tex_max: [f32; 2],
    color: [f32; 4],
}

/// Text rendering system with GPU integration
///
/// Handles text queueing and full GPU rendering pipeline with glyph_brush integration.
pub struct TextRenderer {
    /// Glyph rasterization cache
    glyph_brush: GlyphBrush<()>,
    /// Pending text sections to render this frame
    pending_sections: Vec<(String, [f32; 2], f32, [f32; 4])>,

    // GPU resources
    device: Arc<Device>,
    queue: Arc<Queue>,
    pipeline: RenderPipeline,
    bind_group: BindGroup,
    atlas_texture: Texture,
    #[allow(dead_code)]
    atlas_texture_view: TextureView,

    // Dynamic geometry (recreated each frame)
    vertex_buffer: Option<Buffer>,
    index_buffer: Option<Buffer>,
    cached_vertices: Vec<TextVertex>,
    cached_indices: Vec<u16>,

    // Surface dimensions for coordinate transform
    surface_width: f32,
    surface_height: f32,
}

impl TextRenderer {
    /// Create a new text renderer with GPU resources
    pub fn new(
        device: Arc<Device>,
        queue: Arc<Queue>,
        surface_format: wgpu::TextureFormat,
        surface_width: u32,
        surface_height: u32,
    ) -> AppResult<Self> {
        log::info!("Initializing TextRenderer with GPU resources...");

        // Load font
        let font = FontArc::try_from_slice(include_bytes!("../../assets/fonts/Roboto-Regular.ttf"))
            .map_err(|e| AppError::GpuError(format!("Failed to load font: {}", e)))?;

        // Create glyph brush
        let glyph_brush: GlyphBrush<()> = GlyphBrushBuilder::using_font(font)
            .initial_cache_size((512, 512))
            .build();

        // Create atlas texture (512x512 R8Unorm for grayscale glyph coverage)
        let atlas_texture = device.create_texture(&wgpu::TextureDescriptor {
            label: Some("Text Atlas"),
            size: wgpu::Extent3d {
                width: 512,
                height: 512,
                depth_or_array_layers: 1,
            },
            mip_level_count: 1,
            sample_count: 1,
            dimension: wgpu::TextureDimension::D2,
            format: wgpu::TextureFormat::R8Unorm,
            usage: wgpu::TextureUsages::TEXTURE_BINDING | wgpu::TextureUsages::COPY_DST,
            view_formats: &[],
        });

        let atlas_texture_view = atlas_texture.create_view(&wgpu::TextureViewDescriptor::default());

        // Create sampler with linear filtering for smooth text
        let sampler = device.create_sampler(&wgpu::SamplerDescriptor {
            label: Some("Text Sampler"),
            address_mode_u: wgpu::AddressMode::ClampToEdge,
            address_mode_v: wgpu::AddressMode::ClampToEdge,
            address_mode_w: wgpu::AddressMode::ClampToEdge,
            mag_filter: wgpu::FilterMode::Linear,
            min_filter: wgpu::FilterMode::Linear,
            mipmap_filter: wgpu::FilterMode::Linear,
            ..Default::default()
        });

        // Create bind group layout
        let bind_group_layout = device.create_bind_group_layout(&wgpu::BindGroupLayoutDescriptor {
            label: Some("Text Bind Group Layout"),
            entries: &[
                wgpu::BindGroupLayoutEntry {
                    binding: 0,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Texture {
                        sample_type: wgpu::TextureSampleType::Float { filterable: true },
                        view_dimension: wgpu::TextureViewDimension::D2,
                        multisampled: false,
                    },
                    count: None,
                },
                wgpu::BindGroupLayoutEntry {
                    binding: 1,
                    visibility: wgpu::ShaderStages::FRAGMENT,
                    ty: wgpu::BindingType::Sampler(wgpu::SamplerBindingType::Filtering),
                    count: None,
                },
            ],
        });

        // Create bind group
        let bind_group = device.create_bind_group(&wgpu::BindGroupDescriptor {
            label: Some("Text Bind Group"),
            layout: &bind_group_layout,
            entries: &[
                wgpu::BindGroupEntry {
                    binding: 0,
                    resource: wgpu::BindingResource::TextureView(&atlas_texture_view),
                },
                wgpu::BindGroupEntry {
                    binding: 1,
                    resource: wgpu::BindingResource::Sampler(&sampler),
                },
            ],
        });

        // Load shader
        let shader = device.create_shader_module(wgpu::ShaderModuleDescriptor {
            label: Some("Text Shader"),
            source: wgpu::ShaderSource::Wgsl(include_str!("shaders/text.wgsl").into()),
        });

        // Create pipeline layout
        let pipeline_layout = device.create_pipeline_layout(&wgpu::PipelineLayoutDescriptor {
            label: Some("Text Pipeline Layout"),
            bind_group_layouts: &[&bind_group_layout],
            push_constant_ranges: &[],
        });

        // Create render pipeline
        let pipeline = device.create_render_pipeline(&wgpu::RenderPipelineDescriptor {
            label: Some("Text Pipeline"),
            layout: Some(&pipeline_layout),
            vertex: wgpu::VertexState {
                module: &shader,
                entry_point: "vs_main",
                buffers: &[TextVertex::desc()],
            },
            primitive: wgpu::PrimitiveState {
                topology: wgpu::PrimitiveTopology::TriangleList,
                strip_index_format: None,
                front_face: wgpu::FrontFace::Ccw,
                cull_mode: None,
                unclipped_depth: false,
                polygon_mode: wgpu::PolygonMode::Fill,
                conservative: false,
            },
            depth_stencil: None,
            multisample: wgpu::MultisampleState {
                count: 1,
                mask: !0,
                alpha_to_coverage_enabled: false,
            },
            fragment: Some(wgpu::FragmentState {
                module: &shader,
                entry_point: "fs_main",
                targets: &[Some(wgpu::ColorTargetState {
                    format: surface_format,
                    blend: Some(wgpu::BlendState::ALPHA_BLENDING),
                    write_mask: wgpu::ColorWrites::ALL,
                })],
            }),
            multiview: None,
        });

        log::info!("TextRenderer GPU resources initialized");

        Ok(Self {
            glyph_brush,
            pending_sections: Vec::new(),
            device,
            queue,
            pipeline,
            bind_group,
            atlas_texture,
            atlas_texture_view,
            vertex_buffer: None,
            index_buffer: None,
            cached_vertices: Vec::new(),
            cached_indices: Vec::new(),
            surface_width: surface_width as f32,
            surface_height: surface_height as f32,
        })
    }

    /// Queue text for rendering
    ///
    /// # Arguments
    /// * `text` - Text string to render
    /// * `position` - [x, y] position in screen coordinates
    /// * `size` - Font size in pixels
    /// * `color` - RGBA color [r, g, b, a] in range [0.0, 1.0]
    pub fn queue_text(&mut self, text: &str, position: [f32; 2], size: f32, color: [f32; 4]) {
        self.pending_sections.push((
            text.to_string(),
            position,
            size,
            color,
        ));
    }

    /// Render all queued text to the surface
    ///
    /// Call this once per frame after queueing all text sections
    pub fn render<'a>(
        &'a mut self,
        render_pass: &mut wgpu::RenderPass<'a>,
        surface_size: (u32, u32),
    ) -> AppResult<()> {
        // Update dimensions
        self.surface_width = surface_size.0 as f32;
        self.surface_height = surface_size.1 as f32;

        // Early return if no text
        if self.pending_sections.is_empty() {
            log::warn!("Text render: No pending sections to render!");
            return Ok(());
        }

        log::info!("Text render: Processing {} pending sections (bounds: {}x{})",
                  self.pending_sections.len(), self.surface_width as i32, self.surface_height as i32);

        // Queue all pending text with glyph_brush
        for (text, position, size, color) in self.pending_sections.drain(..) {
            log::debug!("Queueing text: '{}' at [{:.1}, {:.1}] size {:.1}", text, position[0], position[1], size);
            let section = Section {
                screen_position: (position[0], position[1]),
                bounds: (self.surface_width, self.surface_height),
                text: vec![Text::new(&text)
                    .with_scale(size)
                    .with_color(color)],
                ..Section::default()
            };
            self.glyph_brush.queue(section);
        }

        // Process queued glyphs - collect quads in closure
        use std::cell::RefCell;
        let quads_ref = RefCell::new(Vec::new());

        // Create references for closures to avoid borrow conflicts
        let queue = &self.queue;
        let atlas_texture = &self.atlas_texture;

        let brush_action = self.glyph_brush.process_queued(
            |rect, tex_data| {
                // Update atlas texture directly
                queue.write_texture(
                    wgpu::ImageCopyTexture {
                        texture: atlas_texture,
                        mip_level: 0,
                        origin: wgpu::Origin3d {
                            x: rect.min[0],
                            y: rect.min[1],
                            z: 0,
                        },
                        aspect: wgpu::TextureAspect::All,
                    },
                    tex_data,
                    wgpu::ImageDataLayout {
                        offset: 0,
                        bytes_per_row: Some(rect.width()),
                        rows_per_image: Some(rect.height()),
                    },
                    wgpu::Extent3d {
                        width: rect.width(),
                        height: rect.height(),
                        depth_or_array_layers: 1,
                    },
                );
            },
            |glyph_vertex| {
                // Collect glyph vertices as quads
                let quad = GlyphQuad {
                    pixel_min: [glyph_vertex.pixel_coords.min.x, glyph_vertex.pixel_coords.min.y],
                    pixel_max: [glyph_vertex.pixel_coords.max.x, glyph_vertex.pixel_coords.max.y],
                    tex_min: [glyph_vertex.tex_coords.min.x, glyph_vertex.tex_coords.min.y],
                    tex_max: [glyph_vertex.tex_coords.max.x, glyph_vertex.tex_coords.max.y],
                    color: glyph_vertex.extra.color,
                };
                quads_ref.borrow_mut().push(quad);
            },
        );

        let quads = quads_ref.into_inner();
        log::info!("Text render: Brush generated {} quads", quads.len());

        match brush_action {
            Ok(BrushAction::Draw(_)) => {
                log::info!("Text render: BrushAction::Draw");
                if !quads.is_empty() {
                    self.generate_geometry(&quads);
                    self.create_buffers();
                    self.draw_impl(render_pass);
                    log::info!("Text render: {} glyphs drawn", quads.len());
                } else {
                    log::warn!("Text render: Draw action but no quads!");
                }
            }
            Ok(BrushAction::ReDraw) => {
                log::info!("Text render: BrushAction::ReDraw with {} quads", quads.len());
                // IMPORTANT: Only regenerate geometry if we have NEW quads
                // This ensures text changes are reflected while preserving cached geometry
                if !quads.is_empty() {
                    self.generate_geometry(&quads);
                    self.create_buffers();
                    log::info!("Text render: regenerated geometry for {} quads", quads.len());
                } else {
                    log::info!("Text render: using cached geometry (text unchanged)");
                }
                // Draw the geometry (either new or cached)
                if self.vertex_buffer.is_some() {
                    self.draw_impl(render_pass);
                } else {
                    log::warn!("Text render: No vertex buffer to draw!");
                }
            }
            Err(glyph_brush::BrushError::TextureTooSmall { suggested }) => {
                log::error!("Atlas too small! Suggested: {:?}. Skipping frame.", suggested);
                return Ok(());
            }
            other => {
                log::error!("Text render: Unexpected brush action: {:?}", other);
            }
        }

        Ok(())
    }

    /// Transform screen-space pixel coordinates to clip space [-1, 1]
    fn screen_to_clip(&self, pixel: [f32; 2]) -> [f32; 2] {
        [
            (pixel[0] / self.surface_width) * 2.0 - 1.0,
            -((pixel[1] / self.surface_height) * 2.0 - 1.0),
        ]
    }

    /// Generate quad vertices and indices from glyphs
    fn generate_geometry(&mut self, quads: &[GlyphQuad]) {
        self.cached_vertices.clear();
        self.cached_indices.clear();

        for quad in quads {
            let base_idx = self.cached_vertices.len() as u16;

            // 4 vertices per quad (counter-clockwise)
            let vertices = [
                TextVertex {
                    position: self.screen_to_clip(quad.pixel_min),
                    tex_coords: quad.tex_min,
                    color: quad.color,
                },
                TextVertex {
                    position: self.screen_to_clip([quad.pixel_max[0], quad.pixel_min[1]]),
                    tex_coords: [quad.tex_max[0], quad.tex_min[1]],
                    color: quad.color,
                },
                TextVertex {
                    position: self.screen_to_clip(quad.pixel_max),
                    tex_coords: quad.tex_max,
                    color: quad.color,
                },
                TextVertex {
                    position: self.screen_to_clip([quad.pixel_min[0], quad.pixel_max[1]]),
                    tex_coords: [quad.tex_min[0], quad.tex_max[1]],
                    color: quad.color,
                },
            ];

            self.cached_vertices.extend_from_slice(&vertices);

            // 2 triangles (6 indices) per quad
            self.cached_indices.extend_from_slice(&[
                base_idx, base_idx + 1, base_idx + 2,
                base_idx, base_idx + 2, base_idx + 3,
            ]);
        }
    }

    /// Create GPU buffers from cached geometry
    fn create_buffers(&mut self) {
        if self.cached_vertices.is_empty() {
            return;
        }

        self.vertex_buffer = Some(self.device.create_buffer_init(
            &wgpu::util::BufferInitDescriptor {
                label: Some("Text Vertex Buffer"),
                contents: bytemuck::cast_slice(&self.cached_vertices),
                usage: wgpu::BufferUsages::VERTEX,
            }
        ));

        self.index_buffer = Some(self.device.create_buffer_init(
            &wgpu::util::BufferInitDescriptor {
                label: Some("Text Index Buffer"),
                contents: bytemuck::cast_slice(&self.cached_indices),
                usage: wgpu::BufferUsages::INDEX,
            }
        ));
    }

    /// Execute draw call for text quads
    fn draw_impl<'a>(&'a self, render_pass: &mut wgpu::RenderPass<'a>) {
        if let (Some(vb), Some(ib)) = (&self.vertex_buffer, &self.index_buffer) {
            render_pass.set_pipeline(&self.pipeline);
            render_pass.set_bind_group(0, &self.bind_group, &[]);
            render_pass.set_vertex_buffer(0, vb.slice(..));
            render_pass.set_index_buffer(ib.slice(..), wgpu::IndexFormat::Uint16);
            render_pass.draw_indexed(0..self.cached_indices.len() as u32, 0, 0..1);
        }
    }

    /// Update surface dimensions (called on window resize)
    pub fn resize(&mut self, new_size: (u32, u32)) {
        self.surface_width = new_size.0 as f32;
        self.surface_height = new_size.1 as f32;
    }
}
