#define STB_IMAGE_IMPLEMENTATION
#define GB_MATH_IMPLEMENTATION
#include <SDL.h>

#include "imgui.h"
#include "ImGui/backends/imgui_impl_sdl2.h"
#include "ImGui/backends/imgui_impl_dx11.h"
#include "Tracy.hpp"
#include "stb/stb_image.h"

#include "Math.h"
#include "Debug.h"
#include "Timers.h"
#include "Rendering.h"
#include "Input.h"
#include "WinInterop_File.h"
#include "Vox.h"
#include "Raycast.h"

#include <unordered_map>
#include <vector>

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
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    ImGuiIO& imGuiIO = ImGui::GetIO();
    bool g_cursorEngaged = true;
    CommandHandler playerInput;

    float   camera_dis              = 2.0f;
    float   camera_yaw              = 0.0f;
    float   camera_pitch            = pi / 4;
    Vec3    camera_velocity         = { };
    Vec3    camera_look_at_target   = { };



    VoxData voxels;
    LoadVoxFile(voxels, "assets/Test_01.vox");
    //LoadVoxFile(voxels, "assets/castle.vox");
#if RASTERIZED_RENDERING == 1
    std::vector<Vertex_Voxel> voxel_vertices;
    u32 vox_mesh_index_count = CreateMeshFromVox(voxel_vertices, voxels);
    if (voxel_vertices.size())
        g_renderer.voxel_rast_vb->Upload(voxel_vertices.data(), voxel_vertices.size(), sizeof(voxel_vertices[0]));
    assert(vox_mesh_index_count);
#endif

    {

        Texture::TextureParams voxel_indices_parameters = {
            .size   = { VOXEL_MAX_SIZE, VOXEL_MAX_SIZE, VOXEL_MAX_SIZE },
            .format = Texture::Format_R8_UINT,
            .mode   = Texture::Address_Clamp,
            .filter = Texture::Filter_Point,
            .render_target = false,
            .bytes_per_pixel = sizeof(voxels.color_indices[0].e[0][0][0]),
            .data   = voxels.color_indices[0].e,
        };
        CreateTexture(&g_renderer.textures[Texture::Index_Voxel_Indices], voxel_indices_parameters);
        CreateGpuBuffer(&g_renderer.structure_voxel_materials,"voxel_materials", false, GpuBuffer::Type::Structure);
        g_renderer.structure_voxel_materials->Upload(voxels.materials, VOXEL_PALETTE_MAX, sizeof(voxels.materials[0]));
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
            Vec3 camera_position_swivel = gb_quat_rotate_vec3(camera_rotation, camera_3d_distance);

            //travel distance
            Vec3 wanted_direction = {   playerInput.keyStates[SDLK_a].down ? 1.0f : (playerInput.keyStates[SDLK_d].down ? -1.0f : 0.0f),
                                        0.0f,
                                        playerInput.keyStates[SDLK_w].down ? 1.0f : (playerInput.keyStates[SDLK_s].down ? -1.0f : 0.0f) };

            float speed = 10.0f;
            if (playerInput.keyStates[SDLK_LSHIFT].down)
                speed *= 10.0f;
            if (playerInput.keyStates[SDLK_LCTRL].down)
                speed /= 10.0f;
            Mat4 yaw_only_rotation = gb_mat4_from_quat(gb_quat_euler_angles(0.0f, camera_yaw, 0.0f));
            Vec3 front = (yaw_only_rotation * GetVec4(wanted_direction, 0)).xyz;
            front = gb_vec3_norm0(front);
            Vec3 targetVelocity = front * speed;
            camera_velocity = Converge(camera_velocity, targetVelocity, 16.0f, deltaTime);
            camera_look_at_target += camera_velocity * deltaTime;


            //Mat4 projection_from_view = gb_mat4_perspective(tau / 4, float(g_renderer.size.x) / g_renderer.size.y, 1.0f, 1000.0f);
            Mat4 projection_from_view = gb_mat4_perspective_directx_rh(tau / 4, float(g_renderer.size.x) / g_renderer.size.y, 1.0f, 1000.0f);
            Vec3 camera_pos_world = camera_position_swivel + camera_look_at_target;
            Mat4 view_from_world        = gb_mat4_look_at(camera_pos_world, camera_look_at_target, { 0,1,0 });
            Mat4 view_from_projection   = gb_mat4_inverse(projection_from_view);
            Mat4 world_from_view        = gb_mat4_inverse(view_from_world);
#if 0
            //Testing broken linecasting
            projection_from_view = {
             0.562500715f,  0.00000000f,   0.00000000f,   0.00000000f,
             0.00000000f,   1.00000131f,   0.00000000f,   0.00000000f,
             0.00000000f,   0.00000000f,  -1.00200200f,  -1.00000000f,
             0.00000000f,   0.00000000f,  -2.00200200f,   0.00000000f
            };

            view_from_world = {
             0.252490938f,  0.225881487f, -0.940864384f,  0.00000000f,
            -0.00000000f,   0.972369909f,  0.233445287f,  0.00000000f,
             0.967599273f, -0.0589428209f, 0.245514587f,  0.00000000f,
            -27.1199570f,  -3.91920853f,   2.30917740f,   1.00000000f
            };

            world_from_view = {
             0.252490938f, -0.00000000f,   0.967599332f, -0.00000000f,
             0.225881517f,  0.972370028f, -0.0589428283f, 0.00000000f,
            -0.940864563f,  0.233445331f,  0.245514616f, -0.00000000f,
             9.90544319f,   3.27185416f,   25.4433079f,   1.00000000f
            };

            view_from_projection = {
             1.77777553f,  0.00000000f,   -0.00000000f,   0.00000000f,
             0.00000000f,  0.999998689f,   0.00000000f,  -0.00000000f,
            -0.00000000f,  0.00000000f,   -0.00000000f,  -0.499499977f,
             0.00000000f, -0.00000000f,   -1.00000000f,   0.500500023f
            };

            camera_pos_world = { 9.90544319f, 3.27185392f, 25.4433041f };

            Ray ray_working = MouseToRaycast({ 557, 587 }, g_renderer.size, camera_pos_world, view_from_projection, world_from_view);
            Ray ray_broken  = MouseToRaycast({ 557, 588 }, g_renderer.size, camera_pos_world, view_from_projection, world_from_view);

            RaycastResult working = Linecast(ray_working, voxels, 1000.0f);
            RaycastResult broken  = Linecast(ray_broken, voxels, 1000.0f);
#endif

            Ray ray = MouseToRaycast(playerInput.mouse.pos, g_renderer.size, camera_pos_world, view_from_projection, world_from_view);
#if 0
            RaycastResult voxel_hit_result = RayVsVoxel(ray, voxels);
            if (voxel_hit_result.success)
                AddCubeToRender(voxel_hit_result.p, transPurple, 0.25f);

                
#else
#define RAY_BOUNCES 3
            RaycastResult voxel_rays[RAY_BOUNCES] = {};
            for (i32 i = 0; i < RAY_BOUNCES; i++)
            {
                voxel_rays[i] = RayVsVoxel(ray, voxels);
                if (voxel_rays[i].success)
                {
                    Color c = {};
                    c.e[i] = 1.0f;
                    c.a = 0.5f;
                    AddCubeToRender(voxel_rays[i].p, c, 0.5f, false);
                    ray.direction = ReflectRay(ray.direction, voxel_rays[i].normal);
                    ray.origin = voxel_rays[i].p + ray.direction * 0.0001f;
                }
            }
            {
                Vec3 voxel_size = ToVec3(voxels.size);
                AddCubeToRender(voxel_size / 2.0f, Orange, voxel_size, true);
            }

            RaycastResult voxel_hit_result = voxel_rays[0];

            {
                float scale = 0.5;
                float scale_half = scale * 0.5f;
                float scale_double = scale * 2;
                AddCubeToRender({},              White, scale, false);
                AddCubeToRender({ scale, 0, 0 }, Red,   { scale_double, scale_half,     scale_half   }, false);
                AddCubeToRender({ 0, scale, 0 }, Green, { scale_half,   scale_double,   scale_half   }, false);
                AddCubeToRender({ 0, 0, scale }, Blue,  { scale_half,   scale_half,     scale_double }, false);
                
#if 0
                for (i32 x = -1; x < 2; x++)
                {
                    for (i32 y = -1; y < 2; y++)
                    {
                        for (i32 z = -1; z < 2; z++)
                        {
                            if (x == 0 && y == 0 && z == 0)
                                continue;
                            Vec3 p = { (float)x, (float)y, (float)z };
                            Color c = { 0, 0, 0, 1 };
                            c.rgb = (p + 1.0f) / 2.0f;
                            AddTetrahedronToRender(p, p, c, { 0.5, 1, 0.5 }, false);
                        }
                    }
                }
#endif
            }

#endif

            if (showIMGUI)
            {
                ZoneScopedN("ImGui Update");
                float transformInformationWidth = 0.0f;
                {
                    // Start the Dear ImGui frame
                    ImGui_ImplDX11_NewFrame();
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

                            float framerate = 1 / deltaTime;
                            GenericImGuiTable("Camera_dis",     "%+08.2f",  &camera_dis, 1);
                            GenericImGuiTable("Camera_yaw",     "%+08.2f",  &camera_yaw, 1);
                            GenericImGuiTable("Camera_pitch",   "%+08.2f",  &camera_pitch, 1);
                            GenericImGuiTable("Camera_swivel",  "%+08.2f",  camera_position_swivel.e);
                            GenericImGuiTable("Camera_look_at", "%+08.2f",  camera_look_at_target.e);
                            GenericImGuiTable("Camera_world",   "%+08.2f",  camera_pos_world.e);
                            GenericImGuiTable("delta_time",     "%+08.8f",  &deltaTime, 1);
                            GenericImGuiTable("framerate",      "%+08.8f",  &framerate, 1);
                            GenericImGuiTable("hit_pos",        "%+08.2f",  voxel_hit_result.p.e);
                            GenericImGuiTable("hit_suc",        "%i",       &voxel_hit_result.success, 1);
                            GenericImGuiTable("mouse_pos",      "%i",       playerInput.mouse.pos.e, 2);
                            GenericImGuiTable("ray_dir",        "%+08.2f",  ray.direction.e);

                            ImGui::EndTable();
                        }
                    }
                    ImGui::End();
                }
            }


            RenderUpdate(g_renderer.size, deltaTime);

            CB_Common common = {
                .projection_from_view = projection_from_view,
                .view_from_world = view_from_world,
                .view_from_projection = view_from_projection,
                .world_from_view = world_from_view,
                .screen_size = g_renderer.size,
                .random_texture_size = g_renderer.textures[Texture::Index_Random]->m_size.xy,
                .voxel_size = voxels.size,
                .total_time = float(totalTime),
                .camera_position = camera_pos_world,
                ._pad0 = 0.0f,
            };
            g_renderer.cb_common->Upload(&common, 1, sizeof(common));
            g_renderer.cb_common->Bind(SLOT_CB_COMMON, GpuBuffer::BindLocation::All);

#if RASTERIZED_RENDERING == 0
            //Pathtraced voxel rendering
            {
                ZoneScopedN("Voxel Render");
                DrawPathTracedVoxels();
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
#endif

            {
                ZoneScopedN("Cube Render");
                g_renderer.cb_common->Bind(SLOT_CB_COMMON, GpuBuffer::BindLocation::All);
                RenderPrimitives();
            }
            {
                ZoneScopedN("Final Draw");
                FinalDraw();
            }

            {
                ZoneScopedN("ImGui Render");
                if (showIMGUI)
                {
                    ImGui::Render();
                    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
                }
            }
        }
        {
            ZoneScopedN("Frame End");
            RenderPresent();
        }
        FrameMark;
    }
    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    //SDL_GL_DeleteContext(g_renderer.GL_Context);
    SDL_DestroyWindow(g_renderer.SDL_Context);
    SDL_Quit();
    return 0;

}
