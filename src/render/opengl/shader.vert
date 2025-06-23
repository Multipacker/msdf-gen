#version 330 core

#define Render_ShapeFlag_Texture   uint(1 << 0)
#define Render_ShapeFlag_MSDF      uint(1 << 1)
#define Render_ShapeFlag_AlphaMask uint(1 << 2)
#define Render_ShapeFlag_Line      uint(1 << 3)

layout(location = 0) in vec4  instance_position;
layout(location = 1) in vec4  instance_source;
layout(location = 2) in mat4  instance_colors;
layout(location = 6) in vec4  instance_radies;
layout(location = 7) in float instance_thickness;
layout(location = 8) in float instance_softness;
layout(location = 9) in uint  instance_flags;

flat out mat4  vert_colors;
     out vec2  vert_source;
flat out uint  vert_flags;
flat out float vert_thickness;
flat out float vert_softness;
flat out vec4  vert_radies;
     out vec2  vert_position;
flat out vec2  vert_half_size;
     out vec2  vert_uv;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;
uniform mat3      uniform_transform;

const vec2 verticies[] = vec2[](
    vec2(-1.0, -1.0),
    vec2(+1.0, -1.0),
    vec2(-1.0, +1.0),
    vec2(+1.0, +1.0)
);

void main() {
    vec2 position = vec2(0);
    vec2 half_size = vec2(0);

    if ((instance_flags & Render_ShapeFlag_Line) != 0u) {
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

    vec2 source_min = instance_source.xy;
    vec2 source_max = instance_source.zw;

    vec2 source_center    = 0.5 * (source_max + source_min);
    vec2 source_half_size = 0.5 * (source_max - source_min);
    vec2 source           = source_center + source_half_size * verticies[gl_VertexID];

    vec2 transformed_position = (uniform_transform * vec3(position, 1.0f)).xy;

    gl_Position    = uniform_projection * vec4(transformed_position, 0.0, 1.0);
    vert_colors    = instance_colors;
    vert_source    = source;
    vert_flags     = instance_flags;
    vert_thickness = instance_thickness;
    vert_softness  = instance_softness;
    vert_radies    = instance_radies;
    vert_position  = half_size * verticies[gl_VertexID];
    vert_half_size = half_size;
    vert_uv        = vec2(gl_VertexID & 1, gl_VertexID >> 1);
}
