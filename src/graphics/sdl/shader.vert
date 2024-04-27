#version 450

layout (location = 0) in vec2 instance_min;
layout (location = 1) in vec2 instance_max;
layout (location = 2) in vec4 instance_color;

out vec4 vert_color;

uniform mat4 uniform_projection;

const vec2 verticies[] = {
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(-1.0, +1.0),
    vec2(+1.0, +1.0)
};

void main() {
    vec2 center    = 0.5 * (instance_max + instance_min);
    vec2 half_size = 0.5 * (instance_max - instance_min);
    vec2 position  = center + half_size * verticies[gl_VertexID];

    gl_Position = uniform_projection * vec4(position, 0.0, 1.0);
    vert_color  = instance_color;
}
