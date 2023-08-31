#version 460
#if 1
layout(location = 0) in vec3 v_world_position;
#else
layout(location = 0) in vec3 v_position;
#endif

uniform mat4 u_projection_from_view;
uniform mat4 u_view_from_world;

out vec2 f_uv;
out vec3 f_normal;

void main()
{
#if 1
    gl_Position = u_projection_from_view * u_view_from_world * vec4(v_world_position, 1.0);
#else
	gl_Position = vec4(v_position, 1);
#endif
}
