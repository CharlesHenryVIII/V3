#pragma once
#include "SDL.h"
#include "Math.h"
#include "Misc.h"
//#include "Rendering_Texture.h"
#include "Vox.h"
#include "GpuSharedData.h"

#include <unordered_map>

struct ShaderProgram
{
    enum ShaderType : u32 {
        Type_Invalid,
        Type_Vertex,
        Type_Pixel,
        Type_Count,
    };
    ENUMOPS(ShaderType);

    enum ShaderIndex : u32 {
        Index_Invalid,
        Index_Main,
        Index_Voxel_Rast,
        Index_Voxel,
        Index_Cube,
        Index_Count,
    };
    ENUMOPS(ShaderIndex);

    struct InputElementDesc {
        const char* SemanticName;
        u32 Format; //DXGI_FORMAT
        u32 AlignedByteOffset;
    };

    static const u32 m_vertex_component_max = 4;

    ~ShaderProgram();
    void CheckForUpdate();

    std::string m_vertexFile;
    std::string m_pixelFile;
    u64 m_vertexLastWriteTime = {};
    u64 m_pixelLastWriteTime = {};
    u32 m_vertex_component_count = 0;

    bool CompileShader(std::string text, const std::string& file_name, ShaderType shader_type);
};
bool CreateShader(ShaderProgram** shader,
    const std::string& vertexFileLocation,
    const std::string& pixelFileLocation,
    ShaderProgram::InputElementDesc* input_layout,
    i32 layout_count);

struct GpuBuffer
{
    enum class Type : u32 {
        Invalid,
        Vertex,
        Index,
        Constant,
        Structure,
        Count,
    };
    ENUMOPS(Type);

    bool m_is_dymamic = true;
    Type m_type = GpuBuffer::Type::Invalid;
    char m_name[32];
    size_t m_count;

    void Upload(const void* data, const size_t count, const u32 element_size, const bool is_byte_format = false);
    //Bind a Constant or Structure buffer
    void Bind(u32 slot, bool for_vertex_shader);
};
bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, bool is_dynamic, GpuBuffer::Type type);
void DeleteBuffer(GpuBuffer** buffer);

struct Texture {
    enum T : u32 {
        Type_Invalid,
        Type_Minecraft,
        Type_Plain,
        Type_Voxel_Indices,
        Type_Random,
        Type_Count,
    }; ENUMOPS(T);
    enum Dimension : u32 {
        Dimension_Invalid,
        Dimension_1D,
        Dimension_2D,
        Dimension_3D,
        Dimension_Count,
    }; ENUMOPS(Dimension);
    enum Filter : u32 {
        Filter_Invalid,
        Filter_Point,
        Filter_Linear,
        Filter_Count,
    }; ENUMOPS(Filter);
    enum AddressMode : u32 {
        Address_Invalid,
        Address_Wrap,
        Address_Mirror,
        Address_Clamp,
        Address_Border,
        Address_MirrorOnce,
        Address_Count,
    }; ENUMOPS(AddressMode);
    enum Format : u32 {
        Format_Invalid,
        Format_R8G8B8A8_UNORM,
        Format_R8G8B8A8_UNORM_SRGB,
        Format_R8G8B8A8_UINT,
        Format_R8_UINT,
        Format_Count,
    }; ENUMOPS(Format);

    struct TextureParams {
        Vec3I size;
        //DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UINT;
        //D3D11_TEXTURE_ADDRESS_MODE wrap = D3D11_TEXTURE_ADDRESS_WRAP;
        //D3D11_FILTER filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Format format = Format_R8G8B8A8_UINT;
        AddressMode mode = Address_Wrap;
        Filter filter = Filter_Linear;
        i32 bytes_per_pixel;
        const void* data;
    };

    Vec3I m_size = {};
    i32 m_bytes_per_pixel = 0;//bytes per pixel
    Dimension m_dimension;
};

bool CreateTexture(Texture** texture, void* data, Vec3I size, Texture::Format format, i32 bytes_per_pixel);
bool CreateTexture(Texture** texture, const char* fileLocation, Texture::Format format);
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp);
void DeleteTexture(Texture** texture);

struct Renderer {
    SDL_Window* SDL_Context     = nullptr;
    GpuBuffer* voxel_rast_ib    = nullptr;
    GpuBuffer* voxel_rast_vb    = nullptr;
    GpuBuffer* voxel_vb         = nullptr;
    GpuBuffer* box_vb           = nullptr;//Does not need index buffer
    GpuBuffer* constant_vbuffer = nullptr;
    GpuBuffer* constant_pbuffer = nullptr;
    GpuBuffer* structure_voxel_materials= nullptr;
    GpuBuffer* structure_voxel_indices  = nullptr;
    //bool msaaEnabled = true;
    bool hasAttention;
    //i32 maxMSAASamples = 1;
    i32 swapInterval = SwapInterval_VSync;
    //float maxAnisotropic;
    //float currentAnisotropic = 1.0f;
    Vec2I size;
    Vec2I pos;
    u32   refresh_rate;
    ShaderProgram*  shaders[+ShaderProgram::Index_Count] = {};
    Texture*        textures[Texture::Type_Count] = {};

    enum SwapInterval_ {
        SwapInterval_AdaptiveSync = -1,
        SwapInterval_Immediate = 0,
        SwapInterval_VSync = 1,
    };
};
extern Renderer g_renderer;


void InitializeVideo();
void RenderUpdate(Vec2I windowSize, float deltaTime);
void RenderPresent();
void DrawPathTracedVoxels();


enum class MessageBoxType {
    Invalid,
    Error = SDL_MESSAGEBOX_ERROR,
    Warning = SDL_MESSAGEBOX_WARNING,
    Informative = SDL_MESSAGEBOX_INFORMATION,
    Count,
};
i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const char* title, const char* message);

#if 0
class TextureArray {
public:

    Vec2I m_size = {};
    GLuint m_handle = {};
    Vec2I m_spritesPerSide;
    float m_anisotropicAmount;


    TextureArray(const char* fileLocation);
    void Update(float anisotropicAmount);
    void Bind();
};

class TextureCube {
public:
    Vec2I m_size = {};
    GLuint m_handle = {};


    TextureCube(const char* fileLocation);
    void Bind();
};
#endif
