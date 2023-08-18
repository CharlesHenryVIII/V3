#define GB_MATH_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "GL/glew.h"
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif
#include "imgui.h"
#include "ImGui/backends/imgui_impl_sdl2.h"
#include "ImGui/backends/imgui_impl_opengl3.h"
#include "Tracy.hpp"
#include "stb/stb_image.h"

#include "Math.h"
#include "Misc.h"
#include "Rendering.h"
#include "Input.h"
#include "WinInterop_File.h"

#include <unordered_map>
#include <vector>
//#include <algorithm>


static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

int main(int argc, char* argv[])
{
    //Initilizers
    InitializeVideo();

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double startTime = SDL_GetPerformanceCounter() / freq;
    double totalTime = SDL_GetPerformanceCounter() / freq - startTime; //sec
    double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    bool showIMGUI = true;

    //___________
    //IMGUI SETUP
    //___________

#if 1
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(g_renderer.SDL_Context, g_renderer.GL_Context);
    ImGui_ImplOpenGL3_Init(glsl_version);
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& imGuiIO = ImGui::GetIO();
    bool g_cursorEngaged = true;
    SDL_ShowCursor(SDL_ENABLE);
    CommandHandler playerInput;
    float camera_distance = 1.0f;
    float camera_rotation = 0.0f;

    //WorldPos camera_position = GetCameraPosition(camera_distance, camera_rotation);

    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load("Grass_Block_Jumper.png", &x, &y, &n, 4);

    float tf = 1.0f;
    float te = 0.0f;
#if 1
    float p = 0.5f;
    float np = 0.0f;
    float nn = 1.0f;
    Vertex vertices[] = {
        { {  p,  p,  p }, { te, tf }, {  nn,  np,  np } }, // +x
        { {  p, -p,  p }, { te, te }, {  nn,  np,  np } },
        { {  p,  p, -p }, { tf, tf }, {  nn,  np,  np } },
        { {  p, -p, -p }, { tf, te }, {  nn,  np,  np } },
            
        { { -p,  p, -p }, { te, tf }, { -nn,  np,  np } }, // -x
        { { -p, -p, -p }, { te, te }, { -nn,  np,  np } },
        { { -p,  p,  p }, { tf, tf }, { -nn,  np,  np } },
        { { -p, -p,  p }, { tf, te }, { -nn,  np,  np } },
            
        { {  p,  p,  p }, { te, tf }, {  np,  nn,  np } }, // +y
        { {  p,  p, -p }, { te, te }, {  np,  nn,  np } },
        { { -p,  p,  p }, { tf, tf }, {  np,  nn,  np } },
        { { -p,  p, -p }, { tf, te }, {  np,  nn,  np } },
            
        { { -p, -p,  p }, { te, tf }, {  np, -nn,  np } }, // -y 
        { { -p, -p, -p }, { te, te }, {  np, -nn,  np } },
        { {  p, -p,  p }, { tf, tf }, {  np, -nn,  np } },
        { {  p, -p, -p }, { tf, te }, {  np, -nn,  np } },
            
        { { -p,  p,  p }, { te, tf }, {  np,  np,  nn } }, // +z
        { { -p, -p,  p }, { te, te }, {  np,  np,  nn } },
        { {  p,  p,  p }, { tf, tf }, {  np,  np,  nn } },
        { {  p, -p,  p }, { tf, te }, {  np,  np,  nn } },
            
        { {  p,  p, -p }, { te, tf }, {  np,  np, -nn } }, // -z
        { {  p, -p, -p }, { te, te }, {  np,  np, -nn } },
        { { -p,  p, -p }, { tf, tf }, {  np,  np, -nn } },
        { { -p, -p, -p }, { tf, te }, {  np,  np, -nn } },
    };
#else
    float p = 1.0f;
    Vertex vertices[] = {
        { { -p, -p,  p }, { te, tf }, { -1.0f,  0.0f,  0.0f } }, // -x
        { { -p, -p, -p }, { te, te }, { -1.0f,  0.0f,  0.0f } },
        { { -p,  p,  p }, { tf, tf }, { -1.0f,  0.0f,  0.0f } },
        { { -p,  p, -p }, { tf, te }, { -1.0f,  0.0f,  0.0f } },

        { {  p, -p,  p }, { te, tf }, {  1.0f,  0.0f,  0.0f } }, // +x
        { {  p, -p, -p }, { te, te }, {  1.0f,  0.0f,  0.0f } },
        { {  p,  p,  p }, { tf, tf }, {  1.0f,  0.0f,  0.0f } },
        { {  p,  p, -p }, { tf, te }, {  1.0f,  0.0f,  0.0f } },

        { { -p, -p,  p }, { te, tf }, {  0.0f, -1.0f,  0.0f } }, // -y
        { { -p, -p, -p }, { te, te }, {  0.0f, -1.0f,  0.0f } },
        { {  p, -p,  p }, { tf, tf }, {  0.0f, -1.0f,  0.0f } },
        { {  p, -p, -p }, { tf, te }, {  0.0f, -1.0f,  0.0f } },

        { { -p,  p,  p }, { te, tf }, {  0.0f,  1.0f,  0.0f } }, // +y 
        { { -p,  p, -p }, { te, te }, {  0.0f,  1.0f,  0.0f } },
        { {  p,  p,  p }, { tf, tf }, {  0.0f,  1.0f,  0.0f } },
        { {  p,  p, -p }, { tf, te }, {  0.0f,  1.0f,  0.0f } },

        { { -p,  p, -p }, { te, tf }, {  0.0f,  0.0f, -1.0f } }, // -z
        { { -p, -p, -p }, { te, te }, {  0.0f,  0.0f, -1.0f } },
        { {  p,  p, -p }, { tf, tf }, {  0.0f,  0.0f, -1.0f } },
        { {  p, -p, -p }, { tf, te }, {  0.0f,  0.0f, -1.0f } },

        { { -p,  p,  p }, { te, tf }, {  0.0f,  0.0f,  1.0f } }, // z
        { { -p, -p,  p }, { te, te }, {  0.0f,  0.0f,  1.0f } },
        { {  p,  p,  p }, { tf, tf }, {  0.0f,  0.0f,  1.0f } },
        { {  p, -p,  p }, { tf, te }, {  0.0f,  0.0f,  1.0f } },
    };
#endif
    static_assert(arrsize(vertices) == 24, "");

#if 0
    for (int i = 0; i < arrsize(vertices); i += 4)
    {
        int base = i * 4;
        gbVec3 a = vertices3D[base + 1].p - vertices3D[base + 0].p;
        gbVec3 b = vertices3D[base + 2].p - vertices3D[base + 0].p;
        gbVec3 c;
        gb_vec3_cross(&c, a, b);
        gb_vec3_norm(&c, c);
        vertices3D[base + 0].n = c;
        vertices3D[base + 1].n = c;
        vertices3D[base + 2].n = c;
        vertices3D[base + 3].n = c;
    }
#endif

    GLuint vertex_buffer;
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    int indices3D[36] = {};
    for (int face = 0; face < 6; ++face)
    {
        int base_index = face * 4;
        indices3D[face * 6 + 0] = base_index + 0;
        indices3D[face * 6 + 1] = base_index + 1;
        indices3D[face * 6 + 2] = base_index + 2;
        indices3D[face * 6 + 3] = base_index + 1;
        indices3D[face * 6 + 4] = base_index + 3;
        indices3D[face * 6 + 5] = base_index + 2;
        assert(base_index + 3 < arrsize(vertices));
    }

    GLuint index_buffer;
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices3D), indices3D, GL_STATIC_DRAW);

    //GLuint texture;
    //glGenTextures(1, &texture);
    //glBindTexture(GL_TEXTURE_2D, texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    //GLuint program = CreateShaderProgram(VertShader3D, PixShader);
    g_renderer.programs[+Shader::Main]->UseShader();

    Vec4 camera_pos = { 0,  2, -2,  1 };
    //Vec4 camera_rot = { 0,  0,  0,  0 };
    float camera_yaw = 0.0f;
    float camera_pitch = 0.0f;

    //glUseProgram(program);

    while (g_running)
    {
        {
            ZoneScopedN("Frame Update:");
            totalTime = SDL_GetPerformanceCounter() / freq - startTime;
            float deltaTime = float(totalTime - previousTime);// / 10;
            //previousTime = totalTime;
            //TODO: Time stepping for simulation
            if (deltaTime > (1.0f / 60.0f))
                deltaTime = (1.0f / 60.0f);

            //NOTE(CSH): Force the game to run at 1fps
            //WARNING(CSH): BUGGY
            //float delta_time_accumulator = 0;
            //delta_time_accumulator += deltaTime;
            //float desired_frame_time = 1.0f;
            //if (delta_time_accumulator < desired_frame_time)
            //{
            //    float sleep_time_s = desired_frame_time - deltaTime;
            //    Sleep_Thread(i64(sleep_time_s * 1000));
            //}
            //totalTime = SDL_GetPerformanceCounter() / freq - startTime;
            //deltaTime = float(totalTime - previousTime);// / 10;
            //previousTime = totalTime;

            /*********************
             *
             * Event Queing and handling
             *
             ********/

            SDL_Event SDLEvent;
            playerInput.mouse.pDelta = {};
            {
                ZoneScopedN("Poll Events");
                while (SDL_PollEvent(&SDLEvent))
                {
                    ImGui_ImplSDL2_ProcessEvent(&SDLEvent);

                    switch (SDLEvent.type)
                    {
                    case SDL_QUIT:
                        g_running = false;
                        break;
                    case SDL_KEYDOWN:
                    case SDL_KEYUP:
                        if (g_renderer.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureKeyboard)
                            {
                                playerInput.keyStates[SDLEvent.key.keysym.sym].down = (SDLEvent.type == SDL_KEYDOWN);
                            }
                        }
                        break;
                    case SDL_MOUSEBUTTONDOWN:
                    case SDL_MOUSEBUTTONUP:
                        if (g_renderer.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                playerInput.keyStates[SDLEvent.button.button].down = SDLEvent.button.state;
                            }
                        }
                        break;
                    case SDL_MOUSEMOTION:
                    {
                        if (g_renderer.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                if (g_cursorEngaged)
                                {
                                    playerInput.mouse.pDelta.x = SDLEvent.motion.xrel;
                                    playerInput.mouse.pDelta.y = SDLEvent.motion.yrel;

                                    //SDL_WarpMouseInWindow(g_renderer.SDL_Context, g_renderer.size.x / 2, g_renderer.size.y / 2);
                                    //playerInput.mouse.pos.x = SDLEvent.motion.x;
                                    //playerInput.mouse.pos.y = SDLEvent.motion.y;
                                    //playerInput.mouse.pos.x = g_renderer.size.x / 2;
                                    //playerInput.mouse.pos.y = g_renderer.size.y / 2;
                                }

                            }
                        }
                        break;
                    }
                    case SDL_MOUSEWHEEL:
                    {
                        if (g_renderer.hasAttention)
                        {
                            if (!imGuiIO.WantCaptureMouse)
                            {
                                playerInput.mouse.wheelInstant.x = playerInput.mouse.wheel.x = SDLEvent.wheel.preciseX;
                                playerInput.mouse.wheelInstant.y = playerInput.mouse.wheel.y = SDLEvent.wheel.preciseY;
                            }
                        }
                        break;
                    }

                    case SDL_WINDOWEVENT:
                    {
                        switch (SDLEvent.window.event)
                        {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        {
                            g_renderer.size.x = SDLEvent.window.data1;
                            g_renderer.size.y = SDLEvent.window.data2;
                            glViewport(0, 0, g_renderer.size.x, g_renderer.size.y);
                            break;
                        }
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                        {
                            g_renderer.hasAttention = true;
                            playerInput.mouse.pDelta = {};
                            SDL_GetMouseState(&playerInput.mouse.pos.x, &playerInput.mouse.pos.y);
                            if (g_cursorEngaged)
                            {
                                SDL_CaptureMouse(SDL_TRUE);
                                SDL_ShowCursor(SDL_ENABLE);
                            }

                            break;
                        }
                        case SDL_WINDOWEVENT_LEAVE:
                        {
                            //g_renderer.hasAttention = false;
                            break;
                        }
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                        {
                            g_renderer.hasAttention = false;
                            break;
                        }
                        }
                        break;
                    }
                    }
                }
            }

            /*********************
             *
             * Setting Key States
             *
             ********/

            {
                ZoneScopedN("Key Updates");
                playerInput.InputUpdate();

                if (playerInput.keyStates[SDLK_ESCAPE].down)
                    g_running = false;

                if (playerInput.keyStates[SDLK_z].downThisFrame)
                {
                    showIMGUI = !showIMGUI;
                }
            }

            if (playerInput.keyStates[SDL_BUTTON_MIDDLE].down)
            {
                camera_yaw -= ((float(playerInput.mouse.pDelta.x) / float(g_renderer.size.x)) * (tau));
                camera_pitch += ((float(playerInput.mouse.pDelta.y) / float(g_renderer.size.y)) * (tau));
            }

            Mat4 camera_world_matrix;
            Mat4 trans;
            Mat4 rot;
            gb_mat4_identity(&camera_world_matrix);
            //gb_mat4_from_quat(&rot, m_transform.m_quat);
            gb_mat4_from_quat(&rot, gb_quat_euler_angles(camera_pitch, camera_yaw, 0.0f));
            gb_mat4_translate(&trans, camera_pos.xyz);
            camera_world_matrix = rot * trans;//trans * rot;
            Vec3 camera_pos_world = camera_world_matrix.col[3].xyz;

            if (showIMGUI)
            {
                ZoneScopedN("ImGui Update");
                float transformInformationWidth = 0.0f;
                {
                    // Start the Dear ImGui frame
                    ImGui_ImplOpenGL3_NewFrame();
                    ImGui_ImplSDL2_NewFrame(g_renderer.SDL_Context);
                    ImGui::NewFrame();

                    const float PAD = 5.0f;
                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                        ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

                    const ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
                    ImVec2 window_pos;//, window_pos_pivot;
                    window_pos.x = work_pos.x + PAD;
                    window_pos.y = work_pos.y + PAD;
                    //window_pos_pivot.x = window_pos_pivot.y = 0.0f;
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, {});

                    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
                    if (ImGui::Begin("Transform Information", nullptr, windowFlags))
                    {
                        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders;
                        if (ImGui::BeginTable("Position Info", 4, flags))
                        {
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("X");
                            ImGui::TableSetupColumn("Y");
                            ImGui::TableSetupColumn("Z");
                            ImGui::TableHeadersRow();

                            GenericImGuiTable("Camera_pos  ", "%+08.2f", camera_pos.e, 3);
                            GenericImGuiTable("Camera_world", "%+08.2f", camera_pos_world.e, 3);
                            //GenericImGuiTable("Yaw", "%+06.4f", &camera_yaw, 1);
                            //GenericImGuiTable("Chunk", "%i", ToChunk(player->m_transform.m_p).p.e);

                            ImGui::EndTable();
                        }
                    }
                    ImGui::End();
                }
            }

#if 1
            //DebugPrint("playerInput.x = %+04i\n", playerInput.mouse.pDelta.x);
            //NOTE: orignal from OpenGL testing
            //Mat4 rotate;
            //gb_mat4_rotate(&rotate, { 0,1,0 }, (float(playerInput.mouse.pDelta.x) / float(g_renderer.size.x))* tau);
            //Mat4 translate;
            //gb_mat4_translate(&translate, {});
            //Mat4 result = translate * rotate;
            
            //playerInput.mouse.wheelInstant.y //positive zoom in, negative zoom out
                            //Mat4 Entity::GetWorldMatrix() const
                            //{
                            //    Mat4 result;
                            //    Mat4 trans;
                            //    Mat4 rot;
                            //    gb_mat4_identity(&result);
                            //    //gb_mat4_from_quat(&rot, m_transform.m_quat);
                            //    gb_mat4_from_quat(&rot, gb_quat_euler_angles(DegToRad(m_transform.m_pitch), DegToRad(m_transform.m_yaw), 0.0f));
                            //    gb_mat4_translate(&trans, m_transform.m_p.p);
                            //    result = trans * rot;
                            //
                            //    if (m_parent)
                            //    {
                            //        Entity* e = g_entityList.GetEntity(m_parent);
                            //        if (e)
                            //        {
                            //            //result = e->GetWorldMatrix() * result;
                            //            result = e->GetWorldMatrix() * result;
                            //        }
                            //    }
                            //    return result;
                            //}

        //sp->UpdateUniformMat4("u_perspective", 1, false, playerCamera->m_perspective.e);
        //sp->UpdateUniformMat4("u_view", 1, false, playerCamera->m_view.e);

            //Mat4 translate;
            //gb_mat4_translate(&translate, camera_pos.xyz);
            ///Mat4 rotate_yaw;
            ///gb_mat4_rotate(&rotate_yaw, { 0,1,0 }, camera_yaw);
            /////Mat4 rotate_pitch;
            /////gb_mat4_rotate(&rotate_pitch, { 0,1,0 }, camera_pitch);
            ///Mat4 camera_mat = rotate_yaw /** rotate_pitch*/ * translate;
            /////camera_mat * camera_pos;

            gbMat4 perspective;
            gb_mat4_perspective(&perspective, 3.14f / 2, float(g_renderer.size.x) / g_renderer.size.y, 0.1f, 2000.0f);
            gbMat4 view;
            gb_mat4_look_at(&view, camera_pos_world, {}, { 0,1,0 });

#else

            Vec3 lookTarget = {};
            WorldPos cameraRealWorldPosition = playerCamera->GetWorldPosition();
            lookTarget = cameraRealWorldPosition.p + playerCamera->GetForwardVector();
            gb_mat4_look_at(&playerCamera->m_view, cameraRealWorldPosition.p, lookTarget, playerCamera->m_up);

            //Near Clip and Far Clip
            gb_mat4_perspective(&playerCamera->m_perspective, 3.14f / 2, float(g_renderer.size.x) / g_renderer.size.y, 0.1f, 2000.0f);
            playerCamera->m_viewProj = playerCamera->m_perspective * playerCamera->m_view;
#endif



            //Debug Checks
#if 0
            {
                WorldPos cubePosition;
                cubePosition.p = { -125, 200, 0 };
                AddCubeToRender(cubePosition, White, 1);
                cubePosition.p.y -= 1;
                AddCubeToRender(cubePosition, Blue, 0.5f);
                cubePosition.p.y -= 1;
                AddCubeToRender(cubePosition, Orange, 0.25f);
            }
#endif




            RenderUpdate(g_renderer.size, deltaTime);

            glClearDepth(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            ////SKYBOX
            //RenderSkybox(playerCamera);

            {
                glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
                g_renderer.textures[Texture::Minecraft]->Bind();
                g_renderer.programs[+Shader::Main]->UseShader();

                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);

                glEnableVertexArrayAttrib(g_renderer.vao, 0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, p));
                glEnableVertexArrayAttrib(g_renderer.vao, 1);
                glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
                glEnableVertexArrayAttrib(g_renderer.vao, 2);
                glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, n));

                //gbMat4 perspective;
                //int w, h;
                //SDL_GetWindowSize(window, &w, &h);
                //gb_mat4_perspective(&perspective, 3.14f / 2, float(w) / h, 0.65f, 1000.0f);
                //gbMat4 view;
                //gb_mat4_look_at(&view, { 2, 2, 2 }, {}, { 0,1,0 });

                //GLint loc;
                g_renderer.programs[+Shader::Main]->UpdateUniformMat4("u_perspective", 1, false, perspective.e);
                //loc = glGetUniformLocation(dc.program, "u_perspective");
                //glUniformMatrix4fv(loc, 1, false, perspective.e);
                g_renderer.programs[+Shader::Main]->UpdateUniformMat4("u_view", 1, false, view.e);
                //loc = glGetUniformLocation(dc.program, "u_view");
                //glUniformMatrix4fv(loc, 1, false,  view.e);
                //g_renderer.programs[+Shader::Main]->UpdateUniformMat4("u_model", 1, false, camera_mat.e);
                //loc = glGetUniformLocation(dc.program, "u_model");
                //glUniformMatrix4fv(loc, 1, false, camera_mat.e);
                //loc = glGetUniformLocation(dc.program, "u_time");
                //glUniform1f(loc, time);

                glDrawElements(GL_TRIANGLES, arrsize(indices3D), GL_UNSIGNED_INT, 0);
            }



            {
                ZoneScopedN("ImGui Render");
                if (showIMGUI)
                {
                    ImGui::Render();
                    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                }
            }
        }
        {
            ZoneScopedN("Frame End");

            SDL_GL_SwapWindow(g_renderer.SDL_Context);
        }
        FrameMark;
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(g_renderer.GL_Context);
    SDL_DestroyWindow(g_renderer.SDL_Context);
    SDL_Quit();
    return 0;
}
