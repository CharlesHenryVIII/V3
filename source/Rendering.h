#pragma once
#include "SDL.h"
#include "Math.h"
#include "Misc.h"
#include "Rendering_Texture.h"

enum class Shader : u32 {
    Invalid,
    Main,
    Count,
};
ENUMOPS(Shader);

class ShaderProgram;

struct Renderer {
    SDL_Window* SDL_Context = nullptr;
    SDL_GLContext GL_Context = {};
    GLuint vao;
    //bool msaaEnabled = true;
    bool hasAttention;
    //i32 maxMSAASamples = 1;
    i32 swapInterval = 1;
    //float maxAnisotropic;
    //float currentAnisotropic = 1.0f;
    Vec2Int size;
    Vec2Int pos;
    ShaderProgram*  programs[+Shader::Count] = {};
    Texture*        textures[Texture::Count] = {};

    //ShaderProgram*  programs[+Shader::Count] = {};
    //Texture*        textures[Texture::Count] = {};
};
extern Renderer g_renderer;

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
    void UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value);
    void UpdateUniformVec4(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformVec3(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformVec2(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformFloat(const char* name, GLfloat value);
    void UpdateUniformFloatStream(const char* name, GLsizei count, const GLfloat* value);
    void UpdateUniformInt2(const char* name, Vec2Int values);
    void UpdateUniformInt2(const char* name, GLint value1, GLint value2);
    void UpdateUniformUint8(const char* name, GLuint value);
    void UpdateUniformUintStream(const char* name, GLsizei count, GLuint* values);

    template<typename T>
    void UpdateUniform(const char* name, T v);
};


void InitializeVideo();
void DepthWrite(bool status);
void DepthRead(bool status);
//NOTE(CSH): DOES THIS NEED TO BE EXPOSED?
void CheckFrameBufferStatus();
void RenderUpdate(Vec2Int windowSize, float deltaTime);

enum class MessageBoxType {
    Invalid,
    Error = SDL_MESSAGEBOX_ERROR,
    Warning = SDL_MESSAGEBOX_WARNING,
    Informative = SDL_MESSAGEBOX_INFORMATION,
    Count,
};
i32 CreateMessageWindow(SDL_MessageBoxButtonData* buttons, i32 numOfButtons, MessageBoxType type, const char* title, const char* message);
