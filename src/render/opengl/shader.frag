#version 450 core

#define Render_RectangleFlags_Texture      uint(1 << 0)
#define Render_RectangleFlags_MSDF         uint(1 << 1)
#define Render_RectangleFlags_AlphaMask    uint(1 << 2)
#define Render_RectangleFlags_TextureIndex uint(1 << 3)

layout(origin_upper_left) in vec4 gl_FragCoord;

in      vec4  vert_color;
in      vec2  vert_uv;
in flat uint  vert_flags;
in      float vert_thickness;
in      float vert_softness;
in      vec4  vert_radies;
in      vec2  vert_center;
in      vec2  vert_half_size;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_samplers[2];

float median_of_3(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

float sdf_box(vec2 point, vec2 half_size) {
    vec2 distance = abs(point) - half_size;
    return length(max(vec2(0.0), distance)) + min(max(distance.x, distance.y), 0.0);
}

void main() {
    vec4 texture_sample = vec4(1.0);
    float alpha = 1.0f;
    uint texture_index = vert_flags & Render_RectangleFlags_TextureIndex;

    if ((vert_flags & Render_RectangleFlags_Texture) != 0) {
        texture_sample = vec4(texture(uniform_samplers[texture_index], vert_uv).rgb, 1.0);
    }

    if ((vert_flags & Render_RectangleFlags_MSDF) != 0) {
        vec4 msdf_sample = texture(uniform_samplers[texture_index], vert_uv);
        float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5;

        alpha = clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0);
    } else if ((vert_flags & Render_RectangleFlags_AlphaMask) != 0) {
        alpha = texture(uniform_samplers[texture_index], vert_uv).r;
    } else {
        vec2  position = gl_FragCoord.xy - vert_center;
        // NOTE(simon): Compute corner index, left to right, top to bottom.
        int   corner_index = int(0.5 * sign(position.x) + sign(position.y) + 1.5);
        float outer_radius = vert_radies[corner_index];
        float inner_radius = outer_radius - vert_thickness;
        float outer_distance = sdf_box(position, vert_half_size - outer_radius) - outer_radius;
        float inner_distance = sdf_box(position, vert_half_size - inner_radius - vert_thickness) - inner_radius;
        float distance = max(outer_distance, -inner_distance);
        alpha = 1.0 - smoothstep(-vert_softness, vert_softness, distance);
    }

    frag_color = texture_sample * vert_color * vec4(1.0, 1.0, 1.0, alpha);
}
