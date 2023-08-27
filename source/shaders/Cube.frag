#version 460 core
//Color Texture:
layout (binding = 0) uniform sampler2D sampler;

in vec4 p_color;
in vec2 p_uv;

out vec4 color;

void main()
{
    vec4 tempColor = p_color * texture(sampler, p_uv);
    if (tempColor.a == 0)
        discard;

    color = tempColor;
}
