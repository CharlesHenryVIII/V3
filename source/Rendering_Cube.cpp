#include "Rendering_Cube.h"
#include "Math.h"
#include "Vox.h"
#include "Rendering.h"

#include "Tracy.hpp"

///
/// Add Cubes To Render
///
struct Vertex_Cube {
    Vec3 p;
    Color color;
};
std::vector<Vertex_Cube> s_cubesToDraw_transparent;
std::vector<Vertex_Cube> s_cubesToDraw_opaque;

void AddCubeToRender(Vec3 p, Color color, float scale)
{
    AddCubeToRender(p, color, { scale, scale, scale });
}
void AddCubeToRender(Vec3 p, Color color, Vec3  scale)
{
    assert(Abs(scale) == scale);
    Vertex_Cube c;

    auto* list = &s_cubesToDraw_opaque;
    if (color.a != 1.0f)
        list = &s_cubesToDraw_transparent;

    for (i32 f = 0; f < +Face::Count; f++)
        for (i32 v = 0; v < 4; v++)
        {
            c.p = p + HadamardProduct((cubeVertices[f].e[v] - 0.5f), scale);
            c.color = color;
            list->push_back(c);
        }
}

void RenderCubesInternal(const Mat4& projection, const Mat4& view, std::vector<Vertex_Cube>& cubesToDraw)
{
    if (cubesToDraw.size() == 0)
        return;
    g_renderer.voxel_rast_ib->Bind();
    ShaderProgram* sp = g_renderer.shaders[+Shader::Cube];
    sp->UseShader();
    glActiveTexture(GL_TEXTURE0);
    g_renderer.textures[Texture::T::Plain]->Bind();
    VertexBuffer vBuffer = VertexBuffer();
    {
        ZoneScopedN("Upload Cubes");
        vBuffer.Upload(cubesToDraw);
        sp->UpdateUniform("u_projection_from_view", projection, false);
        sp->UpdateUniform("u_view_from_world",      view,       false);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, p));
    glEnableVertexArrayAttrib(g_renderer.vao, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex_Cube), (void*)offsetof(Vertex_Cube, color));
    glEnableVertexArrayAttrib(g_renderer.vao, 1);
    {
        ZoneScopedN("Render Cubes");
        glDrawElements(GL_TRIANGLES, (GLsizei)((cubesToDraw.size() / 24) * 36), GL_UNSIGNED_INT, 0);
    }
    //g_renderer.numTrianglesDrawn += 12 * (uint32)cubesToDraw.size();

    cubesToDraw.clear();
}
void RenderTransparentCubes(const Mat4& projection, const Mat4& view)
{
    ZoneScopedN("Upload and Render Transparent Cubes");
    RenderCubesInternal(projection, view, s_cubesToDraw_transparent);
}

void RenderOpaqueCubes(const Mat4& projection, const Mat4& view)
{
    ZoneScopedN("Upload and Render Opaque Cubes");
    RenderCubesInternal(projection, view, s_cubesToDraw_opaque);
}
