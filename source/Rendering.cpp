#include "Rendering.h"
#include "Misc.h"
#include "WinInterop_File.h"
#include "Vox.h"
#include "imgui.h"
#include "ImGui/backends/imgui_impl_sdl2.h"
#include "ImGui/backends/imgui_impl_dx11.h"
#include "stb/stb_image.h"

#include "SDL.h"
#include "SDL_syswm.h"
#include "Tracy.hpp"

// DirectX
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
//#ifdef _MSC_VER
//#pragma comment(lib, "d3dcompiler") // Automatically link with d3dcompiler.lib as we are using D3DCompile() below.
//#endif

Renderer g_renderer;

struct SwapChain {
    IDXGISwapChain*         handle              = nullptr;
    ID3D11RenderTargetView* render_target_view  = nullptr;

    Vec2I   size;
    u32     refresh_rate;
    u32     sample_count;
    u32     sample_quality;
};

struct DX11Data {
    ID3D11Device*           device;
    ID3D11DeviceContext*    device_context;
    IDXGIFactory*           factory;
    SwapChain               swap_chain;
    ID3D11BlendState*       blend_state;
    ID3D11RasterizerState*  rasterizer_full;
    ID3D11RasterizerState*  rasterizer_wireframe;
    ID3D11RasterizerState*  rasterizer_voxel;
    ID3D11DepthStencilState* depth_stencil_state_depth      = nullptr;
    ID3D11DepthStencilState* depth_stencil_state_no_depth   = nullptr;

    ID3D11RenderTargetView* hdr_rtv = nullptr;

    HRESULT(*D3DCompileFunc)        (LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
    HRESULT(*D3DCompileFromFileFunc)(LPCWSTR, const D3D_SHADER_MACRO*, ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT, ID3DBlob**, ID3DBlob**);
};
static DX11Data s_dx11 = {};

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
void ReportDX11References()
{
    ID3D11Debug* debug_interface;
    s_dx11.device->QueryInterface(__uuidof(ID3D11Debug), (void**)&debug_interface);
    HRESULT result = debug_interface->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
    assert(SUCCEEDED(result));
}
#else
void ReportDX11References() {};
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
    VALIDATE(*texture == nullptr);
    DX11Texture* tex = reinterpret_cast<DX11Texture*>(*texture);
    switch (tex->m_dimension)
    {
    case Texture::Dimension_1D: SafeRelease(tex->m_texture1D); break;
    case Texture::Dimension_2D: SafeRelease(tex->m_texture2D); break;
    case Texture::Dimension_3D: SafeRelease(tex->m_texture3D); break;
    }
    if (tex->m_type == Texture::Type_Depth)
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
}

bool CreateTexture(Texture** texture, void* data, Vec3I size, Texture::Format format, i32 bytes_per_pixel)
{
    Texture::TextureParams tp = {};
    tp.size = size;
    tp.bytes_per_pixel = bytes_per_pixel;
    tp.format = format;
    tp.data = data;
    bool r = CreateTexture(texture, tp);
    DEBUG_LOG("Texture Created\n");
    return r;
}
bool CreateTexture(Texture** texture, const char* fileLocation, Texture::Format format, Texture::Filter filter)
{
    Texture::TextureParams tp = {};
    u8* data = stbi_load(fileLocation, &tp.size.x, &tp.size.y, &tp.bytes_per_pixel, STBI_rgb_alpha);
    tp.format = format;
    tp.data = data;
    tp.filter = filter;
    bool r = CreateTexture(texture, tp);
    stbi_image_free(data);
    DEBUG_LOG("Texture Created\n");
    return r;
}
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp)
{
    VALIDATE_V(texture, false);
    VALIDATE_V(*texture == nullptr, false);
    DX11Texture* tex = new DX11Texture;
    *texture = tex;

    tex->m_parameters = tp;
    tex->m_size = tp.size;
    tex->m_bytes_per_pixel = tp.bytes_per_pixel;
    tex->m_type = tp.type;
    assert(tex->m_size.x != -1 && tex->m_size.x != 0);

    if (tex->m_size.z > 0)
    {
        tex->m_dimension = Texture::Dimension_3D;
    }
    else if (tex->m_size.y > 0)
    {
        tex->m_dimension = Texture::Dimension_2D;
    }
    else
    {
        tex->m_dimension = Texture::Dimension_1D;
    }
    switch (tp.format)
    {
    case Texture::Format_R11G11B10_FLOAT:       tex->m_format = DXGI_FORMAT_R11G11B10_FLOAT;      break;
    case Texture::Format_D32_FLOAT:             tex->m_format = DXGI_FORMAT_D32_FLOAT;            break;
    case Texture::Format_D16_UNORM:             tex->m_format = DXGI_FORMAT_D16_UNORM;            break;
    case Texture::Format_R8G8B8A8_UNORM:        tex->m_format = DXGI_FORMAT_R8G8B8A8_UNORM;       break;
    case Texture::Format_R8G8B8A8_UNORM_SRGB:   tex->m_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;  break;
    case Texture::Format_R8G8B8A8_UINT:         tex->m_format = DXGI_FORMAT_R8G8B8A8_UINT;        break;
    case Texture::Format_R8_UINT:               tex->m_format = DXGI_FORMAT_R8_UINT;              break;
    default: FAIL;                              tex->m_format = DXGI_FORMAT_UNKNOWN;              break;
    }

    switch (tex->m_type)
    {
    case Texture::Type_Depth:
    {
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = (u32)tex->m_size.x;
        desc.Height = (u32)tex->m_size.y;
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

        HRESULT result = s_dx11.device->CreateTexture2D(&desc, NULL, &tex->m_texture2D);
        assert(SUCCEEDED(result));
    }
    {

        D3D11_DEPTH_STENCIL_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Format = tex->m_format;
        desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;

        // Create the depth stencil view
        HRESULT result = s_dx11.device->CreateDepthStencilView(
            tex->m_texture2D,               // Depth stencil texture
            &desc,                          // Depth stencil desc
            &tex->m_depth_stencil_view);    // [out] Depth stencil view
        assert(SUCCEEDED(result));
    }
    return true;
    }


    assert(tex->m_bytes_per_pixel);

    //Create Texture
    switch (tex->m_dimension)
    {
    case Texture::Dimension_1D:
    {
        {
            D3D11_TEXTURE1D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_size.x;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = tex->m_format;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = tp.data;
            subResource.SysMemPitch = desc.Width * tex->m_bytes_per_pixel;
            subResource.SysMemSlicePitch = 0;

            HRESULT result = s_dx11.device->CreateTexture1D(&desc, &subResource, &tex->m_texture1D);
            assert(SUCCEEDED(result));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE1D;
            desc.Texture1D.MipLevels = 1;
            desc.Texture1D.MostDetailedMip = 0;
            HRESULT result = s_dx11.device->CreateShaderResourceView(tex->m_texture1D, &desc, &tex->m_view);
            assert(SUCCEEDED(result));
        }
        break;
    }
    case Texture::Dimension_2D:
    {
        //Create Texture
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_size.x;
            desc.Height = (u32)tex->m_size.y;
            desc.MipLevels = desc.ArraySize = 1;
            desc.Format = tex->m_format;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            if (tp.render_target)
                desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA sub_resource_actual;
            D3D11_SUBRESOURCE_DATA* sub_resource = nullptr;
            if (tp.data)
            {
                sub_resource_actual.pSysMem = tp.data;
                sub_resource_actual.SysMemPitch = desc.Width * tex->m_bytes_per_pixel;
                sub_resource_actual.SysMemSlicePitch = 0;
                sub_resource = &sub_resource_actual;
            }

            HRESULT result = s_dx11.device->CreateTexture2D(&desc, sub_resource, &tex->m_texture2D);
            assert(SUCCEEDED(result));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
            desc.Texture2D.MipLevels = 1;
            desc.Texture2D.MostDetailedMip = 0;
            HRESULT result = s_dx11.device->CreateShaderResourceView(tex->m_texture2D, &desc, &tex->m_view);
            assert(SUCCEEDED(result));
        }
        break;
    }
    case Texture::Dimension_3D:
    {
        //Create Texture
        {
            D3D11_TEXTURE3D_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Width = (u32)tex->m_size.x;
            desc.Height = (u32)tex->m_size.y;
            desc.Depth = (u32)tex->m_size.z;
            desc.MipLevels = 1;
            desc.Format = tex->m_format;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;
            desc.MiscFlags = 0;

            D3D11_SUBRESOURCE_DATA subResource;
            subResource.pSysMem = tp.data;
            subResource.SysMemPitch = desc.Width * tex->m_bytes_per_pixel;
            subResource.SysMemSlicePitch = desc.Height * subResource.SysMemPitch;

            HRESULT result = s_dx11.device->CreateTexture3D(&desc, &subResource, &tex->m_texture3D);
            assert(SUCCEEDED(result));
        }

        //Create View
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Format = tex->m_format;
            desc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE3D;
            desc.Texture3D.MipLevels = 1;
            desc.Texture3D.MostDetailedMip = 0;
            HRESULT result = s_dx11.device->CreateShaderResourceView(tex->m_texture3D, &desc, &tex->m_view);
            assert(SUCCEEDED(result));
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
        HRESULT result = s_dx11.device->CreateSamplerState(&desc, &tex->m_sampler);
        assert(SUCCEEDED(result));
    }
    return true;
}





//************
//Buffer
//************

struct DX11GpuBuffer : public GpuBuffer
{
    //D3D11_USAGE m_usage = D3D11_USAGE_DYNAMIC;
    ID3D11Buffer* m_buffer = nullptr;
    ID3D11ShaderResourceView* structure_resource_view = nullptr;
    //D3D11_BIND_FLAG m_target = {};
};

//void GpuBuffer::UploadData(const void* data, u32 element_size, size_t count)
//TODO: Clean this up with Type::Vertex = D3D11_BIND_VERTEX_BUFFER
void GpuBuffer::Upload(const void* data, const size_t count, const u32 element_size, const bool is_byte_format)
{
    DX11GpuBuffer* buf = reinterpret_cast<DX11GpuBuffer*>(this);
    m_count = count;
    SafeRelease(buf->m_buffer);
    assert(data);
    assert(element_size);
    assert(buf->m_type != GpuBuffer::Type::Invalid);
    VALIDATE(count);
    UINT total_bytes = UINT(element_size * count);
    //assert(total_bytes / 16 == 0);
    UINT buffer_type = 0;
    UINT cpu_access_flags = 0;
    UINT struct_byte_stride = 0;
    UINT memory_pitch = 0;
    UINT misc_flags = 0;
    switch (buf->m_type)
    {
    case GpuBuffer::Type::Vertex:
        buffer_type = D3D11_BIND_VERTEX_BUFFER;
        break;
    case GpuBuffer::Type::Index:
        buffer_type = D3D11_BIND_INDEX_BUFFER;
        break;
    case GpuBuffer::Type::Constant:
        buffer_type = D3D11_BIND_CONSTANT_BUFFER;
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        break;
    case GpuBuffer::Type::Structure:
        cpu_access_flags = D3D11_CPU_ACCESS_WRITE;
        misc_flags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        buffer_type = D3D11_BIND_SHADER_RESOURCE;
        assert(!buf->m_is_dymamic);
        struct_byte_stride = element_size;
        break;
    default:
        FAIL;
    }

    if (!buf->m_buffer)
    {
        {
            D3D11_BUFFER_DESC desc;
            desc.ByteWidth = total_bytes;
            desc.Usage = buf->m_is_dymamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
            desc.BindFlags = buffer_type;
            desc.CPUAccessFlags = buf->m_is_dymamic ? D3D11_CPU_ACCESS_WRITE | cpu_access_flags : cpu_access_flags;
            desc.MiscFlags = misc_flags;
            desc.StructureByteStride = struct_byte_stride;

            D3D11_SUBRESOURCE_DATA dx11_data;
            dx11_data.pSysMem = data;
            dx11_data.SysMemPitch = memory_pitch;
            dx11_data.SysMemSlicePitch = 0;

            HRESULT result = s_dx11.device->CreateBuffer(
                &desc,          //[in]            const D3D11_BUFFER_DESC * pDesc,
                &dx11_data,     //[in, optional]  const D3D11_SUBRESOURCE_DATA * pInitialData,
                &buf->m_buffer  //[out, optional] ID3D11Buffer * *ppBuffer
            );
            assert(SUCCEEDED(result));
        }
        DEBUG_LOG("Created and Uploaded data to gpu buffer: element: %i size: %i", element_size, count);

        if (buf->m_type == GpuBuffer::Type::Structure)
        {
            D3D11_SHADER_RESOURCE_VIEW_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
            desc.Format = is_byte_format ? DXGI_FORMAT_R8_UINT : DXGI_FORMAT_UNKNOWN;
            desc.ViewDimension = D3D_SRV_DIMENSION_BUFFER;
            desc.Buffer.FirstElement = 0;
            desc.Buffer.NumElements = (UINT)count;
            HRESULT result = s_dx11.device->CreateShaderResourceView(
                buf->m_buffer,                  //[in]            ID3D11Resource * pResource,
                &desc,                          //[in, optional]  const D3D11_SHADER_RESOURCE_VIEW_DESC * pDesc,
                &buf->structure_resource_view   //[out, optional] ID3D11ShaderResourceView * *ppSRView
            );
            assert(SUCCEEDED(result));
        }

        return;
    }

    if (buf->m_is_dymamic)
    {
        //map/unmap/memcopy
        D3D11_MAPPED_SUBRESOURCE resource;
        ZeroMemory(&resource, sizeof(D3D11_MAPPED_SUBRESOURCE));
        HRESULT result = s_dx11.device_context->Map(
            buf->m_buffer,          //[in]            ID3D11Resource * pResource,
            0,                      //[in]            UINT                     Subresource,
            D3D11_MAP_WRITE_DISCARD,//[in]            D3D11_MAP                MapType,
            0,                      //[in]            UINT                     MapFlags,
            &resource               //[out, optional] D3D11_MAPPED_SUBRESOURCE * pMappedResource
        );
        assert(SUCCEEDED(result));
        memcpy(resource.pData, data, element_size * count);
        s_dx11.device_context->Unmap(buf->m_buffer, 0);
        DEBUG_LOG("Uploaded dynamic_buffer data to gpu buffer: element: %i size: %i", element_size, count);
    }
    else
    {
        s_dx11.device_context->UpdateSubresource(
            buf->m_buffer,  //[in]           ID3D11Resource * pDstResource,
            0,              //[in]           UINT            DstSubresource,
            NULL,           //[in, optional] const D3D11_BOX * pDstBox,
            data,           //[in]           const void* pSrcData,
            total_bytes,    //[in]           UINT            SrcRowPitch,
            0               //[in]           UINT            SrcDepthPitch
        );
        DEBUG_LOG("Uploaded default_buffer data to gpu buffer: element: %i size: %i", element_size, count);
    }

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
    DX11GpuBuffer* buf = new DX11GpuBuffer;
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
        if (pData == nullptr)
            return E_FAIL;
        return S_OK;
    }
};


struct DX11Shader : public ShaderProgram
{
    //D3D11_USAGE m_usage = D3D11_USAGE_DYNAMIC;
    ID3D11Buffer* m_buffer = nullptr;
    ID3D11ShaderResourceView* structure_resource_view = nullptr;
    ID3D11InputLayout* m_vertex_input_layout = nullptr;
    ID3D11VertexShader* m_vertex_shader = nullptr;
    ID3D11PixelShader* m_pixel_shader = nullptr;
    static const u32 m_vertex_component_max = 4;
    D3D11_INPUT_ELEMENT_DESC m_local_layout[m_vertex_component_max] = {};

    //D3D11_BIND_FLAG m_target = {};
};

bool CreateShader(ShaderProgram** s,
    const std::string& vertexFileLocation,
    const std::string& pixelFileLocation,
    ShaderProgram::InputElementDesc* input_layout,
    i32 layout_count)
{
    assert(s);
    assert(*s == nullptr);
    DX11Shader* shader = new DX11Shader;
    (*s) = reinterpret_cast<DX11Shader*>(shader);

    shader->m_vertexFile = vertexFileLocation;
    shader->m_pixelFile = pixelFileLocation;

    shader->m_vertex_component_count = layout_count;
    for (u32 i = 0; i < shader->m_vertex_component_count; i++)
    {
        shader->m_local_layout[i] = {
        .SemanticName = input_layout[i].SemanticName,
        .SemanticIndex = 0,
        .Format = DXGI_FORMAT(input_layout[i].Format),
        .InputSlot = 0,
        .AlignedByteOffset = input_layout[i].AlignedByteOffset,
        .InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA,
        .InstanceDataStepRate = 0,
        };

    }

    shader->CheckForUpdate();
    return true;
}
ShaderProgram::~ShaderProgram()
{
    DX11Shader* shader = reinterpret_cast<DX11Shader*>(this);
    SafeRelease(shader->m_vertex_shader);
    SafeRelease(shader->m_pixel_shader);
    SafeRelease(shader->m_vertex_input_layout);
    DEBUG_LOG("Shader Program Deleted\n");
}
bool ShaderProgram::CompileShader(std::string text, const std::string& file_name, ShaderType shader_type)
{
    DX11Shader* shader = reinterpret_cast<DX11Shader*>(this);
    bool failed = false;
#if 1
    D3D_SHADER_MACRO* shader_macros = nullptr;
#else
    D3D_SHADER_MACRO shader_macros[] = {
        //{"RAY_TRACING", "1"},
    };
#endif

    //WideCharToMultiByte
    i32 char_count = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        file_name.c_str(),      //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        nullptr,                //[out, optional] LPWSTR                            lpWideCharStr,
        0                       //[in]            int                               cchWideChar
    );
    i32 wide_char_count = char_count;// = char_count / 2;
    WCHAR* wide_char = new WCHAR[wide_char_count];
    //memset(wide_char, '\0', wide_char_count);
    i32 wide_char_actual = MultiByteToWideChar(
        CP_UTF8,                //[in]            UINT                              CodePage,
        MB_ERR_INVALID_CHARS,   //[in]            DWORD                             dwFlags,
        file_name.c_str(),      //[in]            _In_NLS_string_(cbMultiByte)LPCCH lpMultiByteStr,
        -1,                     //[in]            int                               cbMultiByte,
        wide_char,              //[out, optional] LPWSTR                            lpWideCharStr,
        wide_char_count         //[in]            int                               cchWideChar
    );
    assert(wide_char_actual > 0);
    assert(wide_char_actual == wide_char_count);

    std::string entry_point;
    std::string target_version;
    switch (shader_type)
    {
    case Type_Vertex:
        entry_point = "Vertex_Main";
        target_version = "vs_4_0";
        break;
    case Type_Pixel:
        entry_point = "Pixel_Main";
        target_version = "ps_4_0";
        break;
    default:
        FAIL;
    }

    u32 flags1 = D3DCOMPILE_WARNINGS_ARE_ERRORS | D3DCOMPILE_PREFER_FLOW_CONTROL;
#if _DEBUG
    //;
    flags1 |= D3DCOMPILE_DEBUG;
#if 0
    flags1 |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_OPTIMIZATION_LEVEL0;
    //flags1 |= D3DCOMPILE_SKIP_VALIDATION; //dont do this
#else
    flags1 |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
#endif

    ID3DBlob* code;
    ID3DBlob* errors;
    DX11IncludeManager include_manager;
    HRESULT compile_result = s_dx11.D3DCompileFromFileFunc(
        wide_char,              //[in]            LPCWSTR                pFileName,
        shader_macros,          //[in, optional]  const D3D_SHADER_MACRO *pDefines,
        &include_manager,  //[in, optional]  ID3DInclude            *pInclude,
        entry_point.c_str(),    //[in]            LPCSTR                 pEntrypoint,
        target_version.c_str(), //[in]            LPCSTR                 pTarget,
        flags1,                 //[in]            UINT                   Flags1,
        0,                      //[in]            UINT                   Flags2,
        &code,                  //[out]           ID3DBlob               **ppCode,
        &errors                 //[out, optional] ID3DBlob               **ppErrorMsgs
    );
    delete wide_char;
    failed = !code || !!errors || FAILED(compile_result);
    //assert(code);

    if (!failed)
    {
        SafeRelease(errors);
        switch (shader_type)
        {
        case Type_Vertex:
            SafeRelease(shader->m_vertex_shader);
            if (FAILED(s_dx11.device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader->m_vertex_shader)))
            {
                FAIL;
                SafeRelease(code);
                SafeRelease(errors);
                failed = true;
            }
            else
            {
                // Create the input layout
                if (FAILED(s_dx11.device->CreateInputLayout(
                    shader->m_local_layout,             //[in]            const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
                    shader->m_vertex_component_count,   //[in]            UINT                           NumElements,
                    code->GetBufferPointer(),           //[in]            const void                     *pShaderBytecodeWithInputSignature,
                    code->GetBufferSize(),              //[in]            SIZE_T                         BytecodeLength,
                    &shader->m_vertex_input_layout      //[out, optional] ID3D11InputLayout              **ppInputLayout
                )))
                {
                    FAIL;
                    failed = true;
                }
                SafeRelease(code);
            }
            break;
        case Type_Pixel:
            SafeRelease(shader->m_pixel_shader);
            if (FAILED(s_dx11.device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &shader->m_pixel_shader)))
            {
                FAIL;
                SafeRelease(code);
                SafeRelease(errors);
                failed = true;
            }
            break;
        default:
            FAIL;
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

    if (failed)
    {
        std::string info_string;
        info_string.resize(errors->GetBufferSize());
        memcpy(info_string.data(), errors->GetBufferPointer(), errors->GetBufferSize());
        std::string error_title = file_name + " Compilation Error: ";
        DebugPrint((error_title + info_string + "\n").c_str());

        SDL_MessageBoxButtonData buttons[] = {
            //{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Continue" },
        };

        i32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), MessageBoxType::Error, error_title.c_str(), info_string.c_str());
        //if (buttons[buttonID].buttonid == 2)//NOTE: Stop button
        //{
        //    DebugPrint("stop hit");
        //    g_running = false;
        //    return false;
        //}
        //else
        {
            if (buttons[buttonID].buttonid == 0)//NOTE: Retry button
            {
                CheckForUpdate();
            }
            else if (buttons[buttonID].buttonid == 1)//NOTE: Continue button
            {
                return false;
            }
        }
    }
    DEBUG_LOG("Shader Vertex/Fragment Created\n");
    return true;
}

void GetShaderReferenceFileTimes(std::vector<u64>& out, ShaderProgram* p)
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

void ShaderProgram::CheckForUpdate()
{
    std::string vertexText;
    u64 vertexFileTime;
    std::string pixelText;
    u64 pixelFileTime;
    std::vector<u64> referenced_file_times;
    {

        File vertexFile(m_vertexFile, File::Mode::Read, false);
        vertexFile.GetTime();
        VALIDATE(vertexFile.m_timeIsValid);
        vertexFile.GetText();
        vertexText = vertexFile.m_dataString;
        vertexFileTime = vertexFile.m_time;

        File pixelFile(m_pixelFile, File::Mode::Read, false);
        pixelFile.GetTime();
        VALIDATE(pixelFile.m_timeIsValid);
        pixelFile.GetText();
        pixelText = pixelFile.m_dataString;
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
        if (!CompileShader(vertexText, m_vertexFile, Type_Vertex) ||
            !CompileShader(pixelText, m_pixelFile, Type_Pixel))
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















void FillIndexBuffer(GpuBuffer* ib, size_t count)
{
    if (ib->m_count > count)
        return;
    std::vector<u32> arr;

    //size_t amount = VOXEL_MAX_SIZE * VOXEL_MAX_SIZE * VOXEL_MAX_SIZE * 6 * 6;
    size_t amount = 6 * count;
    arr.reserve(amount);
    i32 baseIndex = 0;
    for (i32 i = 0; i < amount; i += 6)

    {
        arr.push_back(baseIndex + 0);
        arr.push_back(baseIndex + 1);
        arr.push_back(baseIndex + 2);
        arr.push_back(baseIndex + 1);
        arr.push_back(baseIndex + 3);
        arr.push_back(baseIndex + 2);

        baseIndex += 4; //Amount of vertices
    }

    ib->Upload(arr.data(), amount, sizeof(baseIndex));
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
    ImGui_ImplDX11_Init(s_dx11.device, s_dx11.device_context);

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
    //SafeRelease(s_dx11.swap_chain.depth_stencil_view);
    //SafeRelease(s_dx11.swap_chain.depth_stencil_state);
    //SafeRelease(s_dx11.swap_chain.depth_texture);
    HRESULT result = s_dx11.swap_chain.handle->ResizeBuffers(
        0,//2,                          //UINT        BufferCount, IS THIS RIGHT???
        (UINT)window_size.x,                     //UINT        Width,
        (UINT)window_size.y,                     //UINT        Height,
        DXGI_FORMAT_UNKNOWN,//DXGI_FORMAT_R8G8B8A8_UNORM, //DXGI_FORMAT NewFormat,
        0                           //UINT        SwapChainFlags
    );
    assert(SUCCEEDED(result));

    DXGI_SWAP_CHAIN_DESC desc;
    s_dx11.swap_chain.handle->GetDesc(&desc);
    assert(desc.BufferDesc.Width == window_size.x);
    assert(desc.BufferDesc.Height == window_size.y);
    s_dx11.swap_chain.size.x = desc.BufferDesc.Width;
    s_dx11.swap_chain.size.y = desc.BufferDesc.Height;
    s_dx11.swap_chain.refresh_rate = desc.BufferDesc.RefreshRate.Numerator;
    s_dx11.swap_chain.sample_count = desc.SampleDesc.Count;
    s_dx11.swap_chain.sample_quality = desc.SampleDesc.Quality;


#if 1 
    ID3D11Texture2D* backbuffer;
    HRESULT backbuffer_result = s_dx11.swap_chain.handle->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
    assert(SUCCEEDED(backbuffer_result));

    D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
    ZeroMemory(&rtv_desc, sizeof(rtv_desc));
    rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtv_desc.Texture2D.MipSlice = 0;

    VERIFY(SUCCEEDED(s_dx11.device->CreateRenderTargetView(
        backbuffer,             //[in]            ID3D11Resource* pResource,
        &rtv_desc,                  //[in, optional]  const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
        &s_dx11.swap_chain.render_target_view//[out, optional] ID3D11RenderTargetView** ppRTView
    )));
#else
    ID3D11Texture2D* backbuffer;
    s_dx11.swap_chain.handle->GetBuffer(0, IID_PPV_ARGS(&backbuffer));
    s_dx11.device->CreateRenderTargetView(backbuffer, NULL, &s_dx11.swap_chain.render_target_view);
#endif

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
            CreateTexture(t, tp);
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
            CreateTexture(t, tp);
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


    {
#if 0
        //Do we need to really do this?
        {
            IDXGIFactory* factory;
            VERIFY(SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&factory))));
            assert(factory);
            IDXGIAdapter* aOutput;
            for (UINT i = 0; factory->EnumAdapters(i, &aOutput) != DXGI_ERROR_NOT_FOUND; i++)
            {
                DXGI_ADAPTER_DESC desc;
                HRESULT r = aOutput->GetDesc(&desc);
                desc.Description;
                UINT id = desc.VendorId;
            }
        }
#endif



        UINT flags = 0;
//#if _DEBUG
        flags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

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

        DXGI_SAMPLE_DESC dxgi_sample_desc;
        {
            //MSAA
            dxgi_sample_desc.Count      = 1;
            dxgi_sample_desc.Quality    = 0;
        }

        DXGI_SWAP_CHAIN_DESC swap_chain_desc;
        swap_chain_desc.BufferDesc = dxgi_mode_desc;
        swap_chain_desc.SampleDesc = dxgi_sample_desc;
        swap_chain_desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 2; //is this right?
        swap_chain_desc.OutputWindow = hwnd;
        swap_chain_desc.Windowed = TRUE;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; //Does not work with MSAA
        swap_chain_desc.Flags = 0; //do we need this?  DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING

        const D3D_FEATURE_LEVEL feature_levels[]= { D3D_FEATURE_LEVEL_11_0 };
        IDXGISwapChain*         swap_chain      = nullptr;
        D3D_FEATURE_LEVEL       feature_level   = {};
        ID3D11DeviceContext*    temp_context    = nullptr;

        HRESULT create_device_and_swap_chain_result = D3D11CreateDeviceAndSwapChain(
            nullptr,                    //[in, optional]  IDXGIAdapter               *pAdapter,
            D3D_DRIVER_TYPE_HARDWARE,   //                D3D_DRIVER_TYPE            DriverType,
            NULL,                       //                HMODULE                    Software,
            flags,                      //                UINT                       Flags,
            feature_levels,             //[in, optional]  const D3D_FEATURE_LEVEL    *pFeatureLevels,
            arrsize(feature_levels),    //                UINT                       FeatureLevels,
            D3D11_SDK_VERSION,          //                UINT                       SDKVersion,
            &swap_chain_desc,           //[in, optional]  const DXGI_SWAP_CHAIN_DESC *pSwapChainDesc,
            &swap_chain,                //[out, optional] IDXGISwapChain             **ppSwapChain,
            &s_dx11.device,                    //[out, optional] ID3D11Device               **ppDevice,
            &feature_level,             //[out, optional] D3D_FEATURE_LEVEL          *pFeatureLevel,
            &temp_context               //[out, optional] ID3D11DeviceContext        **ppImmediateContext
        );
        assert(SUCCEEDED(create_device_and_swap_chain_result));
        assert(s_dx11.device); //FATAL

        VERIFY(SUCCEEDED(temp_context->QueryInterface(IID_PPV_ARGS(&s_dx11.device_context))));
        temp_context->Release();

        s_dx11.swap_chain.handle = swap_chain;
        UpdateSwapchain(g_renderer.size);

        // Get factory from device
        IDXGIDevice*    pDXGIDevice  = nullptr;
        IDXGIAdapter*   pDXGIAdapter = nullptr;
        IDXGIFactory*   pFactory     = nullptr;

        if (SUCCEEDED(s_dx11.device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice))))
            if (SUCCEEDED(pDXGIDevice->GetParent(IID_PPV_ARGS(&pDXGIAdapter))))
                if (SUCCEEDED(pDXGIAdapter->GetParent(IID_PPV_ARGS(&pFactory))))
                {
                    s_dx11.factory         = pFactory;
                }
        if (pDXGIDevice) pDXGIDevice->Release();
        if (pDXGIAdapter) pDXGIAdapter->Release();
        s_dx11.device->AddRef();
        s_dx11.device_context->AddRef();
    }

    {
        HINSTANCE dll_instance = LoadLibrary("d3dcompiler_47.dll");
        VALIDATE(dll_instance);
        s_dx11.D3DCompileFunc          = (D3DCompileFunc)GetProcAddress(dll_instance, "D3DCompile");
        s_dx11.D3DCompileFromFileFunc  = (D3DCompileFromFileFunc)GetProcAddress(dll_instance, "D3DCompileFromFile");
    }

#if 0 //Unsure if this is needed for creating window with SDL and D3D11
    SDL_Renderer* renderer = nullptr;
    for (i32 i = 0; i < SDL_GetNumRenderDrivers(); i++)
    {
        SDL_RendererInfo rendererInfo = {};
        SDL_GetRenderDriverInfo(i, &rendererInfo);
        if (rendererInfo.name == std::string("direct3d11"))
        {
            renderer = SDL_CreateRenderer(g_renderer.SDL_Context, i, 0);
        }

        break;
    }
#endif

#if 0 //OpenGL code
    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(g_renderer.swapInterval);
#endif

    //Create Textures:
    CreateTexture(&g_renderer.textures[Texture::Index_Minecraft], "assets/MinecraftSpriteSheet20120215Modified.png", Texture::Format_R8G8B8A8_UNORM_SRGB, Texture::Filter_Point);
    u8 pixel_texture_data[] = { 255, 255, 255, 255 };
    CreateTexture(&g_renderer.textures[Texture::Index_Plain], pixel_texture_data, { 1, 1 }, Texture::Format_R8G8B8A8_UNORM, sizeof(pixel_texture_data[0]));
    CreateTexture(&g_renderer.textures[Texture::Index_Random], "assets/random-dcode.png", Texture::Format_R8G8B8A8_UNORM, Texture::Filter_Point);

    {
        Texture::TextureParams tp = {
            .size   = ToVec3I(s_dx11.swap_chain.size, 0),
            .format = Texture::Format_D32_FLOAT,
            .mode   = Texture::Address_Invalid,
            .filter = Texture::Filter_Invalid,
            .type   = Texture::Type_Depth,
            .render_target = true,
            .bytes_per_pixel = 0,
            .data = 0,
        };
        CreateTexture(&g_renderer.textures[Texture::Index_Backbuffer_Depth], tp);
    }
    {
        Texture::TextureParams tp = {
            .size   = ToVec3I(s_dx11.swap_chain.size, 0),
            .format = Texture::Format_R11G11B10_FLOAT,
            .mode   = Texture::Address_Clamp,
            .filter = Texture::Filter_Aniso,
            .type   = Texture::Type_Texture,
            .render_target = true,
            .bytes_per_pixel = 4,
            .data = 0,
        };
        CreateTexture(&g_renderer.textures[Texture::Index_Backbuffer_HDR], tp);
    }

    //Create Shaders:
    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, (UINT)offsetof(Vertex, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "UV",         0, DXGI_FORMAT_R32G32_FLOAT,      0, (UINT)offsetof(Vertex, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,   0, (UINT)offsetof(Vertex, n),   D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
    //    g_renderer.shaders[+Shader::Main] = new ShaderProgram("Source/Shaders/Main.vert", "Source/Shaders/Main.frag", layout, arrsize(layout));
    //}
    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Voxel, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "COLOR",      0, DXGI_FORMAT_R32_UINT,        0, (UINT)offsetof(Vertex_Voxel, rgba),D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "NORMAL",     0, DXGI_FORMAT_R8_UINT,         0, (UINT)offsetof(Vertex_Voxel, n),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "AO",         0, DXGI_FORMAT_R8_UINT,         0, (UINT)offsetof(Vertex_Voxel, ao),  D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
    //    g_renderer.shaders[+Shader::Voxel_Rast] = new ShaderProgram("Source/Shaders/Voxel_Rast.vert", "Source/Shaders/Voxel_Rast.frag", layout, arrsize(layout));
    //}
    {
        //D3D11_INPUT_ELEMENT_DESC layout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
        ShaderProgram::InputElementDesc layout[] = { { "POSITION", DXGI_FORMAT_R32G32_FLOAT, 0 } };
        VERIFY(CreateShader(&g_renderer.shaders[+ShaderProgram::Index_Voxel],   "Source/Shaders/Voxel.hlsl",    layout, arrsize(layout)));
    }
    {
        ShaderProgram::InputElementDesc layout[] = {
            { "COLOR",      DXGI_FORMAT_R32G32B32A32_FLOAT, offsetof(Vertex_Cube, color)    },
            { "POSITION",   DXGI_FORMAT_R32G32B32_FLOAT,    offsetof(Vertex_Cube, p)        },
            { "TEXCOORD",   DXGI_FORMAT_R32G32_FLOAT,       offsetof(Vertex_Cube, uv)       } };
        VERIFY(CreateShader(&g_renderer.shaders[+ShaderProgram::Index_Cube],    "Source/Shaders/Cube.hlsl",     layout, arrsize(layout)));
    }
    {
        ShaderProgram::InputElementDesc layout[] = { { "POSITION", DXGI_FORMAT_R32G32_FLOAT, 0 } };
        VERIFY(CreateShader(&g_renderer.shaders[+ShaderProgram::Index_Final_Draw],   "Source/Shaders/Final_Draw.hlsl",  layout, arrsize(layout)));
    }
    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Cube, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "COLOR",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Cube, color),D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    //    g_renderer.shaders[+Shader::Cube] = new ShaderProgram("Source/Shaders/Cube.vert", "Source/Shaders/Cube.frag", layout, arrsize(layout));
    //}

    //Create Buffers:
    CreateGpuBuffer(&g_renderer.quad_ib,        "Quad_IB",          true,   GpuBuffer::Type::Index);
    FillIndexBuffer(g_renderer.quad_ib, 6 * 4);
    //CreateGpuBuffer(&g_renderer.voxel_rast_vb,  "Voxel_Rast_VB",    true,   GpuBuffer::Type::Vertex);
    //CreateGpuBuffer(&g_renderer.box_vb,         "Box_VB",           false,  GpuBuffer::Type::Vertex);
    CreateGpuBuffer(&g_renderer.cube_vb,        "Cube_VB",          false,  GpuBuffer::Type::Vertex);
    {
        float p = 0.5f;
        Vertex vertices[] = {
            // |   Position    |      UV       |         Normal        |
              { { +0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } }, // +x
              { { +0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
              { { +0.5f, +0.5f, -0.5f }, { 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },

              { { +0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
              { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
              { { +0.5f, +0.5f, -0.5f }, { 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },


              { { -0.5f, +0.5f, -0.5f }, { 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } }, // -x
              { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f,  0.0f,  0.0f } },
              { { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },

              { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, { -1.0f,  0.0f,  0.0f } },
              { { -0.5f, -0.5f, +0.5f }, { 1.0f, 0.0f }, { -1.0f,  0.0f,  0.0f } },
              { { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },


              { { +0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } }, // +y
              { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f,  1.0f,  0.0f } },
              { { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },

              { { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f,  1.0f,  0.0f } },
              { { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f }, {  0.0f,  1.0f,  0.0f } },
              { { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },


              { { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } }, // -y
              { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f, -1.0f,  0.0f } },
              { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },

              { { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f, -1.0f,  0.0f } },
              { { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f }, {  0.0f, -1.0f,  0.0f } },
              { { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },


              { { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } }, // +z
              { { -0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
              { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },

              { { -0.5f, -0.5f, +0.5f }, { 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
              { { +0.5f, -0.5f, +0.5f }, { 1.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
              { { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },


              { { +0.5f, +0.5f, -0.5f }, { 0.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } }, // -z
              { { +0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
              { { -0.5f, +0.5f, -0.5f }, { 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },

              { { +0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
              { { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
              { { -0.5f, +0.5f, -0.5f }, { 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
        };
        static_assert(arrsize(vertices) == 36, "");

        Vec3 voxel_box_vertices[arrsize(vertices)] = {};
        for (i32 i = 0; i < arrsize(vertices); i++)
        {
            voxel_box_vertices[i] = vertices[i].p;
        }
        //g_renderer.box_vb->Upload(voxel_box_vertices, arrsize(voxel_box_vertices), sizeof(voxel_box_vertices[0]));
    }
    {
        CreateGpuBuffer(&g_renderer.voxel_vb, "Voxel_VB", false, GpuBuffer::Type::Vertex);
        Vec2 a[] = {
            { -1.0f, +1.0f }, // 0
            { +3.0f, +1.0f }, // 1
            { -1.0f, -3.0f }, // 2
        };
        g_renderer.voxel_vb->Upload(a, arrsize(a), sizeof(a[0]));
    }
    CreateGpuBuffer(&g_renderer.cb_common, "common_cb", true, GpuBuffer::Type::Constant);

    //Create Blender State
    {
        D3D11_BLEND_DESC desc;
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
        s_dx11.device->CreateBlendState(&desc, &s_dx11.blend_state);
    }

    // Create the rasterizer state
    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_BACK;
        desc.FrontCounterClockwise = TRUE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthClipEnable = TRUE;
        desc.ScissorEnable = FALSE;
        desc.MultisampleEnable = TRUE;
        desc.AntialiasedLineEnable = TRUE;
        s_dx11.device->CreateRasterizerState(&desc, &s_dx11.rasterizer_full);
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
    {
        //ID3D11Texture2D* backbuffer;
        //HRESULT backbuffer_result = s_dx11.swap_chain.handle->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backbuffer);
        //assert(SUCCEEDED(backbuffer_result));

        DX11Texture* target = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);

        D3D11_RENDER_TARGET_VIEW_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Format = target->m_format;
        desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;

        VERIFY(SUCCEEDED(s_dx11.device->CreateRenderTargetView(
            target->m_texture2D,//[in]            ID3D11Resource* pResource,
            &desc,              //[in, optional]  const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
            &s_dx11.hdr_rtv     //[out, optional] ID3D11RenderTargetView** ppRTView
        )));
    }
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
        HRESULT result = s_dx11.device->CreateDepthStencilState(&desc, &s_dx11.depth_stencil_state_depth);
        assert(SUCCEEDED(result));
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
        HRESULT result = s_dx11.device->CreateDepthStencilState(&desc, &s_dx11.depth_stencil_state_no_depth);
        assert(SUCCEEDED(result));
    }

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

    //Vec2I window_size;
    //SDL_GetWindowSizeInPixels(g_renderer.SDL_Context, &window_size.x, &window_size.y);
    if (s_dx11.swap_chain.size != window_size)
        UpdateSwapchain(window_size);

    s_dx11.device_context->ClearRenderTargetView(s_dx11.swap_chain.render_target_view, srgb_to_linear(backgroundColor).e);
    s_dx11.device_context->ClearRenderTargetView(s_dx11.hdr_rtv, srgb_to_linear(backgroundColor).e);
    DX11Texture* depth = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);
    s_dx11.device_context->ClearDepthStencilView(depth->m_depth_stencil_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

    if (s_last_shader_update_time + 0.1f <= s_incremental_time)
    {
        for (ShaderProgram* s : g_renderer.shaders)
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
    DX11Shader* shader          = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+ShaderProgram::Index_Voxel]);
    DX11Texture* voxel_indices  = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Voxel_Indices]);
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

void FinalDraw()
{
    ID3D11DeviceContext* context = s_dx11.device_context;
    DX11Shader* shader          = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+ShaderProgram::Index_Final_Draw]);
    DX11GpuBuffer* vb           = reinterpret_cast<DX11GpuBuffer*>(g_renderer.voxel_vb);
    DX11Texture* previous_target= reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);
    DX11Texture* previous_depth = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);

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

void RenderCubesInternal(std::vector<Vertex_Cube>& cubes_to_draw, ID3D11RasterizerState* rasterizer, DX11Texture* texture)
{
#if 1
    if (cubes_to_draw.size() == 0)
        return;

    ID3D11DeviceContext* context= s_dx11.device_context;
    DX11Shader*     shader      = reinterpret_cast<DX11Shader*>(g_renderer.shaders[+ShaderProgram::Index_Cube]);
    DX11GpuBuffer*  vb          = reinterpret_cast<DX11GpuBuffer*>(g_renderer.cube_vb);
    DX11GpuBuffer*  ib          = reinterpret_cast<DX11GpuBuffer*>(g_renderer.quad_ib);
    DX11Texture* depth          = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_Depth]);
    DX11Texture* target         = reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Backbuffer_HDR]);

    {
        ZoneScopedN("Upload");
        FillIndexBuffer(ib, 6 * cubes_to_draw.size());
        vb->Upload(cubes_to_draw);
    }

    //Bindings
    {
    }
    //Input Assembler
    {
        context->IASetInputLayout(shader->m_vertex_input_layout);
        UINT strides[] = { sizeof(Vertex_Cube), };
        UINT offsets[] = { 0, };
        context->IASetVertexBuffers(0, 1, &vb->m_buffer, strides, offsets);
        context->IASetIndexBuffer(ib->m_buffer, DXGI_FORMAT_R32_UINT, 0);
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
        context->PSSetSamplers(SLOT_CUBE_TEXTURE_SAMPLER,  1, &texture->m_sampler);
        context->PSSetShaderResources(SLOT_CUBE_TEXTURE,   1, &texture->m_view);
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
        const UINT total_indices_to_draw = UINT(cubes_to_draw.size() * indices_per_face * faces_per_cube);
        //context->DrawIndexed(UINT((cubes_to_draw.size() / 24) * 36), 0, 0);
        context->Draw(UINT(cubes_to_draw.size()), 0);
    }
#endif
    cubes_to_draw.clear();
}
void RenderTransparentCubes()
{
    ZoneScopedN("Upload and Render Transparent Cubes");
    RenderCubesInternal(s_cubesToDraw_transparent, s_dx11.rasterizer_full, reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Plain]));
}

void RenderOpaqueCubes()
{
    ZoneScopedN("Upload and Render Opaque Cubes");
    RenderCubesInternal(s_cubesToDraw_opaque, s_dx11.rasterizer_full, reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Minecraft]));
}

void RenderWireframeCubes()
{
    ZoneScopedN("Upload and Render Wireframe Cubes");
    RenderCubesInternal(s_cubesToDraw_wireframe, s_dx11.rasterizer_wireframe, reinterpret_cast<DX11Texture*>(g_renderer.textures[Texture::Index_Plain]));
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
