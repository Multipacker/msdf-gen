#version 330 core

#define Render_ShapeFlag_Texture   uint(1 << 0)
#define Render_ShapeFlag_MSDF      uint(1 << 1)
#define Render_ShapeFlag_AlphaMask uint(1 << 2)
#define Render_ShapeFlag_Line      uint(1 << 3)

layout(origin_upper_left) in vec4 gl_FragCoord;

flat in mat4  vert_colors;
     in vec2  vert_source;
flat in uint  vert_flags;
flat in float vert_thickness;
flat in float vert_softness;
flat in vec4  vert_radies;
     in vec2  vert_position;
flat in vec2  vert_half_size;
     in vec2  vert_uv;

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

vec4 linear_from_srgb(vec4 srgb) {
    bvec3 cutoff = lessThan(srgb.rgb, vec3(0.04045));
    vec3  higher = pow((srgb.rgb + 0.055) / 1.055, vec3(2.4));
    vec3  lower  = srgb.rgb / 12.92;
    vec4  linear = vec4(mix(higher, lower, cutoff), srgb.a);
    return linear;
}

vec4 srgb_from_linear(vec4 linear) {
    bvec3 cutoff = lessThan(linear.rgb, vec3(0.0031308));
    vec3  lower  = 12.92 * linear.rgb;
    vec3  higher = 1.055 * pow(linear.rgb, vec3(1.0 / 2.4)) - 0.055;
    vec4  srgb   = vec4(mix(higher, lower, cutoff), linear.a);
    return srgb;
}

void main() {
    vec4 texture_sample = vec4(1.0);
    float alpha = 1.0f;

    if ((vert_flags & Render_ShapeFlag_Texture) != 0u) {
        texture_sample = vec4(texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0)).rgb, 1.0);
    }

    if ((vert_flags & Render_ShapeFlag_MSDF) != 0u) {
        vec4 msdf_sample = texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0));
        float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5;

        alpha *= clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0);
    } else if ((vert_flags & Render_ShapeFlag_AlphaMask) != 0u) {
        alpha *= texture(uniform_sampler, vert_source / textureSize(uniform_sampler, 0)).r;
    } else {
        // NOTE(simon): Box with potentially rounded corners and border.
        int   corner_index = int(0.5 * sign(vert_position.x) + sign(vert_position.y) + 1.5);
        float outer_radius = vert_radies[corner_index];
        float inner_radius = outer_radius - vert_thickness;

        if (vert_thickness > 0.0) {
            float inner_distance = sdf_box(vert_position, vert_half_size - inner_radius - vert_thickness - 2.0 * vert_softness) - inner_radius;
            alpha *= smoothstep(0, 2.0 * vert_softness, inner_distance);
        }

        if (outer_radius > 0.0 || vert_softness > 0.75) {
            float outer_distance = sdf_box(vert_position, vert_half_size - outer_radius - 2.0 * vert_softness) - outer_radius;
            alpha *= 1.0 - smoothstep(0.0, 2.0 * vert_softness, outer_distance);
        }
    }

    vec4 color = linear_from_srgb(mix(
        mix(srgb_from_linear(vert_colors[0]), srgb_from_linear(vert_colors[1]), vert_uv.x),
        mix(srgb_from_linear(vert_colors[2]), srgb_from_linear(vert_colors[3]), vert_uv.x),
        vert_uv.y
    ));

    frag_color = texture_sample * color * vec4(1.0, 1.0, 1.0, alpha);
}
