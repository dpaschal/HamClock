// Text rendering shader for glyph atlas
// Renders text glyphs from a cached texture atlas with per-vertex coloring

// Vertex input
struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) tex_coords: vec2<f32>,
    @location(2) color: vec4<f32>,
}

// Vertex output / Fragment input
struct VertexOutput {
    @builtin(position) clip_position: vec4<f32>,
    @location(0) tex_coords: vec2<f32>,
    @location(1) color: vec4<f32>,
}

// Bindings
@group(0) @binding(0)
var glyph_texture: texture_2d<f32>;

@group(0) @binding(1)
var glyph_sampler: sampler;

/// Vertex shader - pass through position and attributes
@vertex
fn vs_main(input: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    // Convert screen space to clip space
    // Assuming input is in screen pixels with origin at top-left
    // For now, pass through - proper transformation in Phase 6.1
    output.clip_position = vec4<f32>(input.position, 0.0, 1.0);
    output.tex_coords = input.tex_coords;
    output.color = input.color;
    return output;
}

/// Fragment shader - sample glyph texture and apply color
@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    // Sample glyph coverage from texture atlas (R8Unorm format)
    let glyph_alpha = textureSample(glyph_texture, glyph_sampler, input.tex_coords).r;

    // Apply glyph coverage to vertex color
    // Premultiplied alpha blending
    let final_color = vec4<f32>(
        input.color.rgb,
        input.color.a * glyph_alpha
    );

    return final_color;
}
