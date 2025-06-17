#include "generated.h"

global D3D11_State global_d3d11_state;

embed_file(d3d11_shader_source, "shader.hlsl");



// NOTE(simon): Internal helpers.
internal D3D11_Texture2D *d3d11_texture_from_handle(Render_Texture handle) {
    U64 pointer = (U64) handle.u32[1] << 32 | (U64) handle.u32[0];
    D3D11_Texture2D *texture = (D3D11_Texture2D *) pointer_from_integer(pointer);
    return texture;
}

internal Render_Texture d3d11_handle_from_texture(D3D11_Texture2D *texture) {
    U64 pointer = integer_from_pointer(texture);
    Render_Texture result = { 0 };
    result.u32[0] = (pointer >>  0) & 0xFFFFFFFF;
    result.u32[1] = (pointer >> 32) & 0xFFFFFFFF;
    return result;
}

internal D3D11_Window *d3d11_window_from_handle(Render_Window handle) {
    D3D11_Window *result = (D3D11_Window *) pointer_from_integer(handle.u64[0]);
    return result;
}

internal Render_Window d3d11_handle_from_window(D3D11_Window *window) {
    Render_Window result = { 0 };
    result.u64[0] = integer_from_pointer(window);
    return result;
}



internal Render_Texture render_texture_null(Void) {
    Render_Texture result = { 0 };
    return result;
}

internal B32 render_texture_equal(Render_Texture a, Render_Texture b) {
    D3D11_Texture2D *texture_a = d3d11_texture_from_handle(a);
    D3D11_Texture2D *texture_b = d3d11_texture_from_handle(b);
    B32 result = texture_a == texture_b;
    return result;
}

internal Render_Texture render_texture_create(V2U32 size, Render_TextureFormat format, U8 *data) {
    D3D11_State *state = &global_d3d11_state;

    D3D11_Texture2D *texture = state->texture_freelist;
    if (texture) {
        sll_stack_pop(state->texture_freelist);
        memory_zero_struct(texture);
    } else {
        texture = arena_push_struct(state->permanent_arena, D3D11_Texture2D);
    }

    DXGI_FORMAT dxgi_format = DXGI_FORMAT_UNKNOWN;
    switch (format) {
        case Render_TextureFormat_R8: {
            dxgi_format = DXGI_FORMAT_R8_UNORM;
        } break;
        case Render_TextureFormat_RGBA8: {
            dxgi_format = DXGI_FORMAT_R8G8B8A8_UNORM;
        } break;
    }

    D3D11_TEXTURE2D_DESC description = { 0 };
    description.Width              = size.width;
    description.Height             = size.height;
    description.MipLevels          = 1;
    description.ArraySize          = 1;
    description.Format             = dxgi_format;
    description.SampleDesc.Count   = 1;
    description.SampleDesc.Quality = 0;
    description.Usage              = D3D11_USAGE_DEFAULT;
    description.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
    description.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA initial_data = { 0 };
    D3D11_SUBRESOURCE_DATA *initial_data_pointer = 0;
    if (data) {
        initial_data_pointer = &initial_data;
        initial_data.pSysMem = data;
        switch (format) {
            case Render_TextureFormat_R8: {
                initial_data.SysMemPitch = 1 * size.width;
            } break;
            case Render_TextureFormat_RGBA8: {
                initial_data.SysMemPitch = 4 * size.width;
            } break;
        }
    }

    ID3D11Device_CreateTexture2D(state->device, &description, initial_data_pointer, &texture->texture);

    ID3D11ShaderResourceView *texture_shader_resource_view = 0;
    ID3D11Device_CreateShaderResourceView(state->device, (ID3D11Resource *) texture->texture, 0, &texture->shader_resource_view);

    texture->format = format;
    texture->size   = size;

    Render_Texture result = d3d11_handle_from_texture(texture);
    return result;
}

internal Void render_texture_destroy(Render_Texture handle) {
    D3D11_State     *state   = &global_d3d11_state;
    D3D11_Texture2D *texture = d3d11_texture_from_handle(handle);

    if (texture) {
        ID3D11ShaderResourceView_Release(texture->shader_resource_view);
        ID3D11Texture2D_Release(texture->texture);
        sll_stack_push(state->texture_freelist, texture);
    }
}

internal V2U32 render_size_from_texture(Render_Texture handle) {
    D3D11_Texture2D *texture = d3d11_texture_from_handle(handle);
    V2U32 result = texture->size;
    return result;
}

internal Void render_texture_update(Render_Texture handle, V2U32 position, V2U32 size, U8 *data) {
    D3D11_State *state = &global_d3d11_state;

    D3D11_Texture2D *texture = d3d11_texture_from_handle(handle);
    if (texture) {
        D3D11_BOX destination = { 0 };
        destination.left   = position.x;
        destination.top    = position.y;
        destination.front  = 0;
        destination.right  = position.x + size.width;
        destination.bottom = position.y + size.height;
        destination.back   = 1;

        U64 pitch = 0;
        switch (texture->format) {
            case Render_TextureFormat_R8: {
                pitch = 1 * size.width;
            } break;
            case Render_TextureFormat_RGBA8: {
                pitch = 4 * size.width;
            } break;
        }

        ID3D11DeviceContext_UpdateSubresource(
            state->device_context,
            (ID3D11Resource *) texture->texture,
            0,
            &destination,
            data,
            pitch,
            0
        );
    }
}

internal B32 render_init(Void) {
    D3D11_State *state = &global_d3d11_state;

    state->permanent_arena = arena_create();

    // NOTE(simon): Create device and device context.
    UINT device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if DEBUG_BUILD
    device_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL feature_levels[] = {
        D3D_FEATURE_LEVEL_11_0,
    };
    HRESULT error = D3D11CreateDevice(
        0,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        device_flags,
        feature_levels,
        array_count(feature_levels),
        D3D11_SDK_VERSION,
        &state->base_device,
        0,
        &state->base_device_context
    );

    if (FAILED(error)) {
        // TODO(simon): Inform the user that we could not create a D3D11 device.
        os_exit(1);
    }

    ID3D11Device_QueryInterface(state->base_device, &IID_ID3D11Device1, (Void **) &state->device);
    ID3D11DeviceContext_QueryInterface(state->base_device_context, &IID_ID3D11DeviceContext1, (Void **) &state->device_context);

    ID3D11Device_QueryInterface(state->device, &IID_IDXGIDevice1, &state->dxgi_device);
    IDXGIDevice_GetAdapter(state->dxgi_device, &state->dxgi_adapter);
    IDXGIAdapter_GetParent(state->dxgi_adapter, &IID_IDXGIFactory2, &state->dxgi_factory);

    // NOTE(simon): Create rasterizer.
    D3D11_RASTERIZER_DESC1 rasterizer_description = { 0 };
    rasterizer_description.FillMode = D3D11_FILL_SOLID;
    rasterizer_description.CullMode = D3D11_CULL_NONE;
    rasterizer_description.ScissorEnable = true;
    ID3D11Device1_CreateRasterizerState1(state->device, &rasterizer_description, &state->rasterizer_state);

    // NOTE(simon): Create blend state.
    D3D11_BLEND_DESC blend_description = { 0 };
    blend_description.RenderTarget[0].BlendEnable           = true;
    blend_description.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    blend_description.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    blend_description.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blend_description.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_ONE;
    blend_description.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_ZERO;
    blend_description.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blend_description.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11Device1_CreateBlendState(state->device, &blend_description, &state->blend_state);

    // NOTE(simon): Create nearest-neighbour sampler.
    D3D11_SAMPLER_DESC nearest_sampler_description = { 0 };
    nearest_sampler_description.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
    nearest_sampler_description.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    nearest_sampler_description.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    nearest_sampler_description.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    nearest_sampler_description.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ID3D11Device_CreateSamplerState(state->device, &nearest_sampler_description, &state->samplers[Render_Filtering_Nearest]);

    // NOTE(simon): Create bilinear sampler.
    D3D11_SAMPLER_DESC linear_sampler_description = { 0 };
    linear_sampler_description.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    linear_sampler_description.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
    linear_sampler_description.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
    linear_sampler_description.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
    linear_sampler_description.ComparisonFunc = D3D11_COMPARISON_NEVER;
    ID3D11Device_CreateSamplerState(state->device, &linear_sampler_description, &state->samplers[Render_Filtering_Linear]);

    // NOTE(simon): Create constant buffer.
    D3D11_BUFFER_DESC constant_buffer_description = { 0 };
    constant_buffer_description.ByteWidth      = u64_round_up_to_power_of_2(sizeof(D3D11_ConstantBuffer), 16);
    constant_buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
    constant_buffer_description.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
    constant_buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Device_CreateBuffer(state->device, &constant_buffer_description, 0, &state->constant_buffer);

    // NOTE(simon): Create vertex shader.
    ID3DBlob *vertex_blob = 0;
    ID3DBlob *vertex_error_blob = 0;
    error = D3DCompile(
        d3d11_shader_source.data, d3d11_shader_source.size,
        0,
        0,
        0,
        "vertex_main",
        "vs_5_0",
        0,
        0,
        &vertex_blob,
        &vertex_error_blob
    );
    if (FAILED(error)) {
        Str8 errors = str8(ID3D10Blob_GetBufferPointer(vertex_error_blob), ID3D10Blob_GetBufferSize(vertex_error_blob));
        os_console_print(errors);
    }
    ID3D11Device_CreateVertexShader(state->device, ID3D10Blob_GetBufferPointer(vertex_blob), ID3D10Blob_GetBufferSize(vertex_blob), 0, &state->vertex_shader);

    // NOTE(simon): Create input layout.
    D3D11_INPUT_ELEMENT_DESC input_element_description[] = {
        { "POS",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0,                            D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "TEX",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "COLOR",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "COLOR",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "COLOR",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "COLOR",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "RADIES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "THICK",  0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "SOFT",   0, DXGI_FORMAT_R32_FLOAT,          0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
        { "FLAGS",  0, DXGI_FORMAT_R32_UINT,           0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1, },
    };
    ID3D11Device_CreateInputLayout(
        state->device,
        input_element_description, array_count(input_element_description),
        ID3D10Blob_GetBufferPointer(vertex_blob), ID3D10Blob_GetBufferSize(vertex_blob),
        &state->input_layout
    );

    // NOTE(simon): Create pixel shader.
    ID3DBlob *pixel_blob = 0;
    ID3DBlob *pixel_error_blob = 0;
    error = D3DCompile(
        d3d11_shader_source.data, d3d11_shader_source.size,
        0,
        0,
        0,
        "pixel_main",
        "ps_5_0",
        0,
        0,
        &pixel_blob,
        &pixel_error_blob
    );

    if (FAILED(error)) {
        Str8 errors = str8(ID3D10Blob_GetBufferPointer(pixel_error_blob), ID3D10Blob_GetBufferSize(pixel_error_blob));
        os_console_print(errors);
    }

    ID3D11Device_CreatePixelShader(state->device, ID3D10Blob_GetBufferPointer(pixel_blob), ID3D10Blob_GetBufferSize(pixel_blob), 0, &state->pixel_shader);

    return false;
}

internal Void render_begin(Void) {
}

internal Void render_end(Void) {
}



internal Render_Window render_create(Gfx_Window graphics_handle) {
    D3D11_State *d3d11_state = &global_d3d11_state;
    Gfx_Win32State *gfx_state = &gfx_win32_state;

    Gfx_Win32Window *graphics_window = win32_window_from_handle(graphics_handle);
    HWND hwnd = graphics_window->hwnd;

    D3D11_Window *render_window = d3d11_state->window_freelist;
    if (render_window) {
        sll_stack_pop(d3d11_state->window_freelist);
        memory_zero_struct(render_window);
    } else {
        render_window = arena_push_struct(d3d11_state->permanent_arena, D3D11_Window);
    }

    // NOTE(simon): Create swap chain.
    DXGI_SWAP_CHAIN_DESC1 swap_chain_description = { 0 };
    swap_chain_description.Width              = 0; // NOTE(simon): Use the window width.
    swap_chain_description.Height             = 0; // NOTE(simon): Use the window height.
    swap_chain_description.Format             = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    swap_chain_description.Stereo             = false;
    swap_chain_description.SampleDesc.Count   = 1;
    swap_chain_description.SampleDesc.Quality = 0;
    swap_chain_description.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_chain_description.BufferCount        = 2;
    swap_chain_description.Scaling            = DXGI_SCALING_STRETCH;
    swap_chain_description.SwapEffect         = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_description.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED; // TODO(simon): Maybe use DXGI_ALPHA_MODE_IGNORE
    swap_chain_description.Flags              = 0;
    HRESULT error = IDXGIFactory2_CreateSwapChainForHwnd(
        d3d11_state->dxgi_factory,
        (IUnknown *) d3d11_state->device,
        hwnd,
        &swap_chain_description,
        0,
        0,
        &render_window->swap_chain
    );

    if (FAILED(error)) {
        // TODO(simon): Inform the user that we could not create a D3D11 swap chain.
        os_exit(1);
    }

    IDXGISwapChain_GetBuffer(render_window->swap_chain, 0, &IID_ID3D11Texture2D, (Void **) &render_window->framebuffer);
    ID3D11Device_CreateRenderTargetView(d3d11_state->device, (ID3D11Resource *) render_window->framebuffer, 0, &render_window->framebuffer_render_target_view);

    Render_Window result = d3d11_handle_from_window(render_window);
    return result;
}

internal Void render_destroy(Gfx_Window graphics_handle, Render_Window render_handle) {
    D3D11_State *state = &global_d3d11_state;
    D3D11_Window *render_window = d3d11_window_from_handle(render_handle);
    ID3D11RenderTargetView_Release(render_window->framebuffer_render_target_view);
    ID3D11Texture2D_Release(render_window->framebuffer);
    IDXGISwapChain1_Release(render_window->swap_chain);
    sll_stack_push(state->window_freelist, render_window);
}

internal Void render_window_begin(Gfx_Window graphics_handle, Render_Window render_handle) {
    D3D11_State *state = &global_d3d11_state;
    D3D11_Window *render_window = d3d11_window_from_handle(render_handle);

    V2U32 resolution = gfx_client_area_from_window(graphics_handle);

    // NOTE(simon): Resolution change.
    if (resolution.x != render_window->resolution.x || resolution.y != render_window->resolution.y) {
        render_window->resolution = resolution;

        // NOTE(simon): Resize swap chain and framebuffer.
        ID3D11RenderTargetView_Release(render_window->framebuffer_render_target_view);
        ID3D11Texture2D_Release(render_window->framebuffer);
        IDXGISwapChain_ResizeBuffers(render_window->swap_chain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        IDXGISwapChain_GetBuffer(render_window->swap_chain, 0, &IID_ID3D11Texture2D, (Void **) &render_window->framebuffer);
        ID3D11Device_CreateRenderTargetView(state->device, (ID3D11Resource *) render_window->framebuffer, 0, &render_window->framebuffer_render_target_view);
    }

    // NOTE(simon): Clear framebuffer.
    V4F32 clear_color = v4f32(0, 0, 0, 0);
    ID3D11DeviceContext_ClearRenderTargetView(state->device_context, render_window->framebuffer_render_target_view, clear_color.values);

    // NOTE(simon): Set up graphics pipeline.
    ID3D11DeviceContext_IASetPrimitiveTopology(state->device_context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11DeviceContext_IASetInputLayout(state->device_context, state->input_layout);

    ID3D11DeviceContext_VSSetShader(state->device_context, state->vertex_shader, 0, 0);

    D3D11_VIEWPORT viewport = { 0 };
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = resolution.x;
    viewport.Height   = resolution.y;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    ID3D11DeviceContext_RSSetViewports(state->device_context, 1, &viewport);
    ID3D11DeviceContext_RSSetState(state->device_context, (ID3D11RasterizerState *) state->rasterizer_state);

    ID3D11DeviceContext_PSSetShader(state->device_context, state->pixel_shader, 0, 0);

    ID3D11DeviceContext_OMSetRenderTargets(state->device_context, 1, &render_window->framebuffer_render_target_view, 0);
    ID3D11DeviceContext_OMSetBlendState(state->device_context, state->blend_state, 0, 0xFFFFFFFF);
}

internal Void render_window_submit(Gfx_Window graphics_handle, Render_Window render_handle, Render_BatchList batches) {
    D3D11_State *state = &global_d3d11_state;
    D3D11_Window *render_window = d3d11_window_from_handle(render_handle);

    U64 shape_count = 0;
    for (Render_Batch *batch = batches.first; batch; batch = batch->next) {
        shape_count += batch->shapes.shape_count;
    }

    U64 byte_size = shape_count * sizeof(Render_Shape);

    // NOTE(simon): Upload data.
    ID3D11Buffer *vertex_buffer = 0;
    {
        D3D11_BUFFER_DESC buffer_description = { 0 };
        buffer_description.ByteWidth      = byte_size;
        buffer_description.Usage          = D3D11_USAGE_DYNAMIC;
        buffer_description.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
        buffer_description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ID3D11Device_CreateBuffer(state->device, &buffer_description, 0, &vertex_buffer);

        D3D11_MAPPED_SUBRESOURCE mapped_vertex_buffer = { 0 };
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *) vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_vertex_buffer);
        U8 *ptr = mapped_vertex_buffer.pData;
        for (Render_Batch *batch = batches.first; batch; batch = batch->next) {
            for (Render_ShapeChunk *chunk = batch->shapes.first; chunk; chunk = chunk->next) {
                memory_copy(ptr, chunk->shapes, chunk->count * sizeof(Render_Shape));
                ptr += chunk->count * sizeof(Render_Shape);
            }
        }
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *) vertex_buffer, 0);

        UINT stride = sizeof(Render_Shape);
        UINT offset = 0;
        ID3D11DeviceContext_IASetVertexBuffers(state->device_context, 0, 1, &vertex_buffer, &stride, &offset);
    }

    U64 shape_offset = 0;
    for (Render_Batch *batch = batches.first; batch; batch = batch->next) {
        // NOTE(simon): Set constant buffer.
        D3D11_ConstantBuffer constant_data = { 0 };
        constant_data.resolution = v2f32(render_window->resolution.x, render_window->resolution.y);
        constant_data.transform[0] = v4f32(batch->transform.m[0][0], batch->transform.m[0][1], batch->transform.m[0][2], 0);
        constant_data.transform[1] = v4f32(batch->transform.m[1][0], batch->transform.m[1][1], batch->transform.m[1][2], 0);
        constant_data.transform[2] = v4f32(batch->transform.m[2][0], batch->transform.m[2][1], batch->transform.m[2][2], 0);

        // NOTE(simon): Upload constant buffer.
        D3D11_MAPPED_SUBRESOURCE mapped_constant_data = { 0 };
        ID3D11DeviceContext_Map(state->device_context, (ID3D11Resource *) state->constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_constant_data);
        memory_copy(mapped_constant_data.pData, &constant_data, sizeof(constant_data));
        ID3D11DeviceContext_Unmap(state->device_context, (ID3D11Resource *) state->constant_buffer, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(state->device_context, 0, 1, &state->constant_buffer);

        // NOTE(simon): Set clip rectangle.
        D3D11_RECT scissor = { 0 };
        scissor.left   = batch->clip.min.x;
        scissor.top    = batch->clip.min.y;
        scissor.right  = batch->clip.max.x;
        scissor.bottom = batch->clip.max.y;
        ID3D11DeviceContext_RSSetScissorRects(state->device_context, 1, &scissor);

        // NOTE(simon): Set sampler.
        ID3D11DeviceContext_PSSetSamplers(state->device_context, 0, 1, &state->samplers[batch->filtering]);

        // NOTE(simon): Set texture.
        D3D11_Texture2D *texture = d3d11_texture_from_handle(batch->texture);
        if (texture) {
            ID3D11DeviceContext_PSSetShaderResources(state->device_context, 0, 1, &texture->shader_resource_view);
        }

        // NOTE(simon): Draw.
        ID3D11DeviceContext_DrawInstanced(state->device_context, 4, batch->shapes.shape_count, 0, shape_offset);

        shape_offset += batch->shapes.shape_count;
    }

    ID3D11Buffer_Release(vertex_buffer);
}

internal Void render_window_end(Gfx_Window graphics_handle, Render_Window render_handle) {
    D3D11_State *state = &global_d3d11_state;
    D3D11_Window *render_window = d3d11_window_from_handle(render_handle);

    HRESULT error = IDXGISwapChain_Present(render_window->swap_chain, 1, 0);
    if (FAILED(error)) {
        // TODO(simon): Inform the user that we could not present the swap chain.
        os_exit(1);
    }
}

internal Render_Stats render_get_stats(Void) {
    Render_Stats result = { 0 };
    return result;
}
