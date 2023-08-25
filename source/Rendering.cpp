#include "Tracy.hpp"

#include "Rendering.h"
#include "Misc.h"
#include "WinInterop_File.h"
#include "Vox.h"

Renderer g_renderer;

extern "C" {
#ifdef _MSC_VER
    _declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
    _declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#else
    __attribute__((dllexport)) uint32_t NvOptimusEnablement = 0x00000001;
    __attribute__((dllexport)) int AmdPowerXpressRequestHighPerformance = 0x00000001;
#endif
}

void OpenGLErrorCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                         GLsizei length, const GLchar *message, const void *userParam)
{
    std::string _severity;
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
        _severity = "HIGH";
        break;
        case GL_DEBUG_SEVERITY_MEDIUM:
        _severity = "MEDIUM";
        break;
        case GL_DEBUG_SEVERITY_LOW:
        _severity = "LOW";
        break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
        _severity = "NOTIFICATION";
        return;
        //break;
        default:
        _severity = "UNKNOWN";
        break;
    }

    DebugPrint("%s severity: %s\n",
            _severity.c_str(), message);

    AssertOnce(severity != GL_DEBUG_SEVERITY_HIGH);
}

void FillIndexBuffer(IndexBuffer* ib)
{
    std::vector<u32> arr;

    size_t amount = VOXEL_MAX_SIZE * VOXEL_MAX_SIZE * VOXEL_MAX_SIZE * 6 * 6;
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
    
    ib->Upload(arr.data(), amount);
}


void InitializeVideo()
{
    SDL_Init(SDL_INIT_VIDEO);
    {
        SDL_Rect screenSize = {};
        SDL_GetDisplayBounds(0, &screenSize);
        float displayRatio = 16 / 9.0f;
        g_renderer.size.x = screenSize.w / 2;
        g_renderer.size.y = Clamp<int>(int(g_renderer.size.x / displayRatio), 50, screenSize.h);
        g_renderer.pos.x = g_renderer.size.x / 2;
        g_renderer.pos.y = g_renderer.size.y / 2;
    }

    u32 windowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI /*SDL_WINDOW_MOUSE_CAPTURE | *//*SDL_WINDOW_MOUSE_FOCUS | *//*SDL_WINDOW_INPUT_GRABBED*/;

    g_renderer.SDL_Context = SDL_CreateWindow("V3", g_renderer.pos.x, g_renderer.pos.y, g_renderer.size.x, g_renderer.size.y, windowFlags);
    /* Create an OpenGL context associated with the window. */

    {
        const i32 majorVersionRequest = 4;//4;
        const i32 minorVersionRequest = 6;//3;
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, majorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minorVersionRequest);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        g_renderer.GL_Context = SDL_GL_CreateContext(g_renderer.SDL_Context);
        SDL_GL_MakeCurrent(g_renderer.SDL_Context, g_renderer.GL_Context);
        GLint majorVersionActual = {};
        GLint minorVersionActual = {};
        glGetIntegerv(GL_MAJOR_VERSION, &majorVersionActual);
        glGetIntegerv(GL_MINOR_VERSION, &minorVersionActual);

        if (majorVersionRequest != majorVersionActual && minorVersionRequest != minorVersionActual)
        {
            DebugPrint("OpenGL could not set recommended version: %i.%i to %i.%i\n", majorVersionRequest, minorVersionRequest,
                    majorVersionActual,  minorVersionActual);
            FAIL;
        }
    }

    /* This makes our buffer swap syncronized with the monitor's vertical refresh */
    SDL_GL_SetSwapInterval(g_renderer.swapInterval);
    glewExperimental = true;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        DebugPrint("Error: %s\n", glewGetErrorString(err));
    }
    DebugPrint("Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    //stbi_set_flip_vertically_on_load(true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClearColor(0.263f, 0.706f, 0.965f, 0.0f);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLErrorCallback, NULL);
    //glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, GL_FALSE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    //glEnable(GL_FRAMEBUFFER_SRGB);

    glGenVertexArrays(1, &g_renderer.vao);
    glBindVertexArray(g_renderer.vao);

    g_renderer.textures[Texture::Minecraft] = new Texture("assets/MinecraftSpriteSheet20120215Modified.png", GL_SRGB8_ALPHA8);

    g_renderer.shaders[+Shader::Main]       = new ShaderProgram("Source/Shaders/Main.vert", "Source/Shaders/Main.frag");
    g_renderer.shaders[+Shader::Voxel_Rast] = new ShaderProgram("Source/Shaders/Voxel_Rast.vert", "Source/Shaders/Voxel_Rast.frag");
    g_renderer.shaders[+Shader::Voxel]      = new ShaderProgram("Source/Shaders/Voxel.vert", "Source/Shaders/Voxel.frag");

    g_renderer.voxel_rast_ib = new IndexBuffer();
    FillIndexBuffer(g_renderer.voxel_rast_ib);
    g_renderer.voxel_rast_vb = new VertexBuffer();

    {
        g_renderer.voxel_ib = new IndexBuffer();
        u32 a[] = { 0, 1, 2, 1, 3, 2 };
        g_renderer.voxel_ib->Upload(a, arrsize(a));
    }
    

    {
        g_renderer.voxel_vb = new VertexBuffer();
        Vec3 a[] = {
            { -1.0f, -1.0f, 0.0f }, // 0
            { +1.0f, -1.0f, 0.0f }, // 1
            { -1.0f, +1.0f, 0.0f }, // 2
            { +1.0f, +1.0f, 0.0f }  // 3
        };
        g_renderer.voxel_vb->Upload(a, arrsize(a));
    }

    //Vertex verticees[] =
    //{
    //    {-1.0,  1.0, 0, 0.0f, 1.0f },
    //    {-1.0, -1.0, 0, 0.0f, 0.0f },
    //    { 1.0,  1.0, 0, 1.0f, 1.0f },
    //    { 1.0, -1.0, 0, 1.0f, 0.0f },
    //};
    //g_renderer.postVertexBuffer = new VertexBuffer();
    //g_renderer.postVertexBuffer->Upload(verticees, arrsize(verticees));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DepthWrite(bool status)
{
    glDepthMask(status);
}
void DepthRead(bool status)
{
    status ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

void CheckFrameBufferStatus()
{
    GLint err = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (err != GL_FRAMEBUFFER_COMPLETE)
    {
        DebugPrint("Error: Frame buffer error: %d \n", err);
        assert(false);
    }
}

void FrameBufferUpdate(const Vec2Int& size)
{
    if (size == g_renderer.size)
    {
        return;
    }

    CheckFrameBufferStatus();
}

double s_last_shader_update_time= 0;
double s_incremental_time= 0;
void RenderUpdate(Vec2Int windowSize, float deltaTime)
{
    ZoneScopedN("Render Update");
    CheckFrameBufferStatus();
    glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

    SDL_GL_SetSwapInterval(g_renderer.swapInterval);

    CheckFrameBufferStatus();
    FrameBufferUpdate(windowSize);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowSize.x, windowSize.y);
}

ShaderProgram::~ShaderProgram()
{
    //TODO: Delete shaders as well
    glDeleteProgram(m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Program Deleted\n");
#endif
}
bool ShaderProgram::CompileShader(GLuint handle, std::string text, const std::string& fileName)
{
    const char* strings[] = { text.c_str() };
    glShaderSource(handle, 1, strings, NULL);
    glCompileShader(handle);

    GLint status;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint log_length;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        std::string infoString;
        infoString.resize(log_length, 0);
        //GLchar info[4096] = {};
        glGetShaderInfoLog(handle, log_length, NULL, (GLchar*)infoString.c_str());
        //std::string errorTitle = ToString("%s compilation error: %s\n", fileName.c_str(), infoString.c_str());
        std::string errorTitle = fileName + " Compilation Error: ";
        DebugPrint((errorTitle + infoString + "\n").c_str());

        SDL_MessageBoxButtonData buttons[] = {
            //{ /* .flags, .buttonid, .text */        0, 0, "Continue" },
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
        };

        i32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), MessageBoxType::Error, errorTitle.c_str(), infoString.c_str());
        if (buttons[buttonID].buttonid == 2)//NOTE: Stop button
        {
            DebugPrint("stop hit");
            g_running = false;
            return false;
        }
        else
        {
            if (buttons[buttonID].buttonid == 0)//NOTE: Continue button
            {
                return false;
            }
            else if (buttons[buttonID].buttonid == 1)//NOTE: Retry button
            {
                CheckForUpdate();
            }
        }
    }
#ifdef _DEBUGPRINT
    DebugPrint("Shader Vertex/Fragment Created\n");
#endif
    return true;
}

ShaderProgram::ShaderProgram(const std::string& vertexFileLocation, const std::string& pixelFileLocation)
{
    m_vertexFile = vertexFileLocation;
    m_pixelFile = pixelFileLocation;

    CheckForUpdate();
}
void ShaderProgram::UseShader()
{
    glUseProgram(m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Used\n");
#endif
}
void ShaderProgram::CheckForUpdate()
{
    std::string vertexText;
    u64 vertexFileTime;
    std::string pixelText;
    u64 pixelFileTime;
    {

        File vertexFile(m_vertexFile, File::Mode::Read, false);
        vertexFile.GetTime();
        if (!vertexFile.m_timeIsValid)
        {
            assert(false);
            return;
        }
        vertexFile.GetText();
        vertexText = vertexFile.m_dataString;
        vertexFileTime = vertexFile.m_time;

        File pixelFile(m_pixelFile, File::Mode::Read, false);
        pixelFile.GetTime();
        if (!pixelFile.m_timeIsValid)
        {
            assert(false);
            return;
        }
        pixelFile.GetText();
        pixelText = pixelFile.m_dataString;
        pixelFileTime = pixelFile.m_time;
    }

    if (m_vertexLastWriteTime < vertexFileTime ||
        m_pixelLastWriteTime  < pixelFileTime)
    {
        //Compile shaders and link to program
        GLuint vhandle = glCreateShader(GL_VERTEX_SHADER);
        GLuint phandle = glCreateShader(GL_FRAGMENT_SHADER);

        if (!CompileShader(vhandle, vertexText, m_vertexFile) ||
            !CompileShader(phandle, pixelText, m_pixelFile))
            return;


        GLuint handle = glCreateProgram();

        glAttachShader(handle, vhandle);
        glAttachShader(handle, phandle);
        glLinkProgram(handle);

        GLint status;
        glGetProgramiv(handle, GL_LINK_STATUS, &status);
        if (status == GL_FALSE)
        {
            GLint log_length;
            glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
            GLchar info[4096] = {};
            assert(log_length > 0);
            glGetProgramInfoLog(handle, log_length, NULL, info);
            DebugPrint("Shader linking error: %s\n", info);

            SDL_MessageBoxButtonData buttons[] = {
                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "Retry" },
                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Stop" },
            };
            i32 buttonID = CreateMessageWindow(buttons, arrsize(buttons), MessageBoxType::Error, "Shader Compilation Error", reinterpret_cast<char*>(info));

            if (buttonID = 0)
            {
                CheckForUpdate();
            }
            else if (buttonID = 1)
            {
                g_running = false;
                return;
            }
            else
            {
                FAIL;
            }

        }
        else
        {
            m_handle = handle;
#ifdef _DEBUGPRINT
            DebugPrint("Shader Created\n");
#endif
            m_vertexLastWriteTime = vertexFileTime;
            m_pixelLastWriteTime = pixelFileTime;
            glDeleteShader(vhandle);
            glDeleteShader(phandle);
        }
    }
}

void ShaderProgram::UpdateUniformMat4(const char* name, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniformMatrix4fv(loc, count, transpose, value);
    DEBUGLOG("Shader Uniform Updated %s\n", name);
}

void ShaderProgram::UpdateUniformVec4(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform4fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformVec3(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform3fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformVec2(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform2fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformFloat(const char* name, GLfloat value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1f(loc, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformFloatStream(const char* name, GLsizei count, const GLfloat* value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1fv(loc, count, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformInt2(const char* name, Vec2Int values) { UpdateUniformInt2(name, GLint(values.x), GLint(values.y)); }
void ShaderProgram::UpdateUniformInt2(const char* name, GLint value1, GLint value2)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    //glUniform1f(loc, value);
    glUniform2i(loc, value1, value2);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformUint8(const char* name, GLuint value)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1ui(loc, value);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void ShaderProgram::UpdateUniformUintStream(const char* name, GLsizei count, GLuint* values)
{
    GLint loc = glGetUniformLocation(m_handle, name);
    glUniform1uiv(loc, count, values);
#ifdef _DEBUGPRINT
    DebugPrint("Shader Uniform Updated %s\n", name);
#endif
}

void GpuBuffer::UploadData(void* data, size_t size)
{
    Bind();
    size_t required_size = size;
    if (m_allocated_size < required_size)
    {
        glBufferData(m_target, required_size, nullptr, GL_STATIC_DRAW);
        m_allocated_size = required_size;
    }
    glBufferSubData(m_target, 0, required_size, data);
}

GpuBuffer::~GpuBuffer()
{
    glDeleteBuffers(1, &m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("GPU Buffer deleted %i, %i\n", m_target, m_handle);
#endif
}

void GpuBuffer::Bind()
{
    glBindBuffer(m_target, m_handle);
#ifdef _DEBUGPRINT
    DebugPrint("GPU Buffer Bound %i, %i\n", m_target, m_handle);
#endif
}

GLuint GpuBuffer::GetGLHandle()
{
    return m_handle;
}

void IndexBuffer::Upload(u32* indices, size_t count)
{
    m_count = Max(m_count, count);
    UploadData(indices, sizeof(indices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Index Buffer Upload,size %i\n", count);
#endif
}

void VertexBuffer::Upload(Vertex* vertices, size_t count)
{
    UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
}
void VertexBuffer::Upload(Vertex_Voxel* vertices, size_t count)
{
    UploadData(vertices, sizeof(vertices[0]) * count);
#ifdef _DEBUGPRINT
    DebugPrint("Vertex Buffer Upload,size %i\n", count);
#endif
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
