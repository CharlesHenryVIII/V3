#include "GpuSharedData.h"

//**************
//VERTEX SHADER
//**************

struct VS_Output {
    float4 position : SV_POSITION;
    float4 color    : COLOR;
};

struct VS_Input {
    float4 color    : COLOR;
    float3 pos      : POSITION;
};

VS_Output Vertex_Main(VS_Input input)
{
    VS_Output output;
    output.position = mul(projection_from_view, mul(view_from_world, float4(input.pos, 1.0)));
    output.color = input.color;
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

    float4 color = input.color;

    output.color = color;
    //output.depth = output.depth = GetDepth(length(input.position.xyz - camera_position));
    return output;
}
