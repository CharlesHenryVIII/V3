#pragma once
#include "SDL.h"
#include "Math.h"
#include "Debug.h"
//#include "Rendering_Texture.h"
#include "Vox.h"
#include "GpuSharedData.h"

#include <unordered_map>

#define MAX_MIPS 10




//************
//Texture
//************

struct Texture {
    enum Index : u32 {
        Index_Invalid,
        Index_Minecraft,
        Index_Plain,
        Index_Voxel_Indices,
        Index_Voxel_Indices_mip1,
        Index_Voxel_Indices_mip2,
        Index_Voxel_Indices_mip3,
        Index_Voxel_Indices_mip4,
        Index_Voxel_Indices_mip5,
        Index_Voxel_Indices_mip6,
        Index_Random,
        Index_Backbuffer_Depth,
        Index_Backbuffer_HDR,
        Index_Count,
    }; ENUMOPS(Index);
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
        Filter_Aniso,
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
        Format_R11G11B10_FLOAT,
        Format_D32_FLOAT,
        Format_D16_UNORM,
        Format_R8G8B8A8_UNORM,
        Format_R8G8B8A8_UNORM_SRGB,
        Format_R8G8B8A8_UINT,
        Format_R8_UINT,
        Format_Count,
    }; ENUMOPS(Format);
    enum Type : u32 {
        Type_Invalid,
        Type_Texture,
        Type_Depth,
        Type_Count,
    }; ENUMOPS(Type);

    struct TextureParams {
        Vec3I size;
        Format format = Format_R8G8B8A8_UINT;
        AddressMode mode = Address_Wrap;
        Filter filter = Filter_Aniso;
        Type type = Type_Texture;
        bool render_target;
        i32 bytes_per_pixel;
    };

    u32 m_mip_levels = 1;
    Dimension m_dimension;
    TextureParams m_parameters;
};

bool CreateTexture(Texture** texture, const void** data, Vec3I size, Texture::Format format, i32 bytes_per_pixel);
bool CreateTexture(Texture** texture, const char* fileLocation, Texture::Format format, Texture::Filter filter);
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp, u32 mip_levels, const u8* data);
bool CreateTexture(Texture** texture, const Texture::TextureParams& tp, const void* data);
bool UpdateTexture(Texture** texture, u32 mip_slice, void* data, u32 row_pitch_bytes, u32 depth_pitch_bytes);
void DeleteTexture(Texture** texture);





//************
//GpuBuffer
//************

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
    enum class BindLocation : u32 {
        Invalid,
        Vertex,
        Pixel,
        All,
        Count,
    };
    ENUMOPS(BindLocation);

    bool m_is_dymamic = true;
    Type m_type = GpuBuffer::Type::Invalid;
    char m_name[32];
    size_t m_count = 0;

    void Upload(const void* data, const size_t count, const u32 element_size, const bool is_byte_format = false);
    template<typename T>
    inline void Upload(const std::vector<T>& a)
    {
        assert(a.size());
        Upload(a.data(), a.size(), sizeof(T), false);
    }
    //Bind a Constant or Structure buffer
    void Bind(u32 slot, GpuBuffer::BindLocation binding);
};
bool CreateGpuBuffer(GpuBuffer** buffer, const char* name, bool is_dynamic, GpuBuffer::Type type);
void DeleteBuffer(GpuBuffer** buffer);





//************
//Shader
//************

struct Shader
{
    enum Type : u32 {
        Type_Invalid,
        Type_Vertex,
        Type_Pixel,
        Type_Count,
    };
    ENUMOPS(Type);

    enum Index : u32 {
        Index_Invalid,
        Index_Main,
        Index_Voxel_Rast,
        Index_Voxel,
        Index_Cube,
        Index_Tetra,
        Index_Final_Draw,
        Index_Count,
    };
    ENUMOPS(Index);

    struct InputElementDesc {
        const char* SemanticName;
        u32 Format; //DXGI_FORMAT
        u32 AlignedByteOffset;
    };

    static const u32 m_vertex_component_max = 4;

    ~Shader();
    void CheckForUpdate();

    std::string m_vertexFile;
    std::string m_pixelFile;
    u64 m_vertexLastWriteTime = {};
    u64 m_pixelLastWriteTime = {};
    u32 m_vertex_component_count = 0;
    std::vector<std::string> m_reference_file_names;
    std::vector<u64> m_reference_file_times;

    bool CompileShader(std::string text, const std::string& file_name, Shader::Type shader_type);
};
bool CreateShader(Shader** shader,
    const std::string& vertexFileLocation,
    const std::string& pixelFileLocation,
    Shader::InputElementDesc* input_layout,
    i32 layout_count);
inline bool CreateShader(Shader** s, const std::string& shader_file_location, Shader::InputElementDesc* input_layout, i32 layout_count)
{
    return CreateShader(s, shader_file_location, shader_file_location, input_layout, layout_count);
}




//************
//Renderer
//************

struct Renderer {
    SDL_Window* SDL_Context     = nullptr;
    GpuBuffer* quad_ib          = nullptr;
    GpuBuffer* voxel_rast_vb    = nullptr;
    GpuBuffer* voxel_vb         = nullptr;
    GpuBuffer* box_vb           = nullptr;//Does not need index buffer
    GpuBuffer* cube_vb          = nullptr;
    GpuBuffer* tetra_vb         = nullptr;
    GpuBuffer* cb_common        = nullptr;
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
    Shader*  shaders[+Shader::Index_Count] = {};
    Texture*        textures[Texture::Index_Count] = {};

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
        void AddCubeToRender(Vec3 p, Color color, Vec3  scale, bool wireframe);
inline  void AddCubeToRender(Vec3 p, Color color, float scale, bool wireframe) { AddCubeToRender(p, color, { scale, scale, scale }, wireframe); }
void AddTetrahedronToRender(const Vec3 p, const Vec3 dir, Color color, Vec3  scale, bool wireframe);
void RenderPrimitives();
void FinalDraw();


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
