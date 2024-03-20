#version 450

#include "../sdf_types.h"

layout(binding = 0) uniform UniformBufferObject {
    vec2 viewport_size;
    uint render_msdf;
} ubo;

layout(location = 0) in uint inShapeType;
layout(location = 1) in vec2 inPosition;
layout(location = 2) in vec2 inSize;
layout(location = 3) in vec3 inColor;
layout(location = 4) in uint inTextureIndex;
layout(location = 5) in vec2 inUV0;
layout(location = 6) in vec2 inUV1;

layout(location = 0) out flat uint fragShapeType;
layout(location = 1) out flat vec2 fragPosition;
layout(location = 2) out flat vec2 fragSize;
layout(location = 3) out flat vec3 fragColor;
layout(location = 4) out flat uint fragTextureIndex;
layout(location = 5) out flat vec2 fragUV0;
layout(location = 6) out flat vec2 fragUV1;

vec2 verticies[] = vec2[](
    vec2(-1, -1),
    vec2( 1, -1),
    vec2(-1,  1),

    vec2( 1,  1),
    vec2(-1,  1),
    vec2( 1, -1)
);

// TODO: Compute better bounding boxes
void main() {
    vec2 mid_point = vec2(0.0, 0.0);
    vec2 half_size = vec2(0.0, 0.0);
    switch (inShapeType) {
        case SDF_TYPE_RECTANGLE: {
            mid_point = inPosition + inSize / 2.0;
            half_size = inSize / 2.0;
        } break;
        case SDF_TYPE_CIRCLE: {
            mid_point = inPosition;
            half_size = vec2(inSize.x);
        } break;
        case SDF_TYPE_LINE: {
            mid_point = (inPosition + inSize) / 2.0;
            half_size = abs(inPosition - inSize) / 2.0 + 5;
        } break;
        case SDF_TYPE_MSDF: {
            mid_point = inPosition + inSize / 2.0;
            half_size = inSize / 2.0;
        } break;
    }

    vec2 vertex = (mid_point + half_size * verticies[gl_VertexIndex]);

    gl_Position      = vec4(2.0 * vertex / ubo.viewport_size - 1.0, 0.0, 1.0);
    fragShapeType    = inShapeType;
    fragPosition     = inPosition;
    fragSize         = inSize;
    fragColor        = inColor;
    fragTextureIndex = inTextureIndex;
    fragUV0          = inUV0;
    fragUV1          = inUV1;
}
