#version 460 core
layout(location = 0) in vec3 v_world_position;
layout(location = 1) in vec4 v_color;

uniform mat4 u_projection_from_view;
uniform mat4 u_view_from_world;

out vec4 p_color;
out vec2 p_uv;

const vec2 face_uv[4] = vec2[4](
    vec2(0, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);

void main()
{
    gl_Position = u_projection_from_view * u_view_from_world * vec4(v_world_position, 1.0);
    p_color = v_color;
    p_uv = face_uv[gl_VertexID % 4];
}
