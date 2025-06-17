#ifndef D3D11_INCLUDE_H
#define D3D11_INCLUDE_H

#define COBJMACROS
#include <initguid.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <d3dcompiler.h>
#include <d3dcommon.h>

typedef struct D3D11_ConstantBuffer D3D11_ConstantBuffer;
struct D3D11_ConstantBuffer {
    V2F32 resolution;
    V2F32 padding;
    V4F32 transform[3];
};

typedef struct D3D11_Texture2D D3D11_Texture2D;
struct D3D11_Texture2D {
    D3D11_Texture2D *next;

    Render_TextureFormat      format;
    V2U32                     size;
    ID3D11Texture2D          *texture;
    ID3D11ShaderResourceView *shader_resource_view;
};

typedef struct D3D11_Window D3D11_Window;
struct D3D11_Window {
    D3D11_Window *next;

    V2U32                   resolution;
    IDXGISwapChain1        *swap_chain;
    ID3D11Texture2D        *framebuffer;
    ID3D11RenderTargetView *framebuffer_render_target_view;
};

typedef struct D3D11_State D3D11_State;
struct D3D11_State {
    Arena                *permanent_arena;
    ID3D11Device         *base_device;
    ID3D11DeviceContext  *base_device_context;
    ID3D11Device1        *device;
    ID3D11DeviceContext1 *device_context;
    IDXGIDevice1         *dxgi_device;
    IDXGIAdapter         *dxgi_adapter;
    IDXGIFactory2        *dxgi_factory;
    ID3D11SamplerState   *linear_sampler;
    ID3D11SamplerState   *samplers[Render_Filtering_COUNT];
    ID3D11Buffer         *constant_buffer;

    ID3D11InputLayout      *input_layout;
    ID3D11VertexShader     *vertex_shader;
    ID3D11RasterizerState1 *rasterizer_state;
    ID3D11BlendState       *blend_state;
    ID3D11PixelShader      *pixel_shader;

    // NOTE(simon): Resources
    D3D11_Texture2D *texture_freelist;
    D3D11_Window    *window_freelist;
};

#endif // D3D11_INCLUDE_H
