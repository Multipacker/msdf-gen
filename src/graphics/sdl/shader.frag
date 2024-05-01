#version 450 core

#define Render_RectangleFlags_Texture (1 << 0)
#define Render_RectangleFlags_MSDF    (1 << 1)

in      vec4 vert_color;
in      vec2 vert_uv;
in flat uint vert_flags;

out vec4 frag_color;

uniform mat4      uniform_projection;
uniform sampler2D uniform_sampler;

float median_of_3(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

void main() {
    vec4 texture_sample = vec4(1.0);
    float alpha = 1.0f;

    if ((vert_flags & Render_RectangleFlags_Texture) != 0) {
        texture_sample = vec4(texture(uniform_sampler, vert_uv).rgb, 1.0);
    } else if ((vert_flags & Render_RectangleFlags_MSDF) != 0) {
        vec4 msdf_sample = texture(uniform_sampler, vert_uv);
        float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5;

        alpha = clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0);
    }

    frag_color = texture_sample * vert_color * vec4(1.0, 1.0, 1.0, alpha);
}
