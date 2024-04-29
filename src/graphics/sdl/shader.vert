#version 450

layout (location = 0) in vec2 instance_min;
layout (location = 1) in vec2 instance_max;
layout (location = 2) in vec4 instance_color;
layout (location = 3) in vec2 instance_uv_min;
layout (location = 4) in vec2 instance_uv_max;
layout (location = 5) in int  instance_texture_id;

out      vec4 vert_color;
out      vec2 vert_uv;
out flat int  vert_texture_id;

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

    gl_Position     = uniform_projection * vec4(position, 0.0, 1.0);
    vert_color      = instance_color;
    vert_uv         = uv;
    vert_texture_id = instance_texture_id;
}
