#version 450 core

#define Render_ShapeFlag_Texture   uint(1 << 0)
#define Render_ShapeFlag_MSDF      uint(1 << 1)
#define Render_ShapeFlag_AlphaMask uint(1 << 2)
#define Render_ShapeFlag_Line      uint(1 << 3)

layout(location = 0) in vec4  instance_position;
layout(location = 1) in mat4  instance_colors;
layout(location = 5) in vec4  instance_uvs;
layout(location = 6) in uint  instance_flags;
layout(location = 7) in float instance_thickness;
layout(location = 8) in float instance_softness;
layout(location = 9) in vec4  instance_radies;

out      vec4  vert_color;
out      vec2  vert_uv;
out flat uint  vert_flags;
out flat float vert_thickness;
out flat float vert_softness;
out      vec4  vert_radies;
out      vec2  vert_position;
out flat vec2  vert_half_size;

uniform mat4      uniform_projection;
uniform sampler2D uniform_samplers[2];

const vec2 verticies[] = {
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(-1.0, +1.0),
    vec2(+1.0, +1.0)
};

void main() {
    vec2 position = vec2(0);
    vec2 half_size = vec2(0);

    if ((instance_flags & Render_ShapeFlag_Line) != 0) {
        vec2  position_p0 = instance_position.xy;
        vec2  position_p1 = instance_position.zw;
        float radius      = instance_radies[0];

        vec2 center    = 0.5 * (position_p0 + position_p1);
        vec2 line      = position_p1 - position_p0;
        vec2 direction = normalize(line);
        mat2 rotation  = mat2(
            direction.x,  direction.y,
            direction.y, -direction.x
        );

        half_size = vec2(length(line) * 0.5 + radius, radius);
        position  = center + rotation * (half_size * verticies[gl_VertexID]);
    } else {
        vec2 position_min = instance_position.xy;
        vec2 position_max = instance_position.zw;

        vec2 center = 0.5 * (position_max + position_min);

        half_size = 0.5 * (position_max - position_min);
        position  = center + half_size * verticies[gl_VertexID];
    }

    vec2 uv_min = instance_uvs.xy;
    vec2 uv_max = instance_uvs.zw;

    vec2 uv_center    = 0.5 * (uv_max + uv_min);
    vec2 uv_half_size = 0.5 * (uv_max - uv_min);
    vec2 uv           = uv_center + uv_half_size * verticies[gl_VertexID];

    gl_Position    = uniform_projection * vec4(position, 0.0, 1.0);
    vert_color     = instance_colors[gl_VertexID];
    vert_uv        = uv;
    vert_flags     = instance_flags;
    vert_thickness = instance_thickness;
    vert_softness  = instance_softness;
    vert_radies    = instance_radies;
    vert_position  = half_size * verticies[gl_VertexID];
    vert_half_size = half_size;
}
