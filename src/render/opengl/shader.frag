#version 450 core

#define Render_ShapeFlag_Texture   uint(1 << 0)
#define Render_ShapeFlag_MSDF      uint(1 << 1)
#define Render_ShapeFlag_AlphaMask uint(1 << 2)
#define Render_ShapeFlag_Line      uint(1 << 3)

layout(origin_upper_left) in vec4 gl_FragCoord;

in flat mat4  vert_colors;
in      vec2  vert_source;
in flat uint  vert_flags;
in flat float vert_thickness;
in flat float vert_softness;
in flat vec4  vert_radies;
in      vec2  vert_position;
in flat vec2  vert_half_size;
in      vec2  vert_uv;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;
uniform mat3      uniform_transfor;

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

    if ((vert_flags & Render_ShapeFlag_Texture) != 0) {
        texture_sample = vec4(texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0)).rgb, 1.0);
    }

    if ((vert_flags & Render_ShapeFlag_MSDF) != 0) {
        vec4 msdf_sample = texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0));
        float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5;

        alpha = clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0);
    } else if ((vert_flags & Render_ShapeFlag_AlphaMask) != 0) {
        alpha = texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0)).r;
    } else {
        int   corner_index = int(0.5 * sign(vert_position.x) + sign(vert_position.y) + 1.5);
        float outer_radius = vert_radies[corner_index];
        float inner_radius = outer_radius - vert_thickness;
        float outer_distance = sdf_box(vert_position, vert_half_size - outer_radius) - outer_radius;
        float inner_distance = -outer_distance;
        if (vert_thickness > 0.0) {
            inner_distance = sdf_box(vert_position, vert_half_size - inner_radius - vert_thickness) - inner_radius;
        }
        float distance = max(outer_distance, -inner_distance);
        alpha = 1.0 - smoothstep(0, vert_softness, distance);
    }

    vec4 color = mix(
        mix(vert_colors[0], vert_colors[1], vert_uv.x),
        mix(vert_colors[2], vert_colors[3], vert_uv.x),
        vert_uv.y
    );

    frag_color = texture_sample * color * vec4(1.0, 1.0, 1.0, alpha);
}
