#version 460
layout(location = 0) in vec3 v_world_position;
layout(location = 1) in uint v_rgba;
layout(location = 2) in uint v_n;
layout(location = 3) in uint v_ao;
//Vec3  p;
//U32Pack rgba;
//u8 n;
//u8 ao;

uniform mat4 u_projection_from_view;
uniform mat4 u_view_from_world;
out vec2 f_uv;
out vec3 f_normal;
out vec3 f_color;
out float f_ao;

const vec3 world_face_normals[6] = vec3[6](
    vec3(  1.0,  0.0,  0.0 ),
    vec3( -1.0,  0.0,  0.0 ),
    vec3(  0.0,  1.0,  0.0 ),
    vec3(  0.0, -1.0,  0.0 ),
    vec3(  0.0,  0.0,  1.0 ),
    vec3(  0.0,  0.0, -1.0 )
);

void main()
{
    f_ao = v_ao;
    f_normal = (u_view_from_world * vec4(world_face_normals[v_n], 0)).xyz;
    f_color.r = float((v_rgba      ) & 0xFF);
    f_color.g = float((v_rgba >>  8) & 0xFF);
    f_color.b = float((v_rgba >> 16) & 0xFF);
    //f_color = vec3(v_rgba.x, v_rgba.y, v_rgba.z);
    //f_color = vec3((v_rgba >> 24), v_rgba >> 16, v_rgba >> 8); 
    f_color = f_color / 255;
    //f_color = vec3(float(v_rgba >> 24) / 255, 
                    //float(v_rgba >> 16) / 255, 
                    //float(v_rgba >> 8) /255); 

	gl_Position = u_projection_from_view * u_view_from_world * vec4(v_world_position, 1);
}
