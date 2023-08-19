#version 460
uniform sampler2D sampler;

//uniform float u_time = 0;

float f(vec3 normal, vec3 pos)
{
    return clamp(dot(normal, pos), 0, 1);
}

in vec2 f_uv;
in vec3 f_normal;
out vec4 color;
void main()
{
    vec3 c = texture(sampler, f_uv).xyz;

    //color.xyz = vec3(0);
	//color.xyz += c * f(f_normal, vec3(3, 3, 3));    //texture(sampler, f_uv).xyz * clamp((f(f_normal) + 0.1), 0, 1);
    //color.xyz += c * f(f_normal, vec3(-3, -3, 3)) * vec3(0.8, 0.2, 0.2);

    color.xyz = c;
    color.a = 1.0;
    //color.xyz = f_normal;
}
