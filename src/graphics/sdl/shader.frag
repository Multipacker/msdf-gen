#version 450

in vec4 vert_color;
in vec2 vert_uv;

out vec4 frag_color;

void main() {
    frag_color = vec4(vert_uv, 1, 1) * vert_color;
}
