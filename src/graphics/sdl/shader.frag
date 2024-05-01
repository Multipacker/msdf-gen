#version 450 core

#define Render_RectangleFlags_Texture (1 << 0)
#define Render_RectangleFlags_MSDF    (1 << 1)

in      vec4 vert_color;
in      vec2 vert_uv;
in flat uint vert_flags;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;

void main() {
    vec4 texture_sample = vec4(1.0);
    if ((vert_flags & Render_RectangleFlags_Texture) != 0) {
        texture_sample = vec4(texture(uniform_sampler, vert_uv).rgb, 1.0);
    } else if ((vert_flags & Render_RectangleFlags_MSDF) != 0) {
    }

    frag_color = texture_sample * vert_color;
}
