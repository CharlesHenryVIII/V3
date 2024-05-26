#include "Rendering.h"
#include "Debug.h"
#include "WinInterop_File.h"
#include "Vox.h"
#include "imgui.h"
#include "ImGui/backends/imgui_impl_sdl2.h"
#include "ImGui/backends/imgui_impl_dx12.h"
#include "stb/stb_image.h"

#include "SDL.h"
#include "SDL_syswm.h"
#include "Tracy.hpp"

// DirectX
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3dx12.h>
#include <dxgidebug.h>
//#ifdef _MSC_VER
//#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
//#endif

#if RENDER_PIPELINE == RENDER_PIPELINE_DX12

#define FRAME_TARGET_COUNT 2

Renderer g_renderer;

struct SwapChain {
    IDXGISwapChain3*        handle              = nullptr;
    //ID3D12RenderTargetView* render_target_view  = nullptr;

    Vec2I   size;
    u32     refresh_rate;
    u32     sample_count;
    u32     sample_quality;
};

enum class PipelineState : u32 {
    Cube_Full,
    Count,
};
ENUMOPS(PipelineState)

struct DX12Data {
    ID3D12Device*           device;
    //ID3D12DeviceContext*    device_context;
    IDXGIFactory*           factory;
    ID3D12CommandAllocator* command_allocator;
    ID3D12RootSignature*    root_signature;
    ID3D12PipelineState*    pipeline_state[+PipelineState::Count];
    ID3D12GraphicsCommandList* command_list;
    ID3D12CommandQueue*     command_queue;

    ID3D12DescriptorHeap*       rtv_heap;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;

    //Syncronization objects
    ID3D12Fence*    fence;
    UINT64          fence_value;
    HANDLE          fence_event;
    //UINT            frame_index;

    D3D12_VIEWPORT          root_signature;

    SwapChain               swap_chain;

    //ID3D12BlendState*       blend_state;
    //ID3D12RasterizerState*  rasterizer_full;
    //ID3D12RasterizerState*  rasterizer_wireframe;
    //ID3D12RasterizerState*  rasterizer_voxel;
    //ID3D12DepthStencilState* depth_stencil_state_depth      = nullptr;
    //ID3D12DepthStencilState* depth_stencil_state_no_depth   = nullptr;

    //ID3D12RenderTargetView* hdr_rtv = nullptr;
    ID3D12Resource*         render_targets[FRAME_TARGET_COUNT];

    HRESULT(*D3DCompileFunc)        (LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    HRESULT(*D3DCompileFromFileFunc)(LPCWSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
};
static DX12Data s_dx12 = {};

template <typename T>
void SafeRelease(T*& unknown)
{
    if (unknown)
    {
        unknown->Release();
        unknown = nullptr;
    }
}

extern "C" {
#ifdef _MSC_VER
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#else
    __attribute__((dllexport)) uint32_t NvOptimusEnablement = 0x00000001;
    __attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
}

#if _DEBUG
    
    #ifndef HR
        #define HR(x)                                       \
        {                                                   \
            HRESULT hresult = x;                            \
            if(FAILED(hresult))                             \
            {                                               \
                assert(false);                              \
            }                                               \
        }
    #endif

    void ReportDX11References()
    {
        IDXGIDebug* debug_interface;
        HR(DXGIGetDebugInterface(IID_PPV_ARGS(&debug_interface)));
        if (debug_interface)
        {
            debug_interface->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
        }
        SafeRelease(debug_interface);
    }
#else
    void ReportDX11References() {};
    #ifndef HR
    #define HR(x) x;
    #endif
#endif






//************
//Texture
//************

struct DX11Texture : public Texture {
    ID3D11SamplerState* m_sampler = nullptr;
    ID3D11ShaderResourceView* m_view = nullptr;
    union {
        ID3D11Texture1D* m_texture1D;
        ID3D11Texture2D* m_texture2D;
        ID3D11Texture3D* m_texture3D;
    };
    DXGI_FORMAT m_format;

    //Only used for depth and stencil textures
    ID3D11DepthStencilView* m_depth_stencil_view = nullptr;
};

void DeleteTexture(Texture** texture)
{
    VALIDATE(texture);
    VALIDATE(*texture != nullptr);
    DX11Texture* tex = reinterpret_cast<DX11Texture*>(*texture);
    switch (tex->m_dimension)
    {
    case Texture::Dimension_1D: SafeRelease(tex->m_texture1D); break;
    case Texture::Dimension_2D: SafeRelease(tex->m_texture2D); break;
    case Texture::Dimension_3D: SafeRelease(tex->m_texture3D); break;
    }
    if (tex->m_parameters.type == Texture::Type_Depth)
    {
        assert(tex->m_view == nullptr);
        assert(tex->m_sampler == nullptr);
        SafeRelease(tex->m_depth_stencil_view);
    }
    else
    {
        assert(tex->m_depth_stencil_view == nullptr);
        SafeRelease(tex->m_sampler);
        SafeRelease(tex->m_view);
    }
    delete tex;
    *texture = nullptr;
}

bool CreateTexture(Texture** texture, void* data, Vec3I size, Texture::Format format, i32 bytes_per_pixel)
{
    Texture::TextureParams tp = {};
    tp.size = size;
    tp.bytes_per_pixel = bytes_per_pixel;
    tp.format = format;
    bool r = CreateTexture(texture, tp, data);
    return r;
}
bool CreateTexture(Texture** texture, const char* fileLocation, Texture::Format format, Texture::Filter filter)
{
    Texture::TextureParams tp = {};
    u8* data = stbi_load(fileLocation, &tp.size.x, &tp.size.y, &tp.bytes_per_pixel, STBI_rgb_alpha);
    tp.format = format;
    tp.filter = filter;
    bool r = CreateTexture(texture, tp, data);
    stbi_image_free(data);
    return r;
}
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp, const void* data)
{
    const u8* new_data[] = { (u8*)data };
    return CreateTexture(texture, tp, 1, (u8*)data);
}
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp, u32 mip_levels, const u8* data)
{
    VALIDATE_V(texture, false);
    VALIDATE_V(*texture == nullptr, false);
    //VALIDATE_V(data, false);

    DX11Texture* tex = new DX11Texture;
    *texture = tex;

    tex->m_parameters = tp;
    tex->m_mip_levels = mip_levels;
    assert(tex->m_parameters.size.x != -1 && tex->m_parameters.size.x != 0);
    if (tex->m_parameters.size.z > 0)
    {
        tex->m_dimension = Texture::Dimension_3D;
    }
    else if (tex->m_parameters.size.y > 0)
    {
        tex->m_dimension = Texture::Dimension_2D;
    }
    else
    {
        tex->m_dimension = Texture::Dimension_1D;
    }

    switch (tp.format)
    {
    case Texture::Format_R11G11B10_FLOAT:       tex->m_format = DXGI_FORMAT_R11G11B10_FLOAT;    break;
    case Texture::Format_D32_FLOAT:             tex->m_format = DXGI_FORMAT_D32_FLOAT;          break;
    case Texture::Format_D16_UNORM:             tex->m_format = DXGI_FORMAT_D16_UNORM;          break;
    case Texture::Format_R8G8B8A8_UNORM:        tex->m_format = DXGI_FORMAT_R8G8B8A8_UNORM;     break;
    case Texture::Format_R8G8B8A8_UNORM_SRGB:   tex->m_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;break;
    case Texture::Format_R8G8B8A8_UINT:         tex->m_format = DXGI_FORMAT_R8G8B8A8_UINT;      break;
    case Texture::Format_R8_UINT:               tex->m_format = DXGI_FORMAT_R8_UINT;            break;
    default: FAIL;                              tex->m_format = DXGI_FORMAT_UNKNOWN;            break;
    }

    switch (tex->m_parameters.type)
    {
    case Texture::Type_Depth:
    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = (u32)tex->m_parameters.size.x;
        desc.Height = (u32)tex->m_parameters.size.y;
        desc.MipLevels = desc.ArraySize = 1;
        switch (tp.format)
        {
        case Texture::Format_D32_FLOAT:         desc.Format = DXGI_FORMAT_R32_TYPELESS;         break;
        case Texture::Format_D16_UNORM:         desc.Format = DXGI_FORMAT_R16_TYPELESS;         break;
        default: FAIL; break;
        }
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;

        HR(s_dx11.device->CreateTexture2D(&desc, NULL, &tex->m_texture2D));
    }
    {

        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Format = tex->m_format;
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;

        // Create the depth stencil view
        HR(s_dx11.device->CreateDepthStencilView(
            tex->m_texture2D,               // Depth stencil texture
            &desc,                          // Depth stencil desc
            &tex->m_depth_stencil_view));    // [out] Depth stencil view
    }
    DEBUG_LOG("Texture Created\n");
    return true;
    }


    assert(tex->m_parameters.bytes_per_pixel);

    //Create Texture
    switch (tex->m_dimension)
    {
    case Texture::Dimension_1D:
    {
        {
            D3D11_TEXTURE1D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_parameters.size.x;
            desc.MipLevels = desc.ArraySize = tex->m_mip_levels;
            desc.Format = tex->m_format;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            assert(tex->m_mip_levels == 1);
            D3D11_SUBRESOURCE_DATA sub_resource;
            sub_resource.pSysMem = data;
            sub_resource.SysMemPitch = desc.Width * tex->m_parameters.bytes_per_pixel;
            sub_resource.SysMemSlicePitch = 0;

            HR(s_dx11.device->CreateTexture1D(&desc, data ? &sub_resource : nullptr, &tex->m_texture1D));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipLevels = 1;
            desc.Texture1D.MostDetailedMip = 0;
            HR(s_dx11.device->CreateShaderResourceView(tex->m_texture1D, &desc, &tex->m_view));
        }
        break;
    }
    case Texture::Dimension_2D:
    {
        //Create Texture
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_parameters.size.x;
            desc.Height = (u32)tex->m_parameters.size.y;
            desc.MipLevels = desc.ArraySize = tex->m_mip_levels;
            desc.Format = tex->m_format;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (tp.render_target)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            assert(tex->m_mip_levels == 1);
            D3D11_SUBRESOURCE_DATA sub_resource;
            sub_resource.pSysMem = data;
            sub_resource.SysMemPitch = desc.Width * tex->m_parameters.bytes_per_pixel;
            sub_resource.SysMemSlicePitch = 0;

            HR(s_dx11.device->CreateTexture2D(&desc, data ? &sub_resource : nullptr, &tex->m_texture2D));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            desc.Texture2D.MostDetailedMip = 0;
            HR(s_dx11.device->CreateShaderResourceView(tex->m_texture2D, &desc, &tex->m_view));
        }
        break;
    }
    case Texture::Dimension_3D:
    {
        //Create Texture
        {
            D3D11_TEXTURE3D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_parameters.size.x;
            desc.Height = (u32)tex->m_parameters.size.y;
            desc.Depth = (u32)tex->m_parameters.size.z;
            desc.MipLevels = tex->m_mip_levels;
            desc.Format = tex->m_format;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

#if 0
            D3D11_SUBRESOURCE_DATA sub_resource;
            sub_resource.pSysMem = data;
            sub_resource.SysMemPitch = desc.Width * tex->m_parameters.bytes_per_pixel;
            sub_resource.SysMemSlicePitch = desc.Height * sub_resource.SysMemPitch;
#else
            D3D11_SUBRESOURCE_DATA sub_resource[MAX_MIPS] = {};
            for (u32 i = 0; i < tex->m_mip_levels; i++)
            {
                sub_resource[i].pSysMem = nullptr;
#if 0
                sub_resource[i].SysMemPitch = desc.Width * tex->m_parameters.bytes_per_pixel;
                sub_resource[i].SysMemSlicePitch = desc.Height * sub_resource[i].SysMemPitch;
#else
                sub_resource[i].SysMemPitch = (desc.Width >> i) * tex->m_parameters.bytes_per_pixel;
                sub_resource[i].SysMemSlicePitch = (desc.Height >> i) * sub_resource[i].SysMemPitch;
#endif
            }
#endif
                

            HR(s_dx11.device->CreateTexture3D(&desc, nullptr, &tex->m_texture3D));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipLevels = tex->m_mip_levels;
            desc.Texture3D.MostDetailedMip = 0;
            HR(s_dx11.device->CreateShaderResourceView(tex->m_texture3D, &desc, &tex->m_view));
        }
        break;
    }
    default:
        FAIL;
    }

    //Create Sampler
    {
        D3D11_SAMPLER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        switch (tp.filter)
        {
        case Texture::Filter_Point: desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;   break;
        case Texture::Filter_Aniso: desc.Filter = D3D11_FILTER_ANISOTROPIC;         break;
        default: FAIL;              desc.Filter = D3D11_FILTER(0);
        }
        switch (tp.mode)
        {
        case Texture::Address_Wrap:         desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;         break;
        case Texture::Address_Mirror:       desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;       break;
        case Texture::Address_Clamp:        desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;        break;
        case Texture::Address_Border:       desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;       break;
        case Texture::Address_MirrorOnce:   desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;  break;
        default: FAIL;                      desc.AddressU = desc.AddressV = desc.AddressW = D3D11_TEXTURE_ADDRESS_MODE(0);      break;
        }
        desc.MipLODBias = 0;
        desc.MaxAnisotropy = 16;
        desc.ComparisonFunc = D3D11_COMPARISON_LESS;
        desc.BorderColor[0] = desc.BorderColor[1] = desc.BorderColor[2] = desc.BorderColor[3] = 0.0f;
        desc.MinLOD = 0;
        desc.MaxLOD = 0;
        HR(s_dx11.device->CreateSamplerState(&desc, &tex->m_sampler));
    }
    DEBUG_LOG("Texture Created\n");
    return true;
}

bool UpdateTexture(Texture** texture, u32 mip_slice, void* data, u32 row_pitch_bytes, u32 depth_pitch_bytes)
{
    VALIDATE_V(texture, false);
    VALIDATE_V(*texture, false);
    DX11Texture* t = reinterpret_cast<DX11Texture*>(*texture);
    switch (t->m_dimension)
    {
    case Texture::Dimension_1D: FAIL; break;
    case Texture::Dimension_2D: FAIL; break;
    case Texture::Dimension_3D:
        s_dx11.device_context->UpdateSubresource(
            t->m_texture3D,                                         //[in]           ID3D11Resource  *pDstResource,
            D3D11CalcSubresource(mip_slice, 0, t->m_mip_levels),    //[in]           UINT            DstSubresource,
            NULL,                                                   //[in, optional] const D3D11_BOX *pDstBox,
            data,                                                   //[in]           const void      *pSrcData,
            row_pitch_bytes,                                        //[in]           UINT            SrcRowPitch,
            depth_pitch_bytes                                       //[in]           UINT            SrcDepthPitch
        );
    break;
    default: FAIL; break;
    }

    return true;
}



void WaitForPreviousFrame()
{
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. More advanced samples 
    // illustrate how to use fences for efficient resource usage.

    // Signal and increment the fence value.
    const UINT64 fence = s_dx12.fence_value;
    HR(s_dx12.command_queue->Signal(s_dx12.fence, fence));
    s_dx12.fence_value++;

    // Wait until the previous frame is finished.
    if (s_dx12.fence->GetCompletedValue() < fence)
    {
        HR(s_dx12.fence->SetEventOnCompletion(fence, s_dx12.fence_event));
        WaitForSingleObject(s_dx12.fence_event, INFINITE);
    }

    //s_dx12.frame_index = s_dx12.swap_chain->GetCurrentBackBufferIndex();
}



//************
//Buffer
//************

struct DX12GpuBuffer : public GpuBuffer
{
    //D3D11_USAGE m_usage = D3D11_USAGE_DYNAMIC;
    //ID3D12Buffer* m_buffer = nullptr;
    ID3D12Resource* m_buffer = nullptr;
    //ID3D12ShaderResourceView* structure_resource_view = nullptr;
    //D3D11_BIND_FLAG m_target = {};
};

//void GpuBuffer::UploadData(const void* data, u32 element_size, size_t count)
//TODO: Clean this up with Type::Vertex = D3D11_BIND_VERTEX_BUFFER
void GpuBuffer::Upload(const void* data, const size_t count, const u32 element_size, const bool is_byte_format)
{
    DX12GpuBuffer* buf = reinterpret_cast<DX12GpuBuffer*>(this);
    m_count = count;
    SafeRelease(buf->m_buffer);
    assert(data);
    assert(element_size);
    assert(buf->m_type != GpuBuffer::Type::Invalid);
    VALIDATE(count);
    UINT total_bytes = UINT(element_size * count);
    //assert(total_bytes / 16 == 0);
    D3D12_RESOURCE_DIMENSION resource_dimension;
    UINT buffer_type = 0;
    UINT cpu_access_flags = 0;
    UINT struct_byte_stride = 0;
    UINT memory_pitch = 0;
    UINT misc_flags = 0;
    switch (buf->m_type)
    {
    case GpuBuffer::Type::Vertex:
        resource_dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        break;
    case GpuBuffer::Type::Index:
        resource_dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        break;
    case GpuBuffer::Type::Constant:
        resource_dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        break;
    case GpuBuffer::Type::Structure:
        resource_dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        //misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        assert(!buf->m_is_dymamic);
        struct_byte_stride = element_size;
        break;
    default:
        FAIL;
    }

    if (!buf->m_buffer)
    {
        {
            D3D12_HEAP_PROPERTIES heap_props;
            heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
            heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heap_props.CreationNodeMask = 1;
            heap_props.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC desc;
            desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            desc.Alignment = 0;
            desc.Width = total_bytes;
            desc.Height = 1;
            desc.DepthOrArraySize = 1;
            desc.MipLevels = 1;
            desc.Format = DXGI_FORMAT_UNKNOWN;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;


            HR(s_dx12.device->CreateCommittedResource(heap_props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&buf->m_buffer)));

            // Copy the triangle data to the vertex buffer.
            UINT8* pVertexDataBegin;
            D3D12_RANGE read_range;
            HR(buf->m_buffer->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)));
            memcpy(pVertexDataBegin, data, total_bytes);
            buf->m_buffer->Unmap(0, nullptr);

            // Initialize the vertex buffer view.
            D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;
            vertex_buffer_view.BufferLocation = buf->m_buffer->GetGPUVirtualAddress();
            vertex_buffer_view.StrideInBytes = element_size;
            vertex_buffer_view.SizeInBytes = total_bytes;
        }
        DEBUG_LOG("Created and Uploaded data to gpu buffer: element: %i size: %i", element_size, count);

        //if (buf->m_type == GpuBuffer::Type::Structure)
        //{
        //    D3D11_SHADER_RESOURCE_VIEW_DESC desc;
        //    ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
        //    desc.Format = is_byte_format ? DXGI_FORMAT_R8_UINT : DXGI_FORMAT_UNKNOWN;
        //    desc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
        //    desc.Buffer.FirstElement = 0;
        //    desc.Buffer.NumElements = (UINT)count;
        //    HR(s_dx11.device->CreateShaderResourceView(
        //        buf->m_buffer,                  //[in]            ID3D11Resource * pResource,
        //        &desc,                          //[in, optional]  const D3D11_SHADER_RESOURCE_VIEW_DESC * pDesc,
        //        &buf->structure_resource_view   //[out, optional] ID3D11ShaderResourceView * *ppSRView
        //    ));
        //}

        return;
    }

    //if (buf->m_is_dymamic)
    //{
    //    //map/unmap/memcopy
    //    D3D11_MAPPED_SUBRESOURCE resource;
    //    ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
    //    HR(s_dx11.device_context->Map(
    //        buf->m_buffer,          //[in]            ID3D11Resource * pResource,
    //        0,                      //[in]            UINT                     Subresource,
    //        D3D11_MAP_WRITE_DISCARD,//[in]            D3D11_MAP                MapType,
    //        0,                      //[in]            UINT                     MapFlags,
    //        &resource               //[out, optional] D3D11_MAPPED_SUBRESOURCE * pMappedResource
    //    ));
    //    memcpy(resource.pData, data, element_size * count);
    //    s_dx11.device_context->Unmap(buf->m_buffer, 0);
    //    DEBUG_LOG("Uploaded dynamic_buffer data to gpu buffer: element: %i size: %i", element_size, count);
    //}
    //else
    //{
    //    s_dx11.device_context->UpdateSubresource(
    //        buf->m_buffer,  //[in]           ID3D11Resource * pDstResource,
    //        0,              //[in]           UINT            DstSubresource,
    //        NULL,           //[in, optional] const D3D11_BOX * pDstBox,
    //        data,           //[in]           const void* pSrcData,
    //        total_bytes,    //[in]           UINT            SrcRowPitch,
    //        0               //[in]           UINT            SrcDepthPitch
    //    );
    //    DEBUG_LOG("Uploaded default_buffer data to gpu buffer: element: %i size: %i", element_size, count);
    //}
}

void GpuBuffer::Bind(u32 slot, GpuBuffer::BindLocation binding)
{
    DX11GpuBuffer* buf = reinterpret_cast<DX11GpuBuffer*>(this);
    switch (m_type)
    {
    case GpuBuffer::Type::Constant:
    {
        switch (binding)
        {
        case GpuBuffer::BindLocation::Vertex:
            s_dx11.device_context->VSSetConstantBuffers(slot, 1, &buf->m_buffer);
            break;
        case GpuBuffer::BindLocation::Pixel:
            s_dx11.device_context->PSSetConstantBuffers(slot, 1, &buf->m_buffer);
            break;
        case GpuBuffer::BindLocation::All:
            s_dx11.device_context->VSSetConstantBuffers(slot, 1, &buf->m_buffer);
            s_dx11.device_context->PSSetConstantBuffers(slot, 1, &buf->m_buffer);
            break;
        default:
            FAIL;
            break;
        }
        break;
    }
    case GpuBuffer::Type::Structure:
    {
        switch (binding)
        {
        case GpuBuffer::BindLocation::Vertex:
            s_dx11.device_context->VSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        case GpuBuffer::BindLocation::Pixel:
            s_dx11.device_context->PSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        case GpuBuffer::BindLocation::All:
            s_dx11.device_context->VSSetShaderResources(slot, 1, &buf->structure_resource_view);
            s_dx11.device_context->PSSetShaderResources(slot, 1, &buf->structure_resource_view);
            break;
        default:
            FAIL;
            break;
        }
        break;
    }
    default:
        FAIL;
    }
}

bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, bool is_dynamic, GpuBuffer::Type type)
{
    assert(buffer);
    assert(*buffer == nullptr);
    DX12GpuBuffer* buf = new DX12GpuBuffer;
    buf->m_is_dymamic = is_dynamic;
    buf->m_type = type;
    strcpy(buf->m_name, name);
    (*buffer) = reinterpret_cast<GpuBuffer*>(buf);
    return true;
}

void DeleteBuffer(GpuBuffer** buffer)
{
    VALIDATE(buffer);
    DX11GpuBuffer* buf = reinterpret_cast<DX11GpuBuffer*>(*buffer);
    SafeRelease(buf->m_buffer);
    delete buf;
    DEBUG_LOG("GPU Buffer deleted %i, %i\n", m_target, m_handle);
}





//************
//Shader
//************

struct DX11IncludeManager : ID3DInclude
{
    std::vector<std::string> m_included_shader_files;
    File* file;

    virtual HRESULT Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override
    {
        std::vector<std::string> filenames;
        std::string filename = "source/";
        ScanDirectoryForFileNames(filename, filenames);

        for (size_t i = 0; i < filenames.size(); i++)
        {
            if (filenames[i].contains(pFileName))
            {
                filename += filenames[i];
                break;
            }
        }
        m_included_shader_files.push_back(filename);
        file = new File(filename, File::Mode::Read, false);//Why doesnt this work if its not a pointer
        file->GetText();
        if (!file->m_textIsValid)
        {
            ppData = nullptr;
            pBytes = nullptr;
            return E_FAIL;
        }
        char* text = new char[file->m_dataString.size()];
        memcpy(text, file->m_dataString.c_str(), file->m_dataString.size());

        *ppData = text;
        *pBytes = (UINT)file->m_dataString.size();
        return S_OK;
    }
    virtual HRESULT Close(LPCVOID pData) override
    {
        delete file;
        if (pData == nullptr)
            return E_FAIL;
        return S_OK;
    }
};


struct DX12Shader : public Shader
{
    ID3DBlob* m_vertex_blob;
    ID3DBlob* m_pixel_blob;

    D3D12_INPUT_ELEMENT_DESC m_local_layout[m_vertex_component_max] = {};

    ////D3D11_USAGE m_usage = D3D11_USAGE_DYNAMIC;
    //ID3D11Buffer* m_buffer = nullptr;
    //ID3D11ShaderResourceView* structure_resource_view = nullptr;
    //ID3D11InputLayout* m_vertex_input_layout = nullptr;
    //ID3D11VertexShader* m_vertex_shader = nullptr;
    //ID3D11PixelShader* m_pixel_shader = nullptr;
    //static const u32 m_vertex_component_max = 4;

    ////D3D11_BIND_FLAG m_target = {};
};

bool CreateShader(Shader** s,
    const std::string& vertex_filename,
    const std::string& pixel_filename,
    Shader::InputElementDesc* input_layout,
    i32 layout_count)
{
    assert(s);
    assert(*s == nullptr);
    DX12Shader* shader = new DX12Shader;
    (*s) = reinterpret_cast<DX12Shader*>(shader);

    ConvertMultibyteToWideChar(shader->m_vertex_filename, vertex_filename);
    ConvertMultibyteToWideChar(shader->m_pixel_filename, pixel_filename);

    shader->m_vertex_component_count = layout_count;
    for (u32 i = 0; i < shader->m_vertex_component_count; i++)
    {
        shader->m_local_layout[i] = {
        .SemanticName = input_layout[i].SemanticName,
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT(input_layout[i].Format),
        .InputSlot = 0,
        .AlignedByteOffset = input_layout[i].AlignedByteOffset,
        .InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0,
        };

    }

    shader->CheckForUpdate();
    return true;
}
Shader::~Shader()
{
    DX12Shader* shader = reinterpret_cast<DX12Shader*>(this);
    //SafeRelease(shader->m_vertex_shader);
    SafeRelease(shader->m_vertex_blob);
    //SafeRelease(shader->m_pixel_shader);
    SafeRelease(shader->m_pixel_blob);
    //SafeRelease(shader->m_vertex_input_layout);
    DEBUG_LOG("Shader Program Deleted\n");
}
bool Shader::CompileShader(const std::wstring& file_name, Type shader_type)
{
    DX12Shader* shader = reinterpret_cast<DX12Shader*>(this);
    bool failed = false;
#if 1
    D3D_SHADER_MACRO* shader_macros = nullptr;
#else
    D3D_SHADER_MACRO shader_macros[] = {
        //{"RAY_TRACING", "1"},
    };
#endif

    std::string entry_point;
    std::string target_version;
    ID3DBlob** shader_blob;
    switch (shader_type)
    {
    case Type_Vertex:
        entry_point = "Vertex_Main";
        target_version = "vs_5_0";
        *shader_blob = shader->m_vertex_blob;
        break;
    case Type_Pixel:
        entry_point = "Pixel_Main";
        target_version = "ps_5_0";
        *shader_blob = shader->m_pixel_blob;
        break;
    default:
        FAIL;
    }

    u32 flags1 = 0;
    //flags1 |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
    flags1 |= D3DCOMPILE_PREFER_FLOW_CONTROL;
#if _DEBUG
    //;
    flags1 |= D3DCOMPILE_DEBUG;
#endif

    //Create Blob
    SafeRelease(*shader_blob);
    ID3DBlob* errors;
    DX11IncludeManager include_manager;
    HRESULT compile_result = s_dx12.D3DCompileFromFileFunc(
        m_vertex_filename.c_str(),  //[in]            LPCWSTR                pFileName,
        shader_macros,              //[in, optional]  const D3D_SHADER_MACRO *pDefines,
        &include_manager,           //[in, optional]  ID3DInclude            *pInclude,
        entry_point.c_str(),        //[in]            LPCSTR                 pEntrypoint,
        target_version.c_str(),     //[in]            LPCSTR                 pTarget,
        flags1,                     //[in]            UINT                   Flags1,
        0,                          //[in]            UINT                   Flags2,
        shader_blob,                //[out]           ID3DBlob               **ppCode,
        &errors                     //[out, optional] ID3DBlob               **ppErrorMsgs
    );

    if (!shader_blob || !!errors || FAILED(compile_result))
    {
        std::wstring info_string;
        info_string.resize(errors->GetBufferSize());
        memcpy(info_string.data(), errors->GetBufferPointer(), errors->GetBufferSize());
        std::wstring error_title = m_vertex_filename.c_str();
        error_title += L" Compilation Error: ";
        DebugPrint((error_title + info_string + L"\n").c_str());

        SDL_MessageBoxButtonData buttons[] = {
            //{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Continue" },
        };

        i32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), MessageBoxType::Error, error_title.c_str(), info_string.c_str());
        {
            if (buttons[buttonID].buttonid == 0)//NOTE: Retry button
            {
                CheckForUpdate();
            }
            else if (buttons[buttonID].buttonid == 1)//NOTE: Continue button
            {
                return;
            }
        }
    }

    for (i32 i = 0; i < include_manager.m_included_shader_files.size(); i++)
    {
        for (i32 i = 0; i < m_reference_file_names.size(); i++)
        {
            if (m_reference_file_names[i].find(include_manager.m_included_shader_files[i]) != std::string::npos)
            {
                goto postloops;
            }
        }
        m_reference_file_names.push_back(include_manager.m_included_shader_files[i]);
        m_reference_file_times.push_back(0);
    }
postloops:

    DEBUG_LOG("Shader Vertex/Fragment Created\n");
    return true;
}

void GetShaderReferenceFileTimes(std::vector<u64>& out, Shader* p)
{
    out.resize(p->m_reference_file_times.size(), 0);

    for (i32 i = 0; i < p->m_reference_file_names.size(); i++)
    {
        std::string& s = p->m_reference_file_names[i];
        File file(s, File::Mode::Read, false);
        file.GetTime();
        VALIDATE(file.m_timeIsValid);
        out[i] = file.m_time;
    }
}

void Shader::CheckForUpdate()
{
    u64 vertexFileTime;
    u64 pixelFileTime;
    std::vector<u64> referenced_file_times;
    {

        File vertexFile(m_vertex_filename, File::Mode::Read, false);
        vertexFile.GetTime();
        VALIDATE(vertexFile.m_timeIsValid);
        vertexFileTime = vertexFile.m_time;

        File pixelFile(m_pixel_filename, File::Mode::Read, false);
        pixelFile.GetTime();
        VALIDATE(pixelFile.m_timeIsValid);
        pixelFileTime = pixelFile.m_time;

        GetShaderReferenceFileTimes(referenced_file_times, this);
    }

    bool needs_update = false;
    for (i32 i = 0; i < m_reference_file_names.size(); i++)
    {
        if (m_reference_file_times[i] < referenced_file_times[i])
        {
            needs_update = true;
            break;
        }
    }

    if (needs_update ||
        m_vertexLastWriteTime < vertexFileTime ||
        m_pixelLastWriteTime  < pixelFileTime)
    {
        //Compile shaders and link to program
        if (!CompileShader(m_vertex_filename, Type_Vertex) ||
            !CompileShader(m_pixel_filename, Type_Pixel))
            return;

        DEBUG_LOG("Shader Created\n");
        m_vertexLastWriteTime = vertexFileTime;
        m_pixelLastWriteTime = pixelFileTime;

        if (referenced_file_times.size() != m_reference_file_times.size())
        {
            GetShaderReferenceFileTimes(referenced_file_times, this);
        }
        for (i32 i = 0; i < m_reference_file_names.size(); i++)
        {
            m_reference_file_times[i] = referenced_file_times[i];
        }
    }
}













void CreateRenderTargetView(ID3D12RenderTargetView** rtv, DXGI_FORMAT format, ID3D11Texture2D* texture)
{
    assert(rtv);
    if (*rtv)
    {
        SafeRelease(*rtv);
        *rtv = nullptr;
    }

    D3D11_RENDER_TARGET_VIEW_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Format = format;
    desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    desc.Texture2D.MipSlice = 0;

    VERIFY(SUCCEEDED(s_dx11.device->CreateRenderTargetView(
        texture,    //[in]            ID3D11Resource* pResource,
        &desc,      //[in, optional]  const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
        rtv         //[out, optional] ID3D11RenderTargetView** ppRTView
    )));
}
void CreateRenderTargetView(ID3D11RenderTargetView** rtv, Texture::Index texture_index)
{
    DX11Texture* t = reinterpret_cast<DX11Texture*>(g_renderer.textures[texture_index]);
    CreateRenderTargetView(rtv, t->m_format, t->m_texture2D);
}


typedef HRESULT(*D3DCompileFunc)        (LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
typedef HRESULT(*D3DCompileFromFileFunc)(LPCWSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);

void InitializeImGui()
{
    //___________
    //IMGUI SETUP
    //___________

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForD3D(g_renderer.SDL_Context);
    //ImGui_ImplDX11_Init(s_dx12.device, s_dx11.device_context);
    //D3D12_CPU_DESCRIPTOR_HANDLE desc_handle;


    //ImGui_ImplDX12_Init(
    //ID3D12Device* device, 
    //int num_frames_in_flight, 
    //DXGI_FORMAT rtv_format, 
    //ID3D12DescriptorHeap* cbv_srv_heap, 
    //D3D12_CPU_DESCRIPTOR_HANDLE font_srv_cpu_desc_handle, 
    //D3D12_GPU_DESCRIPTOR_HANDLE font_srv_gpu_desc_handle);

    ImGui_ImplDX12_Init(s_dx12.device, FRAME_TARGET_COUNT, DXGI_FORMAT_R8G8B8A8_UNORM, s_dx12.rtv_heap, s_dx12.rtv_handle, s_dx12.gpu_handle);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    SDL_ShowCursor(SDL_ENABLE);
}

void UpdateSwapchain(const Vec2I& window_size)
{
    SafeRelease(s_dx11.swap_chain.render_target_view);
    HR(s_dx11.swap_chain.handle->ResizeBuffers(
        0,                  //UINT        BufferCount, IS THIS RIGHT???
        (UINT)window_size.x,//UINT        Width,
        (UINT)window_size.y,//UINT        Height,
        DXGI_FORMAT_UNKNOWN,//DXGI_FORMAT_R8G8B8A8_UNORM, //DXGI_FORMAT NewFormat,
        0                   //UINT        SwapChainFlags
    ));

    DXGI_SWAP_CHAIN_DESC desc;
    s_dx11.swap_chain.handle->GetDesc(&desc);
    assert(desc.BufferDesc.Width == window_size.x);
    assert(desc.BufferDesc.Height == window_size.y);
    s_dx11.swap_chain.size.x = desc.BufferDesc.Width;
    s_dx11.swap_chain.size.y = desc.BufferDesc.Height;
    s_dx11.swap_chain.refresh_rate = desc.BufferDesc.RefreshRate.Numerator;
    s_dx11.swap_chain.sample_count = desc.SampleDesc.Count;
    s_dx11.swap_chain.sample_quality = desc.SampleDesc.Quality;


    ID3D11Texture2D* backbuffer;
    VERIFY(SUCCEEDED(s_dx11.swap_chain.handle->GetBuffer(0, IID_PPV_ARGS(&backbuffer))));
    CreateRenderTargetView(&s_dx11.swap_chain.render_target_view, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, backbuffer);

    D3D11_TEXTURE2D_DESC backbuffer_desc = {};
    backbuffer->GetDesc(&backbuffer_desc);
    SafeRelease(backbuffer);
    {
        Texture** t = &g_renderer.textures[Texture::Index_Backbuffer_Depth];
        if (*t)
        {
            Texture::TextureParams tp = (*t)->m_parameters;
            tp.size.xy = window_size;
            assert(tp.size.z == 0);
            DeleteTexture(t);
            CreateTexture(t, tp, nullptr);
        }
    }
    {
        Texture** t = &g_renderer.textures[Texture::Index_Backbuffer_HDR];
        if (*t)
        {
            Texture::TextureParams tp = (*t)->m_parameters;
            tp.size.xy = window_size;
            assert(tp.size.z == 0);
            DeleteTexture(t);
            CreateTexture(t, tp, nullptr);
            CreateRenderTargetView(&s_dx11.hdr_rtv, Texture::Index_Backbuffer_HDR);
        }
    }

}

//#pragma comment(lib, "dxgi.lib")
void InitializeVideo()
{
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
    //Is this needed?
    SDL_SetHint(SDL_HINT_RENDER_DRIVER,         "direct3d11");

    SDL_Init(SDL_INIT_VIDEO);
    {
        SDL_Rect screenSize = {};
        SDL_GetDisplayBounds(0, &screenSize);
        float displayRatio = 16 / 9.0f;
        g_renderer.size.x = screenSize.w / 2;
        g_renderer.size.y = Clamp<int>(int(g_renderer.size.x / displayRatio), 50, screenSize.h);
        g_renderer.pos.x = g_renderer.size.x / 2;
        g_renderer.pos.y = g_renderer.size.y / 2;

        SDL_DisplayMode display_mode;
        SDL_GetDisplayMode(0, 0, &display_mode);
        g_renderer.refresh_rate = display_mode.refresh_rate;

    }

    u32 windowFlags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI /*SDL_WINDOW_MOUSE_CAPTURE | *//*SDL_WINDOW_MOUSE_FOCUS | *//*SDL_WINDOW_INPUT_GRABBED*/;

    g_renderer.SDL_Context = SDL_CreateWindow("V3", g_renderer.pos.x, g_renderer.pos.y, g_renderer.size.x, g_renderer.size.y, windowFlags);

    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(g_renderer.SDL_Context, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;



    //
    // INIT DX12
    //



    //Enable DX12 Debug Layer
#ifdef _DEBUG
    {
        ID3D12Debug* debug_controller;
        HR(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
        if (debug_controller)
            debug_controller->EnableDebugLayer();

        SafeRelease(debug_controller);
    }
#endif


    // Get factory and create device

    HR(CreateDXGIFactory(IID_PPV_ARGS(&s_dx12.factory)));
    //GetHardwareAdapter(s_dx12.factory, hardware_adapter);
    IDXGIAdapter* hardware_adapter = nullptr;
    {
        for (UINT i = 0; ; i++)
        {
            IDXGIAdapter* adapter = nullptr;
            if (DXGI_ERROR_NOT_FOUND == s_dx12.factory->EnumAdapters(i, &adapter))
            {
                // No more adapters to enumerate.
                break;
            }
            // Check to see if the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                hardware_adapter = adapter;
                break;
            }
            SafeRelease(adapter);
        }

    }
    assert(hardware_adapter);
    HR(D3D12CreateDevice(hardware_adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&s_dx12.device)));


    //Create the command queue.
    ID3D12CommandQueue* command_queue;
    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        HR(s_dx12.device->CreateCommandQueue(&desc, IID_PPV_ARGS(&command_queue)));
    }


    //Create the swap chain
    {
        DXGI_RATIONAL refresh_rate;
        refresh_rate.Numerator = g_renderer.refresh_rate;
        refresh_rate.Denominator = 1;

        DXGI_MODE_DESC dxgi_mode_desc;
        {
#if 0
            dxgi_mode_desc.Width = g_renderer.size.x;
            dxgi_mode_desc.Height = g_renderer.size.y;
#else
            dxgi_mode_desc.Width = 0;
            dxgi_mode_desc.Height = 0;
#endif
            dxgi_mode_desc.RefreshRate = refresh_rate;
            dxgi_mode_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //DXGI_FORMAT_R32G32B32A32_FLOAT;
            dxgi_mode_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            dxgi_mode_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        }

        //MSAA
        DXGI_SAMPLE_DESC dxgi_sample_desc;
        {
            dxgi_sample_desc.Count = 1;
            dxgi_sample_desc.Quality = 0;
        }

        DXGI_SWAP_CHAIN_DESC swap_chain_desc;
        swap_chain_desc.BufferDesc = dxgi_mode_desc;
        swap_chain_desc.SampleDesc = dxgi_sample_desc;
        //WARNING: Does this need to be only DXGI_USAGE_RENDER_TARGET_OUTPUT?
        swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = FRAME_TARGET_COUNT; //is this right?
        swap_chain_desc.OutputWindow = hwnd;
        swap_chain_desc.Windowed = TRUE;
        //TODO: Change to DXGI_SWAP_EFFECT_FLIP_DISCARD
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; //Does not work with MSAA
        swap_chain_desc.Flags = 0; //do we need this?  DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING

        //HR(s_dx12.factory->CreateSwapChain(
        //    command_queue,        // Swap chain needs the queue so that it can force a flush on it.
        //    &swap_chain_desc,
        //    &s_dx12.swap_chain.handle));
        IDXGISwapChain* swap_chain1;
        HR(s_dx12.factory4->CreateSwapChain(
            command_queue,        // Swap chain needs the queue so that it can force a flush on it.
            &swap_chain_desc,
            &swap_chain1));
        //TODO: Add error checking for failing to get IDXGISwapChain3
        HR(swap_chain1->QueryInterface(&s_dx12.swap_chain.handle));
    }

    UINT frame_index = s_dx12.swap_chain.handle->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    UINT rtv_descriptor_size;
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = FRAME_TARGET_COUNT;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HR(s_dx12.device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&s_dx12.rtv_heap)));

        rtv_descriptor_size = s_dx12.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        s_dx12.rtv_handle = s_dx12.rtv_heap->GetCPUDescriptorHandleForHeapStart();

        // Create a RTV for each frame.
        for (UINT i = 0; i < FRAME_TARGET_COUNT; i++)
        {
            //s_dx12.device->CreateRenderTargetView()
            //CreateRenderTargetView(&s_dx11.hdr_rtv, Texture::Index_Backbuffer_HDR);
            HR(s_dx12.swap_chain.handle->GetBuffer(i, IID_PPV_ARGS(&s_dx12.render_targets[i])));
            s_dx12.device->CreateRenderTargetView(s_dx12.render_targets[i], nullptr, s_dx12.rtv_handle);
            //s_dx12.rtv_handle.Offset(1, rtv_descriptor_size);
            s_dx12.rtv_handle.ptr = SIZE_T(INT64(s_dx12.rtv_handle.ptr) + INT64(1) * INT64(rtv_descriptor_size)):
        }
    }
    {
        //THIS IS CURRENTLY ONLY USED BY IMGUI...
        // WHAT IS THIS !?
        s_dx12.gpu_handle = s_dx12.rtv_heap->GetGPUDescriptorHandleForHeapStart();
    }

    HR(s_dx12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s_dx12.command_allocator)));

    {
        HINSTANCE dll_instance = LoadLibrary(L"d3dcompiler_47.dll");
        VALIDATE(dll_instance);
        s_dx12.D3DCompileFunc          = (D3DCompileFunc)GetProcAddress(dll_instance, "D3DCompile");
        s_dx12.D3DCompileFromFileFunc  = (D3DCompileFromFileFunc)GetProcAddress(dll_instance, "D3DCompileFromFile");
    }



    //
    // LOAD ASSETS
    //

    
    // Create an empty root signature.
    {
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

        ID3DBlob* signature;
        ID3DBlob* error;
        HR(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
        HR(s_dx12.device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&s_dx12.root_signature)));
    }

    //Compile shader
    InitializeData(s_dx12.swap_chain.size);

    D3D12_RASTERIZER_DESC rasterizer_full;
    // Create the rasterizer state
    {
        ZeroMemory(&rasterizer_full, sizeof(rasterizer_full));

        rasterizer_full.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizer_full.CullMode = D3D12_CULL_MODE_BACK;
        rasterizer_full.FrontCounterClockwise = TRUE;
        rasterizer_full.DepthBias = 0;
        rasterizer_full.DepthBiasClamp = 0.0f;
        rasterizer_full.SlopeScaledDepthBias = 0.0f;
        rasterizer_full.DepthClipEnable = TRUE;
        rasterizer_full.MultisampleEnable = TRUE;
        rasterizer_full.AntialiasedLineEnable = TRUE;
        rasterizer_full.ForcedSampleCount = 0;
        rasterizer_full.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    D3D12_BLEND_DESC blend_desc;
    {
        ZeroMemory(&blend_desc, sizeof(blend_desc));
        blend_desc.AlphaToCoverageEnable = false;
        blend_desc.IndependentBlendEnable = false;
        blend_desc.RenderTarget[0].BlendEnable = true;
        blend_desc.RenderTarget[0].LogicOpEnable = false;
        blend_desc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend_desc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blend_desc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blend_desc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        //blend_desc.RenderTarget[0].LogicOp;
        blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    {
        DX12Shader* shader = reinterpret_cast<DX12Shader*>(g_renderer.shaders[+Shader::Index_Cube]);
        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { shader->m_local_layout, arrsize(shader->m_local_layout) };
        psoDesc.pRootSignature = s_dx12.root_signature;
        psoDesc.VS = { reinterpret_cast<UINT8*>(shader->m_vertex_blob->GetBufferPointer()), shader->m_vertex_blob->GetBufferSize()  };
        psoDesc.PS = { reinterpret_cast<UINT8*>(shader->m_pixel_blob->GetBufferPointer()),  shader->m_pixel_blob->GetBufferSize()   };
        psoDesc.RasterizerState = rasterizer_full;
        psoDesc.BlendState = blend_desc;
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; //TODO: Render to a secondary HDR buffer
        psoDesc.SampleDesc.Count = 1;
        HR(s_dx12.device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&s_dx12.pipeline_state[PipelineState::Cube_Full])));
    }

    // Create the command list.
    HR(s_dx12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s_dx12.command_allocator, s_dx12.pipeline_state[PipelineState::Cube_Full], IID_PPV_ARGS(&s_dx12.command_list)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    HR(s_dx12.command_list->Close());


#if 0
    //Create Blender State
    {
        D3D12_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.IndependentBlendEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        s_dx12.device->CreateBlendState(&desc, &s_dx12.blend_state);
    }

    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_WIREFRAME;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = TRUE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthClipEnable = TRUE;
        desc.ScissorEnable = FALSE;
        desc.MultisampleEnable = TRUE;
        desc.AntialiasedLineEnable = TRUE;
        s_dx11.device->CreateRasterizerState(&desc, &s_dx11.rasterizer_wireframe);
    }
    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = TRUE;
        desc.DepthBias = 0;

        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthClipEnable = FALSE;
        desc.ScissorEnable = FALSE;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;
        s_dx11.device->CreateRasterizerState(&desc, &s_dx11.rasterizer_voxel);
    }
    CreateRenderTargetView(&s_dx11.hdr_rtv, Texture::Index_Backbuffer_HDR);
    {
        D3D11_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        // Depth test parameters
        desc.DepthEnable = true;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;

        // Stencil test parameters
        desc.StencilEnable = false;
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;

        // Stencil operations if pixel is front-facing
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Stencil operations if pixel is back-facing
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Create depth stencil state
        HR(s_dx11.device->CreateDepthStencilState(&desc, &s_dx11.depth_stencil_state_depth));
    }
    {
        D3D11_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        // Depth test parameters
        desc.DepthEnable = false;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        desc.DepthFunc = D3D11_COMPARISON_LESS;

        // Stencil test parameters
        desc.StencilEnable = false;
        desc.StencilReadMask = 0xFF;
        desc.StencilWriteMask = 0xFF;

        // Stencil operations if pixel is front-facing
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Stencil operations if pixel is back-facing
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        // Create depth stencil state
        HR(s_dx11.device->CreateDepthStencilState(&desc, &s_dx11.depth_stencil_state_no_depth));
    }
#endif

    {
        HR(s_dx12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s_dx12.fence)));
        s_dx12.fence_value = 1;

        // Create an event handle to use for frame synchronization.
        s_dx12.fence_event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (s_dx12.fence_event == nullptr)
        {
            HR(HRESULT_FROM_WIN32(GetLastError()));
        }
    }

    WaitForPreviousFrame();

    InitializeImGui();
}



Vec3 Step(Vec3 a, float b)
{
    Vec3 r = {};
    if (b > a.x)
        r.x = 1;
    if (b > a.y)
        r.y = 1;
    if (b > a.z)
        r.z = 1;
    return r;
}

// Converts a color from sRGB gamma to linear light gamma
Vec4 srgb_to_linear(Vec4 sRGB)
{
    Vec3 cutoff = Step(sRGB.rgb, 0.04045f);
    // abs is here to silence compiler warning
    Vec3 a = (Abs(sRGB.rgb) + 0.055f) / 1.055f;
    Vec3 higher;
    higher.x = powf(a.x, 2.4f);
    higher.y = powf(a.y, 2.4f);
    higher.z = powf(a.z, 2.4f);
    Vec3 lower = sRGB.rgb / 12.92f;
    Vec3 result = Lerp(higher, lower, cutoff);
    return Vec4(result.r, result.g, result.b, sRGB.a);
}

double s_last_shader_update_time = 0;
double s_incremental_time = 0;
void RenderUpdate(Vec2I window_size, float deltaTime)
{
    ZoneScopedN("Render Update");

#if 1
    //DX12 Implementation:

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    HR(s_dx12.command_allocator->Reset());

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    HR(s_dx12.command_allocator->Reset(s_dx12.command_allocator.Get(), s_dx12.pipeline_state.Get()));

    // Set necessary state.
    s_dx12.command_list->SetGraphicsRootSignature(s_dx12.root_signature.Get());
    s_dx12.command_list->RSSetViewports(1, &s_dx12.m_viewport);
    s_dx12.command_list->RSSetScissorRects(1, &m_scissorRect);

    // Indicate that the back buffer will be used as a render target.
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
    m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    m_commandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_commandList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(m_commandList->Close());
#else
    //Vec2I window_size;
    //SDL_GetWindowSizeInPixels(g_renderer.SDL_Context, &window_size.x, &window_size.y);
    if (s_dx11.swap_chain.size != window_size)
        UpdateSwapchain(window_size);

    Vec4 background_color = srgb_to_linear(backgroundColor);
    s_dx11.device_context->ClearRenderTargetView(s_dx11.swap_chain.render_target_view, background_color.e);
    s_dx11.device_context->ClearRenderTargetView(s_dx11.hdr_rtv, background_color.e);
    DX11Texture* depth = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);
    s_dx11.device_context->ClearDepthStencilView(depth->m_depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

#endif
    if (s_last_shader_update_time + 0.1f <= s_incremental_time)
    {
        for (Shader* s : g_renderer.shaders)
        {
            if (s)
                s->CheckForUpdate();
        }
        s_last_shader_update_time = s_incremental_time;
    }
    s_incremental_time += deltaTime;
}






#if 0
TextureArray::TextureArray(const char* fileLocation)
{
    u8* data = stbi_load(fileLocation, &m_size.x, &m_size.y, NULL, STBI_rgb_alpha);
    Defer{
        stbi_image_free(data);
    };
    assert(data);

    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexImage2D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    u32 mipMapLevels = 5;
    m_spritesPerSide = { 16, 16 };
    u32 height = m_spritesPerSide.y;
    u32 width = m_spritesPerSide.x;
    u32 depth = 256;

    //SRGB8_ALPHA8 is used since the texture is encoded in sRGB color space
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, mipMapLevels, GL_SRGB8_ALPHA8, width, height, depth);

    //TODO: Fix
    u32 colors[16 * 16] = {};
    u32 arrayIndex = 0;
    for (u32 y = height; y--;)
    {
        for (u32 x = 0; x < width; x++)
        {
            for (u32 xp= 0; xp < 16; xp++)  //Total
            {
                for (u32 yp = 0; yp < 16; yp++)  //Total
                {
                    u32 sourceIndex = (y * 16 + yp) * 256 + x * 16 + xp;
                    u32 destIndex = yp * 16 + xp;
                    colors[destIndex] = reinterpret_cast<u32*>(data)[sourceIndex];
                }
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, arrayIndex, 16, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, colors);
            arrayIndex++;
        }
    }
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Created\n");
#endif
}

void TextureArray::Update(float anisotropicAmount)
{
    if (anisotropicAmount == m_anisotropicAmount)
        return;
    m_anisotropicAmount = anisotropicAmount;

    Bind();
    glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_anisotropicAmount);
}

void TextureArray::Bind()
{
    glBindTexture(GL_TEXTURE_2D_ARRAY, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Texture Bound\n");
#endif
}


struct DDS_PIXELFORMAT
{
    u32 dwSize;
    u32 dwFlags;
    u32 dwFourCC;
    u32 dwRGBBitCount;
    u32 dwRBitMask;
    u32 dwGBitMask;
    u32 dwBBitMask;
    u32 dwABitMask;
};
static_assert(sizeof(DDS_PIXELFORMAT) == 32, "Incorrect structure size!");

typedef struct
{
    u32           dwSize;
    u32           dwFlags;
    u32           dwHeight;
    u32           dwWidth;
    u32           dwPitchOrLinearSize;
    u32           dwDepth;
    u32           dwMipMapCount;
    u32           dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    u32           dwCaps;
    u32           dwCaps2;
    u32           dwCaps3;
    u32           dwCaps4;
    u32           dwReserved2;
} DDS_HEADER;
static_assert(sizeof(DDS_HEADER) == 124, "Incorrect structure size!");

TextureCube::TextureCube(const char* fileLocation)
{
    glGenTextures(1, &m_handle);
    Bind();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    FILE* file;
    if (fopen_s(&file, fileLocation, "rb") != 0)
    {
        assert(false);
        return;
    }

    Defer{ fclose(file); };

    fseek(file, 0, SEEK_END);
    auto size = ftell(file);
    fseek(file, 0, SEEK_SET);
    u8* buffer = new u8[size];
    fread(buffer, size, 1, file);
    DDS_HEADER* header = (DDS_HEADER*)((u32*)buffer + 1);
    u8* data = (u8*)(header + 1);

    i32 levels = header->dwMipMapCount;

#if 0
    void glTexStorage3D(GL_PROXY_TEXTURE_CUBE_MAP_ARRAY,
                        levels,
                        GLenum internalformat,
                        GLsizei width,
                        GLsizei height,
                        GLsizei depth);
#endif

    GLenum targets[] = {
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
        GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
        GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    for (int i = 0; i < arrsize(targets); ++i)
    {
        GLsizei width = header->dwWidth;
        GLsizei height = header->dwHeight;

        auto Align = [](GLsizei i) { return i + 3 & ~(3); };

        for (int level = 0; level < levels; ++level)
        {
            GLsizei bw = Align(width);
            GLsizei bh = Align(height);
            GLsizei byte_size = bw * bh;

            glCompressedTexImage2D(targets[i],
                                    level,
                                    GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                                    width,
                                    height,
                                    0,
                                    byte_size,
                                    data);

            data += byte_size;
            width = Max(width >> 1, 1);
            height = Max(height >> 1, 1);
        }
    }
}

void TextureCube::Bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_handle);
}
#endif

void RenderPresent()
{
    s_dx11.swap_chain.handle->Present(1, 0);
}

void DrawPathTracedVoxels()
{
    ID3D11DeviceContext* context = s_dx11.device_context;
    DX11Shader* shader          = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+Shader::Index_Voxel]);
    DX11Texture* voxel_indices  = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices]);
    DX11Texture* voxel_indices_mip1 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip1]);
    DX11Texture* voxel_indices_mip2 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip2]);
    DX11Texture* voxel_indices_mip3 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip3]);
    DX11Texture* voxel_indices_mip4 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip4]);
    DX11Texture* voxel_indices_mip5 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip5]);
    DX11Texture* voxel_indices_mip6 = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices_mip6]);
    DX11Texture* random         = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Random]);
    DX11GpuBuffer* vb           = reinterpret_cast<DX11GpuBuffer*>(g_renderer.voxel_vb);
    DX11Texture* depth  = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);
    DX11Texture* target = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);

    //Bindings
    {
        g_renderer.structure_voxel_materials->Bind(SLOT_VOXEL_MATERIALS, GpuBuffer::BindLocation::Pixel);
    }

    //Input Assembler
    {
        context->IASetInputLayout(shader->m_vertex_input_layout);
        UINT strides[] = { sizeof(Vec2), };
        UINT offsets[] = { 0, };
        context->IASetVertexBuffers(0, 1, &vb->m_buffer, strides, offsets);
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    //Vertex Shader
    {
        context->VSSetShader(shader->m_vertex_shader, NULL, 0);
    }
    //Hull shader
    {
        context->HSSetShader(nullptr, nullptr, 0);
    }
    //Domain shader
    {
        context->DSSetShader(nullptr, nullptr, 0);
    }
    //Geometry shader
    {
        context->GSSetShader(nullptr, nullptr, 0);
    }

    //Rasterizer
    {
        context->RSSetState(s_dx11.rasterizer_voxel);
        D3D11_VIEWPORT view_port = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = (float)s_dx11.swap_chain.size.x,
            .Height = (float)s_dx11.swap_chain.size.y,
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
        context->RSSetViewports(1, &view_port);
    }

    //Pixel Shader
    {
        context->PSSetShader(shader->m_pixel_shader, NULL, 0);
        context->PSSetSamplers(SLOT_VOXEL_INDICES_SAMPLER,  1, &voxel_indices->m_sampler);
        context->PSSetSamplers(SLOT_RANDOM_TEXTURE_SAMPLER, 1, &random->m_sampler);

        context->PSSetShaderResources(SLOT_VOXEL_INDICES,   1, &voxel_indices->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP1,  1, &voxel_indices_mip1->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP2,  1, &voxel_indices_mip2->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP3,  1, &voxel_indices_mip3->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP4,  1, &voxel_indices_mip4->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP5,  1, &voxel_indices_mip5->m_view);
        //context->PSSetShaderResources(SLOT_VOXEL_INDICES_MIP6,  1, &voxel_indices_mip6->m_view);
        context->PSSetShaderResources(SLOT_RANDOM_TEXTURE,  1, &random->m_view);
    }

    //Output Merger
    {
        context->OMSetDepthStencilState(s_dx11.depth_stencil_state_depth, 1);
        context->OMSetRenderTargets(1, &s_dx11.hdr_rtv, depth->m_depth_stencil_view);
        context->OMSetBlendState(s_dx11.blend_state, NULL, 0xffffffff);
    }

    //Compute shader
    {
        context->CSSetShader(nullptr, nullptr, 0);
    }

    //Draw
    {
        context->Draw((UINT)vb->m_count, 0);
    }
}

void DrawFinal()
{
    ID3D11DeviceContext* context = s_dx11.device_context;
    DX11Shader* shader          = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+Shader::Index_Final_Draw]);
    DX11GpuBuffer* vb           = reinterpret_cast<DX11GpuBuffer*>(g_renderer.voxel_vb);
    DX11Texture* previous_target= reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);
    DX11Texture* previous_depth = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);

    //Input Assembler
    {
        context->IASetInputLayout(shader->m_vertex_input_layout);
        UINT strides[] = { sizeof(Vec2), };
        UINT offsets[] = { 0, };
        context->IASetVertexBuffers(0, 1, &vb->m_buffer, strides, offsets);
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    //Vertex Shader
    {
        context->VSSetShader(shader->m_vertex_shader, NULL, 0);
    }
    //Hull shader
    {
        context->HSSetShader(nullptr, nullptr, 0);
    }
    //Domain shader
    {
        context->DSSetShader(nullptr, nullptr, 0);
    }
    //Geometry shader
    {
        context->GSSetShader(nullptr, nullptr, 0);
    }

    //Rasterizer
    {
        context->RSSetState(s_dx11.rasterizer_voxel);
        D3D11_VIEWPORT view_port = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = (float)s_dx11.swap_chain.size.x,
            .Height = (float)s_dx11.swap_chain.size.y,
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
        context->RSSetViewports(1, &view_port);
    }

    //NOTE(CSH): The output merger steps need to be done first for the final draw since 
    //we are using the previously bound render target as the input to the this.
    //Output Merger
    {
        //context->OMSetDepthStencilState(s_dx11.swap_chain.depth_stencil_state, 1);
        context->OMSetDepthStencilState(s_dx11.depth_stencil_state_no_depth, 1);
        context->OMSetRenderTargets(1, &s_dx11.swap_chain.render_target_view, NULL);
        context->OMSetBlendState(s_dx11.blend_state, NULL, 0xffffffff);
    }

    //Pixel Shader
    {
        context->PSSetShader(shader->m_pixel_shader, NULL, 0);
        context->PSSetSamplers(SLOT_PREVIOUS_TARGET_SAMPLER,1, &previous_target->m_sampler);
        context->PSSetSamplers(SLOT_PREVIOUS_DEPTH_SAMPLER, 1, &previous_depth->m_sampler);
        context->PSSetShaderResources(SLOT_PREVIOUS_TARGET, 1, &previous_target->m_view);
        context->PSSetShaderResources(SLOT_PREVIOUS_DEPTH,  1, &previous_depth->m_view);
    }

    //Compute shader
    {
        context->CSSetShader(nullptr, nullptr, 0);
    }

    //Draw
    {
        context->Draw((UINT)vb->m_count, 0);
    }
}




//**********************
// Primitives Render
//**********************

template<typename T>
void DrawPrimitiveInternal(
    std::vector<T>& verts_to_draw, 
    ID3D11RasterizerState* rasterizer, 
    Texture::Index texture_i,
    Shader::Index shader_i,
    GpuBuffer* vertex_buffer)
{
    if (verts_to_draw.size() == 0)
        return;

    ID3D11DeviceContext* context= s_dx11.device_context;
    DX11Shader* shader      = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+shader_i]);
    DX11Texture* texture    = reinterpret_cast<DX11Texture*>(g_renderer.textures[+texture_i]);
    DX11GpuBuffer* vb       = reinterpret_cast<DX11GpuBuffer*>(vertex_buffer);
    DX11Texture* depth      = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);
    DX11Texture* target     = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);

    {
        ZoneScopedN("Upload");
        vb->Upload(verts_to_draw);
    }

    //Bindings
    {
    }
    //Input Assembler
    {
        context->IASetInputLayout(shader->m_vertex_input_layout);
        UINT strides[] = { sizeof(T), };
        UINT offsets[] = { 0, };
        context->IASetVertexBuffers(0, 1, &vb->m_buffer, strides, offsets);
        context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    //Vertex Shader
    {
        context->VSSetShader(shader->m_vertex_shader, NULL, 0);
    }
    //Hull shader
    {
        context->HSSetShader(nullptr, nullptr, 0);
    }
    //Domain shader
    {
        context->DSSetShader(nullptr, nullptr, 0);
    }
    //Geometry shader
    {
        context->GSSetShader(nullptr, nullptr, 0);
    }

    //Rasterizer
    {
        context->RSSetState(rasterizer);
        D3D11_VIEWPORT view_port = {
            .TopLeftX = 0.0f,
            .TopLeftY = 0.0f,
            .Width = (float)s_dx11.swap_chain.size.x,
            .Height = (float)s_dx11.swap_chain.size.y,
            .MinDepth = 0.0f,
            .MaxDepth = 1.0f,
        };
        context->RSSetViewports(1, &view_port);
    }

    //Pixel Shader
    {
        context->PSSetShader(shader->m_pixel_shader, NULL, 0);
        context->PSSetSamplers(SLOT_PRIMITIVE_TEXTURE_SAMPLER,  1, &texture->m_sampler);
        context->PSSetShaderResources(SLOT_PRIMITIVE_TEXTURE,   1, &texture->m_view);
    }

    //Output Merger
    {
        context->OMSetDepthStencilState(s_dx11.depth_stencil_state_depth, 1);
        context->OMSetRenderTargets(1, &s_dx11.hdr_rtv, depth->m_depth_stencil_view);
        context->OMSetBlendState(s_dx11.blend_state, NULL, 0xffffffff);
    }

    //Compute shader
    {
        context->CSSetShader(nullptr, nullptr, 0);
    }

    //Draw
    {
        const size_t indices_per_face = 6;
        const size_t faces_per_cube = 6;
        const UINT total_indices_to_draw = UINT(verts_to_draw.size() * indices_per_face * faces_per_cube);
        //context->DrawIndexed(UINT((verts_to_draw.size() / 24) * 36), 0, 0);
        context->Draw(UINT(verts_to_draw.size()), 0);
    }
    verts_to_draw.clear();
}




//**********************
// Add Cubes To Render
//**********************
std::vector<Vertex_Cube> s_cubesToDraw_transparent;
std::vector<Vertex_Cube> s_cubesToDraw_opaque;
std::vector<Vertex_Cube> s_cubesToDraw_wireframe;


void AddCubeToRender(Vec3 p, Color color, Vec3  scale, bool wireframe)
{
    assert(Abs(scale) == scale);
    Vertex_Cube c;

    auto* list = &s_cubesToDraw_opaque;
    if (wireframe)
    {
        list = &s_cubesToDraw_wireframe;
    }
    else if (color.a != 1.0f)
    {
        list = &s_cubesToDraw_transparent;
    }

    for (i32 f = 0; f < +Face::Count; f++)
        for (i32 v = 0; v < 6; v++)
        {
            c.p = p + HadamardProduct(vertices_cube_full[f * 6 + v].p, scale);
            c.color = color;
            c.uv = uv_coordinates_full[v];
            list->push_back(c);
        }
}







//**********************
// Add Tetrahedron To Render
//**********************

#define TETRA_TIP   { +0.0f, +0.5f, +0.0f }
#define TETRA_TOP   { +0.0f, -0.5f, +0.5f }
#define TETRA_LEFT  { -0.5f, -0.5f, -0.5f }
#define TETRA_RIGHT { +0.5f, -0.5f, -0.5f }
const Vec3 tetrahedron_positions[] = {
    TETRA_LEFT, //bottom
    TETRA_RIGHT,
    TETRA_TOP,

    TETRA_LEFT, //z
    TETRA_TOP,
    TETRA_TIP,

    TETRA_RIGHT,//x
    TETRA_LEFT,
    TETRA_TIP,

    TETRA_TOP,  //xz
    TETRA_RIGHT,
    TETRA_TIP,
};
#undef TETRA_TIP
#undef TETRA_TOP
#undef TETRA_LEFT
#undef TETRA_RIGHT

std::vector<Vertex_Tetra> s_tetrasToDraw_transparent;
std::vector<Vertex_Tetra> s_tetrasToDraw_opaque;
std::vector<Vertex_Tetra> s_tetrasToDraw_wireframe;

void AddTetrahedronToRender(const Vec3 p, const Vec3 dir, Color color, Vec3  scale, bool wireframe)
{
    assert(Abs(scale) == scale);
    assert(dir.x != 0 || dir.y != 0 || dir.z != 0);


    auto* list = &s_tetrasToDraw_opaque;
    if (wireframe)
    {
        list = &s_tetrasToDraw_wireframe;
    }
    else if (color.a != 1.0f)
    {
        list = &s_tetrasToDraw_transparent;
    }

    Quat rotate_to_correct_forward = gb_quat_axis_angle({ -1, 0, 0 }, -tau / 4);
    Quat rot = OrientationForDirectionAndUp(Normalize(dir), { 0, 1, 0 });
    Vertex_Tetra v[3] = {};
    for (i32 i = 0; i < arrsize(tetrahedron_positions); i+= 3)
    {
        for (i32 j = 0; j < arrsize(v); j++)
        {
            Vec3 scaled = HadamardProduct(tetrahedron_positions[i + j], scale);
            Vec3 forward_aligned = gb_quat_rotate_vec3(rotate_to_correct_forward, scaled);
            Vec3 rotated = gb_quat_rotate_vec3(rot, forward_aligned);
            v[j].p = p + rotated;
        }
        Vec3 normal = Normalize(CrossProduct(v[1].p - v[0].p, v[2].p - v[0].p));
        for (i32 j = 0; j < arrsize(v); j++)
        {
            v[j].color = color;
            v[j].n = normal;
            list->push_back(v[j]);
        }
    }
}

void DrawPrimitives()
{
    ZoneScopedN("Render Primitives");
    g_renderer.cb_common->Bind(SLOT_CB_COMMON, GpuBuffer::BindLocation::All);
    DrawPrimitiveInternal(s_tetrasToDraw_opaque,      s_dx12.rasterizer_full,     Texture::Index_Plain, Shader::Index_Tetra,   g_renderer.tetra_vb);
    DrawPrimitiveInternal(s_cubesToDraw_opaque,       s_dx12.rasterizer_full,     Texture::Index_Plain, Shader::Index_Cube,    g_renderer.cube_vb);
    DrawPrimitiveInternal(s_tetrasToDraw_transparent, s_dx12.rasterizer_full,     Texture::Index_Plain, Shader::Index_Tetra,   g_renderer.tetra_vb);
    DrawPrimitiveInternal(s_cubesToDraw_transparent,  s_dx12.rasterizer_full,     Texture::Index_Plain, Shader::Index_Cube,    g_renderer.cube_vb);
    DrawPrimitiveInternal(s_tetrasToDraw_wireframe,   s_dx12.rasterizer_wireframe,Texture::Index_Plain, Shader::Index_Tetra,   g_renderer.tetra_vb);
    DrawPrimitiveInternal(s_cubesToDraw_wireframe,    s_dx12.rasterizer_wireframe,Texture::Index_Plain, Shader::Index_Cube,    g_renderer.cube_vb);
}

const SDL_MessageBoxColorScheme colorScheme = {
    /* .colors (.r, .g, .b) */
       /* [SDL_MESSAGEBOX_COLOR_BACKGROUND] */
    {{ 200, 200, 200 },
    /* [SDL_MESSAGEBOX_COLOR_TEXT] */
    {   0,   0,   0 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BORDER] */
    { 100, 100, 100 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_BACKGROUND] */
    { 220, 220, 220 },
    /* [SDL_MESSAGEBOX_COLOR_BUTTON_SELECTED] */
    { 240, 240, 240 }}
};

i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const char*  title, const char* message)
{
    SDL_MessageBoxData messageBoxData = {
        .flags = u32(type),
        .window = NULL,
        .title = title, //an UTF-8 title
        .message = message, //an UTF-8 message text
        .numbuttons = numOfButtons, //the number of buttons
        .buttons = buttons, //an array of SDL_MessageBoxButtonData with length of numbuttons
        .colorScheme = &colorScheme
    };

    i32 buttonID = 0;

    if (SDL_ShowMessageBox(&messageBoxData, &buttonID))
    {
        FAIL;
    }
    if (buttonID == -1)
    {
        FAIL;
    }
    return buttonID;
}

i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const wchar_t* title, const wchar_t* message)
{

    std::string title_mb;
    ConvertWideCharToMultiByte(title_mb, title);
    std::string message_mb;
    ConvertWideCharToMultiByte(message_mb, message);

    SDL_MessageBoxData messageBoxData = {
        .flags = u32(type),
        .window = NULL,
        .title = title_mb.c_str(),      //an UTF-8 title
        .message = message_mb.c_str(),  //an UTF-8 message text
        .numbuttons = numOfButtons,     //the number of buttons
        .buttons = buttons,             //an array of SDL_MessageBoxButtonData with length of numbuttons
        .colorScheme = &colorScheme
    };

    i32 buttonID = 0;

    if (SDL_ShowMessageBox(&messageBoxData, &buttonID))
    {
        FAIL;
    }
    if (buttonID == -1)
    {
        FAIL;
    }
    return buttonID;
}
#endif
