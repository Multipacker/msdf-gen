#version 450

in      vec4 vert_color;
in      vec2 vert_uv;
in flat int  vert_texture_id;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;

void main() {
    vec4 texture_sample = vec4(1.0);
    if (vert_texture_id != 0) {
        texture_sample = texture(uniform_sampler, vert_uv);
    }

    frag_color = texture_sample * vert_color;
}
