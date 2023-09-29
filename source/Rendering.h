#pragma once
#include "SDL.h"
#include "Math.h"
#include "Misc.h"
#include "Rendering_Texture.h"
#include "Vox.h"

enum class Shader : u32 {
    Invalid,
    Main,
    Voxel_Rast,
    Voxel,
    Cube,
    Count,
};
ENUMOPS(Shader);


class ShaderProgram
{
    GLuint m_handle = 0;
    std::string m_vertexFile;
    std::string m_pixelFile;
    u64 m_vertexLastWriteTime = {};
    u64 m_pixelLastWriteTime = {};

    ShaderProgram(const ShaderProgram& rhs) = delete;
    ShaderProgram& operator=(const ShaderProgram& rhs) = delete;
    bool CompileShader(GLuint handle, std::string text, const std::string& fileName);

public:
    ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation);
    ~ShaderProgram();
    void CheckForUpdate();
    void UseShader();
    void UpdateUniform(const char* name, const Mat4*    value, const i32 count, bool transpose  );
    void UpdateUniform(const char* name, const Mat4&    value, bool transpose                   );
    void UpdateUniform(const char* name, const Vec4*    value, const i32 count                  );
    void UpdateUniform(const char* name, const Vec3&    value                                   );
    void UpdateUniform(const char* name, const Vec3*    value, const i32 count                  );
    void UpdateUniform(const char* name, const Vec2*    value, const i32 count                  );
    void UpdateUniform(const char* name, const float    value                                   );
    void UpdateUniform(const char* name, const float*   value, const i32 count                  );
    void UpdateUniform(const char* name, const Vec2I&   value                                   );
    void UpdateUniform(const char* name, const Vec3I&   value                                   );
    void UpdateUniform(const char* name, const u32      value                                   );
    void UpdateUniform(const char* name, const u32*     value, const i32 count                  );
};

class GpuBuffer
{
    GLuint m_target;
    size_t m_allocated_size;
    GLuint m_handle;

    GpuBuffer(const GpuBuffer& rhs) = delete;
    GpuBuffer& operator=(const GpuBuffer& rhs) = delete;

protected:
    GpuBuffer(GLuint target)
        : m_target(target)
        , m_allocated_size(0)
    {
        glGenBuffers(1, &m_handle);
#ifdef _DEBUGPRINT
        DebugPrint("GPU Buffer Created %i\n", m_target);
#endif
    }
    void UploadData(void* data, size_t size);

public:
    virtual ~GpuBuffer();
    void Bind();
    GLuint GetGLHandle();
};

class IndexBuffer : public GpuBuffer
{
public:
    IndexBuffer()
        : GpuBuffer(GL_ELEMENT_ARRAY_BUFFER)
    { }
    size_t m_count = 0;
    void Upload(u32* indices, size_t count);
};

class VertexBuffer : public GpuBuffer
{
public:
    VertexBuffer()
        : GpuBuffer(GL_ARRAY_BUFFER)
    { }
    void Upload(Vertex* vertices, size_t count);
    void Upload(Vertex_Voxel* vertices, size_t count);
    template <typename T>
    void Upload(T* vertices, size_t count)
    {
        UploadData(vertices, sizeof(vertices[0]) * count);
        DEBUG_LOG("Vertex Buffer Upload,size %i\n", count);
    }
    template <typename T>
    void Upload(std::vector<T>& vertices)
    {
        UploadData(vertices.data(), (sizeof(vertices[0]) * vertices.size()));
        DEBUG_LOG("Vertex Buffer Upload,size %i\n", count);
    }
};

struct Renderer {
    SDL_Window* SDL_Context = nullptr;
    SDL_GLContext GL_Context = {};
    GLuint vao;
    IndexBuffer*    voxel_rast_ib   = nullptr;
    VertexBuffer*   voxel_rast_vb   = nullptr;
    IndexBuffer*    voxel_ib        = nullptr;
    VertexBuffer*   voxel_vb        = nullptr;
    VertexBuffer*   voxel_box_vb = nullptr;
    //bool msaaEnabled = true;
    bool hasAttention;
    //i32 maxMSAASamples = 1;
    i32 swapInterval = SwapInterval_VSync;
    //float maxAnisotropic;
    //float currentAnisotropic = 1.0f;
    Vec2I size;
    Vec2I pos;
    ShaderProgram*  shaders[+Shader::Count] = {};
    Texture*        textures[Texture::Count] = {};

    enum SwapInterval_ {
        SwapInterval_AdaptiveSync = -1,
        SwapInterval_Immediate = 0,
        SwapInterval_VSync = 1,
    };
};
extern Renderer g_renderer;


void InitializeVideo();
void DepthWrite(bool status);
void DepthRead(bool status);
//NOTE(CSH): DOES THIS NEED TO BE EXPOSED?
void CheckFrameBufferStatus();
void RenderUpdate(Vec2I windowSize, float deltaTime);

enum class MessageBoxType {
    Invalid,
    Error = SDL_MESSAGEBOX_ERROR,
    Warning = SDL_MESSAGEBOX_WARNING,
    Informative = SDL_MESSAGEBOX_INFORMATION,
    Count,
};
i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const char* title, const char* message);
