#version 460
in vec2 f_uv;
in vec3 f_normal;
in vec3 f_color;
in float f_ao;
out vec4 color;

void main()
{
    //color.xyz = vec3(0);
	//color.xyz += c * f(f_normal, vec3(3, 3, 3));    //texture(sampler, f_uv).xyz * clamp((f(f_normal) + 0.1), 0, 1);
    //color.xyz += c * f(f_normal, vec3(-3, -3, 3)) * vec3(0.8, 0.2, 0.2);

    float ao = min(f_ao, 2) / 3.0;
    ao = clamp(ao, 0, 1);
    ao = ao * ao * ao;


    color.xyz = (max(vec3(1) - ao, 0.01)) * f_color;
    //color.xyz = f_color;
    color.a = 1.0;
    //color.xyz = f_normal;
}
