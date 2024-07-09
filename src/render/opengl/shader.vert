#version 450 core

layout(location = 0)  in vec2  instance_min;
layout(location = 1)  in vec2  instance_max;
layout(location = 2)  in mat4  instance_colors;
layout(location = 6)  in vec2  instance_uv_min;
layout(location = 7)  in vec2  instance_uv_max;
layout(location = 8)  in uint  instance_flags;
layout(location = 9)  in float instance_thickness;
layout(location = 10) in float instance_softness;
layout(location = 11) in vec4  instance_radies;

out      vec4  vert_color;
out      vec2  vert_uv;
out flat uint  vert_flags;
out      float vert_thickness;
out      float vert_softness;
out      vec4  vert_radies;
out      vec2  vert_center;
out      vec2  vert_half_size;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;

const vec2 verticies[] = {
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(-1.0, +1.0),
    vec2(+1.0, +1.0)
};

void main() {
    vec2 center       = 0.5 * (instance_max + instance_min);
    vec2 half_size    = 0.5 * (instance_max - instance_min);
    vec2 position     = center + half_size * verticies[gl_VertexID];
    vec2 uv_center    = 0.5 * (instance_uv_max + instance_uv_min);
    vec2 uv_half_size = 0.5 * (instance_uv_max - instance_uv_min);
    vec2 uv           = uv_center + uv_half_size * verticies[gl_VertexID];

    gl_Position    = uniform_projection * vec4(position, 0.0, 1.0);
    vert_color     = instance_colors[gl_VertexID];
    vert_uv        = uv;
    vert_flags     = instance_flags;
    vert_thickness = instance_thickness;
    vert_softness  = instance_softness;
    vert_radies    = instance_radies;
    vert_center    = center;
    vert_half_size = half_size;
}
