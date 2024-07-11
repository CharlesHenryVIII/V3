#pragma once
#include "SDL.h"
#include "Math.h"
#include "Debug.h"
//#include "Rendering_Texture.h"
#include "Vox.h"
#include "GpuSharedData.h"

#include "dxgiformat.h"

#include <unordered_map>

#define MAX_MIPS 10

//#define RENDER_PIPELINE_OPENGL BIT(1)
#define RENDER_PIPELINE_DX11 BIT(2)
#define RENDER_PIPELINE_DX12 BIT(3)
#define RENDER_PIPELINE RENDER_PIPELINE_DX12



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
        Index_Hello_Triangle,
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

    std::wstring m_vertex_filename;
    std::wstring m_pixel_filename;
    u64 m_vertexLastWriteTime = {};
    u64 m_pixelLastWriteTime = {};
    u32 m_vertex_component_count = 0;
    std::vector<std::string> m_reference_file_names;
    std::vector<u64> m_reference_file_times;

    bool CompileShader(const std::wstring& file_name, Shader::Type shader_type);
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
void DrawPrimitives();
void DrawFinal();


enum class MessageBoxType {
    Invalid,
    Error = SDL_MESSAGEBOX_ERROR,
    Warning = SDL_MESSAGEBOX_WARNING,
    Informative = SDL_MESSAGEBOX_INFORMATION,
    Count,
};
i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const char* title, const char* message);
i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const wchar_t* title, const wchar_t* message);

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


inline void InitializeData(const Vec2I backbuffer_size)
{
    //
    //Create Textures:
    //

    //CreateTexture(&g_renderer.textures[Texture::Index_Minecraft], "assets/MinecraftSpriteSheet20120215Modified.png", Texture::Format_R8G8B8A8_UNORM_SRGB, Texture::Filter_Point);
    //u8 pixel_texture_data[] = { 255, 255, 255, 255 };
    //CreateTexture(&g_renderer.textures[Texture::Index_Plain], pixel_texture_data, { 1, 1, 0 }, Texture::Format_R8G8B8A8_UNORM, sizeof(pixel_texture_data[0]));
    //CreateTexture(&g_renderer.textures[Texture::Index_Random], "assets/random-dcode.png", Texture::Format_R8G8B8A8_UNORM, Texture::Filter_Point);

    //{
    //    Texture::TextureParams tp = {
    //        .size = ToVec3I(backbuffer_size, 0),
    //        .format = Texture::Format_D32_FLOAT,
    //        .mode = Texture::Address_Invalid,
    //        .filter = Texture::Filter_Invalid,
    //        .type = Texture::Type_Depth,
    //        .render_target = true,
    //        .bytes_per_pixel = 0,
    //    };
    //    CreateTexture(&g_renderer.textures[Texture::Index_Backbuffer_Depth], tp, nullptr);
    //}
    //{
    //    Texture::TextureParams tp = {
    //        .size   = ToVec3I(backbuffer_size, 0),
    //        .format = Texture::Format_R11G11B10_FLOAT,
    //        .mode   = Texture::Address_Clamp,
    //        .filter = Texture::Filter_Aniso,
    //        .type   = Texture::Type_Texture,
    //        .render_target = true,
    //        .bytes_per_pixel = 4,
    //    };
    //    CreateTexture(&g_renderer.textures[Texture::Index_Backbuffer_HDR], tp, nullptr);
    //}


    //
    //Create Shaders:
    //

    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT,   0, (UINT)offsetof(Vertex, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "UV",         0, DXGI_FORMAT_R32G32_FLOAT,      0, (UINT)offsetof(Vertex, uv),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "NORMAL",     0, DXGI_FORMAT_R32G32B32_FLOAT,   0, (UINT)offsetof(Vertex, n),   D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
    //    g_renderer.shaders[+Shader::Main] = new Shader("Source/Shaders/Main.vert", "Source/Shaders/Main.frag", layout, arrsize(layout));
    //}
    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Voxel, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "COLOR",      0, DXGI_FORMAT_R32_UINT,        0, (UINT)offsetof(Vertex_Voxel, rgba),D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "NORMAL",     0, DXGI_FORMAT_R8_UINT,         0, (UINT)offsetof(Vertex_Voxel, n),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "AO",         0, DXGI_FORMAT_R8_UINT,         0, (UINT)offsetof(Vertex_Voxel, ao),  D3D11_INPUT_PER_VERTEX_DATA, 0 }, };
    //    g_renderer.shaders[+Shader::Voxel_Rast] = new Shader("Source/Shaders/Voxel_Rast.vert", "Source/Shaders/Voxel_Rast.frag", layout, arrsize(layout));
    //}
    {
        Shader::InputElementDesc layout[] = { { "POSITION", DXGI_FORMAT_R32G32_FLOAT, 0 },
                                              { "COLOR",    DXGI_FORMAT_R32G32_FLOAT, 12 },  };
        VERIFY(CreateShader(&g_renderer.shaders[+Shader::Index_Hello_Triangle],   "source/shaders/HelloTriangle.hlsl",    layout, arrsize(layout)));
    }
    //{
    //    //D3D11_INPUT_ELEMENT_DESC layout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    //    Shader::InputElementDesc layout[] = { { "POSITION", DXGI_FORMAT_R32G32_FLOAT, 0 } };
    //    VERIFY(CreateShader(&g_renderer.shaders[+Shader::Index_Voxel],   "Source/Shaders/Voxel.hlsl",    layout, arrsize(layout)));
    //}
    //{
    //    Shader::InputElementDesc layout[] = {
    //        { "COLOR",      DXGI_FORMAT_R32G32B32A32_FLOAT, offsetof(Vertex_Cube, color)    },
    //        { "POSITION",   DXGI_FORMAT_R32G32B32_FLOAT,    offsetof(Vertex_Cube, p)        },
    //        { "TEXCOORD",   DXGI_FORMAT_R32G32_FLOAT,       offsetof(Vertex_Cube, uv)       } };
    //    VERIFY(CreateShader(&g_renderer.shaders[+Shader::Index_Cube],    "Source/Shaders/Cube.hlsl",     layout, arrsize(layout)));
    //}
    //{
    //    Shader::InputElementDesc layout[] = {
    //        { "COLOR",      DXGI_FORMAT_R32G32B32A32_FLOAT, offsetof(Vertex_Tetra, color)    },
    //        { "POSITION",   DXGI_FORMAT_R32G32B32_FLOAT,    offsetof(Vertex_Tetra, p)        },
    //        { "NORMAL",     DXGI_FORMAT_R32G32B32_FLOAT,    offsetof(Vertex_Tetra, n)        } };
    //    VERIFY(CreateShader(&g_renderer.shaders[+Shader::Index_Tetra],    "Source/Shaders/Tetra.hlsl",   layout, arrsize(layout)));
    //}
    //{
    //    Shader::InputElementDesc layout[] = { { "POSITION", DXGI_FORMAT_R32G32_FLOAT, 0 } };
    //    VERIFY(CreateShader(&g_renderer.shaders[+Shader::Index_Final_Draw],   "Source/Shaders/Final_Draw.hlsl",  layout, arrsize(layout)));
    //}
    //{
    //    D3D11_INPUT_ELEMENT_DESC layout[] = {
    //        { "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Cube, p),   D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //        { "COLOR",      0, DXGI_FORMAT_R32G32B32_FLOAT, 0, (UINT)offsetof(Vertex_Cube, color),D3D11_INPUT_PER_VERTEX_DATA, 0 } };
    //    g_renderer.shaders[+Shader::Cube] = new Shader("Source/Shaders/Cube.vert", "Source/Shaders/Cube.frag", layout, arrsize(layout));
    //}

    //
    //Create Buffers:
    //
    CreateGpuBuffer(&g_renderer.quad_ib,        "Quad_IB",          true,   GpuBuffer::Type::Index);
    {
        size_t count = 6 * 4;
        if (g_renderer.quad_ib->m_count > count)
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

        g_renderer.quad_ib->Upload(arr.data(), amount, sizeof(baseIndex));
    }
    CreateGpuBuffer(&g_renderer.tetra_vb,       "Tetra_VB",         false,  GpuBuffer::Type::Vertex);
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
}

void TempPopulateCommandQueue();

void StartImgui();
void RenderImgui(bool showImgui);
void ShutdownImgui();
void GetImguiSDLEvent(const SDL_Event* event);
