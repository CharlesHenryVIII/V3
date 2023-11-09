#include "GpuSharedData.h"

//**************
//VERTEX SHADER
//**************

struct VS_Output {
    float4 color    : COLOR;
    float4 p        : SV_POSITION;
    float3 n        : NORMAL;
};

struct VS_Input {
    float4 color    : COLOR;
    float3 p        : POSITION;
    float3 n        : NORMAL;
};

VS_Output Vertex_Main(VS_Input input)
{
    VS_Output output;
    output.color = input.color;
    output.p = mul(projection_from_view, mul(view_from_world, float4(input.p, 1.0)));
    output.n = float4(input.n, 0.0).xyz;

    return output;
}





//**************
//PIXEL SHADER
//**************

Texture2D t_texture            TEXTURE_REGISTER(SLOT_PRIMITIVE_TEXTURE);
sampler   t_texture_sampler    SAMPLER_REGISTER(SLOT_PRIMITIVE_TEXTURE_SAMPLER);

struct PS_Output {
    float4 color : SV_Target;
    //float depth : SV_Depth;
};

PS_Output Pixel_Main(VS_Output input)
{
    PS_Output output;
    output.color.a = 1;
    const float3 sun_color = { 1, 1, 1 };
    const float3 sun_dir = normalize(float3(1, 1, -1));
    const float3 ambient = { 0.2, 0.2, 0.2 };

    float sun_diffuse = max(dot(input.n, sun_dir), 0.0);
    float3 diffuse = sun_color * sun_diffuse;
    output.color.rgb = (max(ambient + diffuse, 0.01)) * input.color.rgb;

    //output.depth = output.depth = GetDepth(length(input.position.xyz - camera_position));
    return output;
}
