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
#include "Vox.h"
#include "Raycast.h"
#include "Rendering_Cube.h"

#include <unordered_map>
#include <vector>
//#include <algorithm>

#define RASTERIZED_RENDERING 0

template <typename T>
void GenericImGuiTable(const std::string& title, const std::string& fmt, T* firstValue, i32 length = 3)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextUnformatted(title.data());
    for (i32 column = 0; column < length; column++)
    {
        ImGui::TableSetColumnIndex(column + 1);
        std::string string = ToString(fmt.c_str(), firstValue[column]);
        ImGui::TextUnformatted(string.data());
    }
}

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

void InitializeImGui()
{
    //___________
    //IMGUI SETUP
    //___________

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 460";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);

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

    SDL_ShowCursor(SDL_ENABLE);
}

int main(int argc, char* argv[])
{
    //Initilizers
    InitializeVideo();
    InitializeImGui();

    double freq = double(SDL_GetPerformanceFrequency()); //HZ
    double startTime = SDL_GetPerformanceCounter() / freq;
    double totalTime = SDL_GetPerformanceCounter() / freq - startTime; //sec
    double previousTime = totalTime;
    double LastShaderUpdateTime = totalTime;

    bool showIMGUI = true;
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& imGuiIO = ImGui::GetIO();
    bool g_cursorEngaged = true;
    CommandHandler playerInput;

    float p = 0.5f;
    Vertex vertices[] = {
      // |   Position    |      UV       |         Normal        |
        { {  p,  p,  p }, { 0.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } }, // +x
        { {  p, -p,  p }, { 0.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },
        { {  p,  p, -p }, { 1.0f, 1.0f }, {  1.0f,  0.0f,  0.0f } },
        { {  p, -p, -p }, { 1.0f, 0.0f }, {  1.0f,  0.0f,  0.0f } },

        { { -p,  p, -p }, { 0.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } }, // -x
        { { -p, -p, -p }, { 0.0f, 0.0f }, { -1.0f,  0.0f,  0.0f } },
        { { -p,  p,  p }, { 1.0f, 1.0f }, { -1.0f,  0.0f,  0.0f } },
        { { -p, -p,  p }, { 1.0f, 0.0f }, { -1.0f,  0.0f,  0.0f } },
            
        { {  p,  p,  p }, { 0.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } }, // +y
        { {  p,  p, -p }, { 0.0f, 0.0f }, {  0.0f,  1.0f,  0.0f } },
        { { -p,  p,  p }, { 1.0f, 1.0f }, {  0.0f,  1.0f,  0.0f } },
        { { -p,  p, -p }, { 1.0f, 0.0f }, {  0.0f,  1.0f,  0.0f } },
            
        { { -p, -p,  p }, { 0.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } }, // -y 
        { { -p, -p, -p }, { 0.0f, 0.0f }, {  0.0f, -1.0f,  0.0f } },
        { {  p, -p,  p }, { 1.0f, 1.0f }, {  0.0f, -1.0f,  0.0f } },
        { {  p, -p, -p }, { 1.0f, 0.0f }, {  0.0f, -1.0f,  0.0f } },
            
        { { -p,  p,  p }, { 0.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } }, // +z
        { { -p, -p,  p }, { 0.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
        { {  p,  p,  p }, { 1.0f, 1.0f }, {  0.0f,  0.0f,  1.0f } },
        { {  p, -p,  p }, { 1.0f, 0.0f }, {  0.0f,  0.0f,  1.0f } },
            
        { {  p,  p, -p }, { 0.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } }, // -z
        { {  p, -p, -p }, { 0.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
        { { -p,  p, -p }, { 1.0f, 1.0f }, {  0.0f,  0.0f, -1.0f } },
        { { -p, -p, -p }, { 1.0f, 0.0f }, {  0.0f,  0.0f, -1.0f } },
    };

    static_assert(arrsize(vertices) == 24, "");


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

    g_renderer.shaders[+Shader::Main]->UseShader();

    float   camera_dis              = 2.0f;
    float   camera_yaw              = 0.0f;
    float   camera_pitch            = pi / 4;
    Vec3    camera_velocity         = { };
    Vec3    camera_look_at_target   = { };



    VoxData voxels;
    LoadVoxFile(voxels, "assets/Test_01.vox");
    //LoadVoxFile(voxels, "assets/castle.vox");
    std::vector<Vertex_Voxel> voxel_vertices;
    u32 vox_mesh_index_count = CreateMeshFromVox(voxel_vertices, voxels);
    g_renderer.voxel_rast_vb->Upload(voxel_vertices.data(), voxel_vertices.size());
    assert(vox_mesh_index_count);

    Vec3 voxel_box_vertices[arrsize(vertices)] = {};
    for (i32 i = 0; i < arrsize(vertices); i++)
    {
        voxel_box_vertices[i] = vertices[i].p + p;
        voxel_box_vertices[i] = HadamardProduct(voxel_box_vertices[i], ToVec3(voxels.size));
    }
    g_renderer.voxel_box_vb->Upload(voxel_box_vertices, arrsize(voxel_box_vertices));
    {
        Texture::TextureParams voxel_indices_parameters = {
            .size = { VOXEL_MAX_SIZE, VOXEL_MAX_SIZE, VOXEL_MAX_SIZE },
            .minFilter  = GL_NEAREST,       //GL_LINEAR,
            .magFilter  = GL_NEAREST,       //GL_LINEAR,
            .wrapS      = GL_CLAMP_TO_EDGE, //GL_REPEAT,
            .wrapT      = GL_CLAMP_TO_EDGE, //GL_REPEAT,
            .internalFormat = GL_R8UI,
            .format     = GL_RED_INTEGER,
            .type       = GL_UNSIGNED_BYTE,
            .samples    = 1,
            .data       = voxels.color_indices[0].e,
        };

        Texture::TextureParams voxel_colors_parameters = {
            .size = { VOXEL_PALETTE_MAX, 0, 0 },
            .minFilter  = GL_NEAREST,       //GL_LINEAR,
            .magFilter  = GL_NEAREST,       //GL_LINEAR,
            .wrapS      = GL_CLAMP_TO_EDGE, //GL_REPEAT,
            .wrapT      = GL_CLAMP_TO_EDGE, //GL_REPEAT,
            .internalFormat = GL_RGBA,
            .format     = GL_RGBA,
            .type       = GL_UNSIGNED_BYTE,
            .samples    = 1,
            .data       = voxels.color_palette,
        };

        g_renderer.textures[Texture::Voxel_Indices] = new Texture(voxel_indices_parameters);
        g_renderer.textures[Texture::Color_Palette] = new Texture(voxel_colors_parameters);
    }

    while (g_running)
    {
        {
            ZoneScopedN("Frame Update:");
            totalTime = SDL_GetPerformanceCounter() / freq - startTime;
            float deltaTime = float(totalTime - previousTime);// / 10;
            previousTime = totalTime;
            //TODO: Time stepping for simulation
            //if (deltaTime > (1.0f / 60.0f))
                //deltaTime = (1.0f / 60.0f);

            //NOTE(CSH): Force the game to run at 1fps
            //WARNING(CSH): BUGGY
#if 0
            float delta_time_accumulator = 0;
            delta_time_accumulator += deltaTime;
            float desired_frame_time = 1.0f;
            if (delta_time_accumulator < desired_frame_time)
            {
                float sleep_time_s = desired_frame_time - deltaTime;
                Sleep_Thread(i64(sleep_time_s * 1000));
            }
            totalTime = SDL_GetPerformanceCounter() / freq - startTime;
            deltaTime = float(totalTime - previousTime);// / 10;
            previousTime = totalTime;
#endif

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
                                    playerInput.mouse.pos.x = SDLEvent.motion.x;
                                    playerInput.mouse.pos.y = g_renderer.size.y - SDLEvent.motion.y;

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
                            playerInput.mouse.pos.y = g_renderer.size.y - playerInput.mouse.pos.y;
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

            /**********************************************
             *
             * Camera Position
             *
             ***************/

            if (playerInput.keyStates[SDL_BUTTON_MIDDLE].down)
            {
                camera_yaw -= ((float(playerInput.mouse.pDelta.x) / float(g_renderer.size.x)) * (tau));
                camera_pitch += ((float(playerInput.mouse.pDelta.y) / float(g_renderer.size.y)) * (tau));
                float half_pi = pi / 2;
                float offset = 0.01f;
                camera_pitch = Clamp(camera_pitch, (-half_pi) + offset, half_pi - offset);
            }
            camera_dis = Max(1.0f, camera_dis - ((playerInput.mouse.wheelInstant.y / 4) * (0.3f * camera_dis)));

            Quat camera_rotation = gb_quat_euler_angles(camera_pitch, camera_yaw, 0.0f);
            Vec3 camera_3d_distance = { 0, 0, -camera_dis };
            Vec3 camera_position_swivel;
            gb_quat_rotate_vec3(&camera_position_swivel, camera_rotation, camera_3d_distance);

            //travel distance
            Vec3 wanted_direction = {   playerInput.keyStates[SDLK_a].down ? 1.0f : (playerInput.keyStates[SDLK_d].down ? -1.0f : 0.0f),
                                        0.0f,
                                        playerInput.keyStates[SDLK_w].down ? 1.0f : (playerInput.keyStates[SDLK_s].down ? -1.0f : 0.0f) };

            float speed = 10.0f;
            if (playerInput.keyStates[SDLK_LSHIFT].down)
                speed *= 10.0f;
            if (playerInput.keyStates[SDLK_LCTRL].down)
                speed /= 10.0f;
            Mat4 yaw_only_rotation;
            gb_mat4_from_quat(&yaw_only_rotation, gb_quat_euler_angles(0.0f, camera_yaw, 0.0f));
            Vec3 front = (yaw_only_rotation * GetVec4(wanted_direction, 0)).xyz;
            gb_vec3_norm0(&front, front);
            Vec3 targetVelocity = front * speed;
            camera_velocity = Converge(camera_velocity, targetVelocity, 16.0f, deltaTime);
            camera_look_at_target += camera_velocity * deltaTime;


            Mat4 projection_from_view;
            gb_mat4_perspective(&projection_from_view, tau / 4, float(g_renderer.size.x) / g_renderer.size.y, 0.1f, 2000.0f);
            Mat4 view_from_world;
            Vec3 camera_pos_world = camera_position_swivel + camera_look_at_target;
            gb_mat4_look_at(&view_from_world, camera_pos_world, camera_look_at_target, { 0,1,0 });

            Mat4 view_from_projection;
            Mat4 world_from_view;
            gb_mat4_inverse(&view_from_projection,  &projection_from_view);
            gb_mat4_inverse(&world_from_view,       &view_from_world);

#if 1
            Ray ray = MouseToRaycast(playerInput.mouse.pos, g_renderer.size, camera_pos_world, &view_from_projection, &world_from_view);
            AABB aabb = {
                .min = {},
                .max = ToVec3(voxels.size),
            };
            RaycastResult rr = RayVsAABB(ray, aabb);
            RaycastResult lc = {};
            if (rr.success)
            {
#if 1
                Ray linecast_ray = { rr.p, ray.direction };
                //RaycastResult lc = LineCast(ray, voxels, INFINITY);
                //lc = LineCast(ray, voxels, 1000.0f);
                lc = Linecast(ray, voxels, 1000.0f);
                if (lc.success)
                {
                    //AddCubeToRender(lc.p, transPurple, 1.1f);
                    AddCubeToRender(lc.p, transPurple, 0.1f);
                }
#else
                AddCubeToRender(rr.p, transPurple, 2);
#endif
            }
#endif

            //AddCubeToRender(aabb.Center(), transOrange, Vec3IntToVec3(voxels.size));
            
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

                            GenericImGuiTable("Camera_dis",     "%+08.2f",  &camera_dis, 1);
                            GenericImGuiTable("Camera_yaw",     "%+08.2f",  &camera_yaw, 1);
                            GenericImGuiTable("Camera_pitch",   "%+08.2f",  &camera_pitch, 1);
                            GenericImGuiTable("Camera_swivel",  "%+08.2f",  camera_position_swivel.e);
                            GenericImGuiTable("Camera_look_at", "%+08.2f",  camera_look_at_target.e);
                            GenericImGuiTable("Camera_world",   "%+08.2f",  camera_pos_world.e);
                            GenericImGuiTable("delta_time",     "%+08.8f",  &deltaTime, 1);
                            GenericImGuiTable("hit_pos",        "%+08.2f",  lc.p.e);
                            GenericImGuiTable("hit_suc",        "%i",       &lc.success, 1);
                            GenericImGuiTable("mouse_pos",      "%i",       playerInput.mouse.pos.e, 2);
                            GenericImGuiTable("ray_dir",        "%+08.2f",  ray.direction.e);

                            ImGui::EndTable();
                        }
                    }
                    ImGui::End();
                }
            }


            RenderUpdate(g_renderer.size, deltaTime);

            glClearDepth(1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if RASTERIZED_RENDERING == 0
            //Pathtraced voxel rendering
            {
                ZoneScopedN("Voxel Render");
                
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDisable(GL_CULL_FACE);
                static_assert(GL_TEXTURE1 == GL_TEXTURE0 + 1);
                static_assert(GL_TEXTURE2 == GL_TEXTURE0 + 2);
                g_renderer.textures[Texture::Voxel_Indices]->Bind(GL_TEXTURE0);
                g_renderer.textures[Texture::Color_Palette]->Bind(GL_TEXTURE1);
                g_renderer.shaders[+Shader::Voxel]->UseShader();
#if 1
                g_renderer.voxel_rast_ib->Bind();
                g_renderer.voxel_box_vb->Bind();
#else
                g_renderer.voxel_ib->Bind();
                g_renderer.voxel_vb->Bind();
#endif

#if 0
                glDisable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
#else
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);
#endif

                glEnableVertexArrayAttrib(g_renderer.vao, 0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vec3), 0);

                g_renderer.shaders[+Shader::Voxel]->UpdateUniformMat4("u_projection_from_view", 1, false, projection_from_view.e);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformMat4("u_view_from_world",      1, false, view_from_world.e);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformMat4("u_world_from_view",      1, false, world_from_view.e);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformMat4("u_view_from_projection", 1, false, view_from_projection.e);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformInt2("u_screen_size",          g_renderer.size);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformInt3("u_voxel_size",           voxels.size);
                g_renderer.shaders[+Shader::Voxel]->UpdateUniformVec3("u_camera_position",      camera_pos_world);

                glDrawElements(GL_TRIANGLES, 6 * 6, GL_UNSIGNED_INT, 0);
                glEnable(GL_CULL_FACE);
            }
            {
                Vec3 voxels_size = ToVec3(voxels.size);
                AddCubeToRender(voxels_size / 2.0f, transRed, voxels_size);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                RenderOpaqueCubes(      projection_from_view, view_from_world);
                RenderTransparentCubes( projection_from_view, view_from_world);
            }
#else



            //Rasterized voxel rendering
            {
                //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                g_renderer.shaders[+Shader::Voxel_Rast]->UseShader();
                g_renderer.voxel_rast_ib->Bind();
                g_renderer.voxel_rast_vb->Bind();

                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_TRUE);

                glEnableVertexArrayAttrib(g_renderer.vao, 0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, p));
                glEnableVertexArrayAttrib(g_renderer.vao, 1);
                glVertexAttribIPointer(1, 1, GL_UNSIGNED_INT,   sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, rgba));
                glEnableVertexArrayAttrib(g_renderer.vao, 2);
                glVertexAttribIPointer(2, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, n));
                glEnableVertexArrayAttrib(g_renderer.vao, 3);
                glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,  sizeof(Vertex_Voxel), (void*)offsetof(Vertex_Voxel, ao));

                g_renderer.shaders[+Shader::Voxel_Rast]->UpdateUniformMat4("u_projection_from_view", 1, false, projection_from_view.e);
                g_renderer.shaders[+Shader::Voxel_Rast]->UpdateUniformMat4("u_view_from_world",        1, false, view_from_world.e);

                glDrawElements(GL_TRIANGLES, vox_mesh_index_count, GL_UNSIGNED_INT, 0);
            }
            {
                //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                RenderOpaqueCubes(      projection_from_view, view_from_world);
                RenderTransparentCubes( projection_from_view, view_from_world);
            }
#endif



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
