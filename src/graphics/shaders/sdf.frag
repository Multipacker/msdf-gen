#version 450

#include "../sdf_types.h"
#include "../conversion_types.h"

layout (constant_id = 0) const uint OUTPUT_CONVERSION = VULKAN_CONVERSION_NONE;

layout(binding = 0) uniform UniformBufferObject {
    vec2 viewport_size;
    uint render_msdf;
} ubo;
layout(binding = 1) uniform sampler   samp;
layout(binding = 2) uniform texture2D textures[2];
layout(binding = 3) uniform texture2D msdf_texture;

layout(location = 0) in flat uint fragShapeType;
layout(location = 1) in flat vec2 fragPosition;
layout(location = 2) in flat vec2 fragSize;
layout(location = 3) in flat vec3 fragColor;
layout(location = 4) in flat uint fragTextureIndex;
layout(location = 5) in flat vec2 fragUV0;
layout(location = 6) in flat vec2 fragUV1;

layout(location = 0) out vec4 outColor;

float median_of_3(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

float sdf_box(vec2 position, vec2 box_position, vec2 box_size, out vec2 uv) {
    uv = (position - box_position) / box_size;
    uv = fragUV0 + uv * (fragUV1 - fragUV0);
    vec2 distance = abs(position - box_position - box_size / 2.0) - box_size / 2.0;
    return length(max(distance, 0.0)) + min(max(distance.x, distance.y), 0.0);
}

float sdf_circle(vec2 position, vec2 circle_position, float radius, out vec2 uv) {
    uv = (position - circle_position + radius) / (2.0 * radius);
    uv = fragUV0 + uv * (fragUV1 - fragUV0);
    return length(position - circle_position) - radius;
}

float sdf_line(vec2 position, vec2 p0, vec2 p1, float width) {
    vec2 position_p0 = position - p0, direction = p1 - p0;
    float t = clamp(dot(position_p0, direction) / dot(direction, direction), 0.0, 1.0);
    return length(position_p0 - direction * t) - width / 2.0;
}

float msdf_from_texture(vec2 position, vec2 msdf_position, vec2 msdf_size, out vec2 uv) {
    uv = (position - msdf_position) / msdf_size;
    uv = fragUV0 + uv * (fragUV1 - fragUV0);
    vec4 msdf_sample = texture(sampler2D(msdf_texture, samp), uv);
    float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5;
    return distance;
}

vec4 to_linear(vec4 sRGB) {
    bvec3 cutoff = lessThan(sRGB.rgb, vec3(0.04045));
    vec3  higher = pow((sRGB.rgb + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3  lower  = sRGB.rgb / vec3(12.92);
    return vec4(mix(higher, lower, cutoff), sRGB.a);
}

vec4 to_srgb(vec4 linearRGB) {
    bvec3 cutoff = lessThan(linearRGB.rgb, vec3(0.0031308));
    vec3  higher = vec3(1.055) * pow(linearRGB.rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3  lower  = linearRGB.rgb * vec3(12.92);
    return vec4(mix(higher, lower, cutoff), linearRGB.a);
}

void main() {
    vec2 uv = vec2(0.0, 0.0);
    float distance = 0.0;

    switch (fragShapeType) {
        case SDF_TYPE_RECTANGLE: {
            distance = -sdf_box(gl_FragCoord.xy, fragPosition, fragSize, uv);
        } break;
        case SDF_TYPE_CIRCLE: {
            distance = -sdf_circle(gl_FragCoord.xy, fragPosition, fragSize.x, uv);
        } break;
        case SDF_TYPE_LINE: {
            distance = -sdf_line(gl_FragCoord.xy, fragPosition, fragSize, 2.0);
        } break;
        case SDF_TYPE_MSDF: {
            distance = msdf_from_texture(gl_FragCoord.xy, fragPosition, fragSize, uv);
        } break;
    }
    float alpha = ubo.render_msdf != 0 ? clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0) : 1;

    vec3 color = ubo.render_msdf != 0 ? fragColor : texture(sampler2D(msdf_texture, samp), uv).rgb;
    if (fragTextureIndex != 0) {
        color *= texture(sampler2D(textures[fragTextureIndex - 1], samp), uv).rgb;
    }

    if (OUTPUT_CONVERSION == VULKAN_CONVERSION_NONE) {
        outColor = vec4(color, alpha);
    } else if (OUTPUT_CONVERSION == VULKAN_CONVERSION_LINEAR) {
        outColor = to_linear(vec4(color, alpha));
    } else if (OUTPUT_CONVERSION == VULKAN_CONVERSION_SRGB) {
        outColor = to_srgb(vec4(color, alpha));
    } else {
        outColor = vec4(color, alpha);
    }
}
