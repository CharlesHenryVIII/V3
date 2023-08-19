#version 460
layout(location = 0) in vec3 v_position;
layout(location = 1) in vec2 v_uv;
layout(location = 2) in vec3 v_normal;

//uniform mat4 u_model;
uniform mat4 u_perspective;
uniform mat4 u_view;
//uniform float u_time = 0;


out vec2 f_uv;
out vec3 f_normal;

void main()
{
	f_uv = v_uv / 16;
    f_normal = (u_view * vec4(v_normal, 0)).xyz;
	//gl_Position = u_perspective * u_view * u_model * vec4(v_position, 1);
	gl_Position = u_perspective * u_view * vec4(v_position, 1);
}
