#version 460
layout(location = 0) in vec3 v_position;

uniform mat4 u_perspective;
uniform mat4 u_view;

out vec2 f_uv;
out vec3 f_normal;

void main()
{
	gl_Position = vec4(v_position, 1);
}
