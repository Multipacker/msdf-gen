#define Render_ShapeFlag_Texture   uint(1 << 0)
#define Render_ShapeFlag_MSDF      uint(1 << 1)
#define Render_ShapeFlag_AlphaMask uint(1 << 2)
#define Render_ShapeFlag_Line      uint(1 << 3)

cbuffer Globals : register(b0) {
              float2   viewport_size;
    row_major float3x3 transform;
};

Texture2D    shader_texture : register(t0);
SamplerState shader_sampler : register(s0);

struct CPUToVertex {
    float4 position  : POS;
    float4 source    : TEX;
    float4 color00   : COLOR0;
    float4 color01   : COLOR1;
    float4 color10   : COLOR2;
    float4 color11   : COLOR3;
    float4 radies    : RADIES;
    float  thickness : THICK;
    float  softness  : SOFT;
    uint   flags     : FLAGS;
    uint   vertex_id : SV_VertexID;
};

struct VertexToPixel {
                    float4 position  : SV_POSITION;
                    float2 source    : TEX;
    nointerpolation float4 color00   : COLOR0;
    nointerpolation float4 color01   : COLOR1;
    nointerpolation float4 color10   : COLOR2;
    nointerpolation float4 color11   : COLOR3;
    nointerpolation float4 radies    : RADIES;
    nointerpolation float  thickness : THICK;
    nointerpolation float  softness  : SOFT;
    nointerpolation uint   flags     : FLAGS;
                    float2 uv        : UV;
                    float2 pos       : POSITION;
    nointerpolation float2 half_size : SIZE;
};

float4 linear_from_srgb(float4 srgb) {
    float3 higher = pow((srgb.rgb + 0.055f) / 1.055f, 2.4f);
    float3 lower  = srgb.rgb / 12.92f;
    float4 rgb    = float4(lerp(higher, lower, srgb.rgb < 0.04045f), srgb.a);
    return rgb;
}

float4 srgb_from_linear(float4 rgb) {
    float3 lower  = 12.92f * rgb.rgb;
    float3 higher = 1.055f * pow(rgb.rgb, 1.0f / 2.4f) - 0.055f;
    float4 srgb   = float4(lerp(higher, lower, rgb.rgb < 0.0031308f), rgb.a);
    return srgb;
}

float sdf_box(float2 position, float2 half_size) {
    float2 distance = abs(position) - half_size;
    return length(max(0.0f, distance)) + min(max(distance.x, distance.y), 0.0f);
}

float median_of_3(float a, float b, float c) {
    return max(min(a, b), min(max(a, b), c));
}

VertexToPixel vertex_main(CPUToVertex cpu_to_vertex) {
    float2 position_p0 = cpu_to_vertex.position.xy;
    float2 position_p1 = cpu_to_vertex.position.zw;

    // NOTE(simon): Fill shape specific fields.
    float2 half_size;
    float2x2 local_transform;
    if (cpu_to_vertex.flags & Render_ShapeFlag_Line) {
        // NOTE(simon): Line.
        float radius = cpu_to_vertex.radies[0];

        float2 delta     = position_p1 - position_p0;
        float  size      = length(delta);
        float2 direction = delta / size;

        half_size = float2(0.5f * size + radius, radius);
        local_transform = float2x2(
            direction.x, -direction.y,
            direction.y,  direction.x
        );
    } else {
        // NOTE(simon): Rectangle.
        half_size = 0.5f * (position_p1 - position_p0);
        local_transform = float2x2(
            1.0f, 0.0f,
            0.0f, 1.0f
        );
    }

    float2 verticies[] = {
        float2(-half_size.x, -half_size.y),
        float2( half_size.x, -half_size.y),
        float2(-half_size.x,  half_size.y),
        float2( half_size.x,  half_size.y)
    };

    float2 source[] = {
        float2(cpu_to_vertex.source.x, cpu_to_vertex.source.y),
        float2(cpu_to_vertex.source.z, cpu_to_vertex.source.y),
        float2(cpu_to_vertex.source.x, cpu_to_vertex.source.w),
        float2(cpu_to_vertex.source.z, cpu_to_vertex.source.w),
    };

    float2 local_position = verticies[cpu_to_vertex.vertex_id];
    float2 center         = 0.5f * (position_p1 + position_p0);

    float2 transformed_position = mul(transform, float3(center + mul(local_transform, local_position), 1.0f)).xy;

    VertexToPixel vertex_to_pixel;
    vertex_to_pixel.position.x = 2.0f * transformed_position.x / viewport_size.x - 1.0f;
    vertex_to_pixel.position.y = 1.0f - 2.0f * transformed_position.y / viewport_size.y;
    vertex_to_pixel.position.z = 0.0f;
    vertex_to_pixel.position.w = 1.0f;
    vertex_to_pixel.source     = source[cpu_to_vertex.vertex_id];
    vertex_to_pixel.color00    = cpu_to_vertex.color00;
    vertex_to_pixel.color01    = cpu_to_vertex.color01;
    vertex_to_pixel.color10    = cpu_to_vertex.color10;
    vertex_to_pixel.color11    = cpu_to_vertex.color11;
    vertex_to_pixel.radies     = cpu_to_vertex.radies;
    vertex_to_pixel.thickness  = cpu_to_vertex.thickness;
    vertex_to_pixel.softness   = cpu_to_vertex.softness;
    vertex_to_pixel.flags      = cpu_to_vertex.flags;
    vertex_to_pixel.uv         = float2(cpu_to_vertex.vertex_id & 1, cpu_to_vertex.vertex_id >> 1);
    vertex_to_pixel.pos        = local_position;
    vertex_to_pixel.half_size  = half_size;

    return vertex_to_pixel;
}

float4 pixel_main(VertexToPixel vertex_to_pixel) : SV_TARGET {
    float4 texture_sample = 1.0f;
    float  alpha          = 1.0f;

    if (vertex_to_pixel.flags & Render_ShapeFlag_Texture) {
        float2 texture_size;
        shader_texture.GetDimensions(texture_size.x, texture_size.y);
        texture_sample.xyz = shader_texture.Sample(shader_sampler, vertex_to_pixel.source / texture_size).xyz;
    }

    if (vertex_to_pixel.flags & Render_ShapeFlag_MSDF) {
        float2 texture_size;
        shader_texture.GetDimensions(texture_size.x, texture_size.y);
        float4 msdf_sample = shader_texture.Sample(shader_sampler, vertex_to_pixel.source / texture_size);
        float distance = median_of_3(msdf_sample.r, msdf_sample.g, msdf_sample.b) - 0.5f;
        alpha *= clamp(distance / fwidth(distance) + 0.5, 0.0, 1.0);
    } else if (vertex_to_pixel.flags & Render_ShapeFlag_AlphaMask) {
        float2 texture_size;
        shader_texture.GetDimensions(texture_size.x, texture_size.y);
        alpha = shader_texture.Sample(shader_sampler, vertex_to_pixel.source / texture_size).r;
    } else {
        int   corner_index = (0.5f * sign(vertex_to_pixel.pos.x) + sign(vertex_to_pixel.pos.y) + 1.5f);
        float outer_radius = vertex_to_pixel.radies[corner_index];
        float inner_radius = outer_radius - vertex_to_pixel.thickness;

        if (vertex_to_pixel.thickness > 0.0f) {
            float inner_distance = sdf_box(vertex_to_pixel.pos, vertex_to_pixel.half_size - inner_radius - vertex_to_pixel.thickness - 2.0f * vertex_to_pixel.softness) - inner_radius;
            alpha *= smoothstep(0, 2.0f * vertex_to_pixel.softness, inner_distance);
        }

        if (outer_radius > 0.0f || vertex_to_pixel.softness > 0.75f) {
            float outer_distance = sdf_box(vertex_to_pixel.pos, vertex_to_pixel.half_size - outer_radius - 2.0f * vertex_to_pixel.softness) - outer_radius;
            alpha *= 1.0f - smoothstep(0.0f, 2.0f * vertex_to_pixel.softness, outer_distance);
        }
    }

    float4 color = linear_from_srgb(lerp(
        lerp(srgb_from_linear(vertex_to_pixel.color00), srgb_from_linear(vertex_to_pixel.color01), vertex_to_pixel.uv.x),
        lerp(srgb_from_linear(vertex_to_pixel.color10), srgb_from_linear(vertex_to_pixel.color11), vertex_to_pixel.uv.x),
        vertex_to_pixel.uv.y
    ));

    return texture_sample * color * float4(1.0f, 1.0f, 1.0f, alpha);
}
