#include "GpuSharedData.h"

//**************
//VERTEX SHADER
//**************

struct VS_Output {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

struct VS_Input {
    float2 pos : POSITION;
};

VS_Output Vertex_Main(VS_Input input)
{
    VS_Output output;
    output.position = float4(input.pos, 0, 1);
    output.uv.x = (input.pos.x + 1) / 2;
    output.uv.y = 1 - (input.pos.y + 1) / 2;
    return output;
}




//**************
//PIXEL SHADER
//**************
struct PS_Output {
    float4 color : SV_Target;
};

//Previous Render Target
Texture2D   previous_target         TEXTURE_REGISTER(SLOT_PREVIOUS_TARGET);
sampler     previous_target_sampler SAMPLER_REGISTER(SLOT_PREVIOUS_TARGET_SAMPLER);
//Previous Depth Target
Texture2D   previous_depth          TEXTURE_REGISTER(SLOT_PREVIOUS_DEPTH);
sampler     previous_depth_sampler  SAMPLER_REGISTER(SLOT_PREVIOUS_DEPTH_SAMPLER);

static const float FLT_INF     = 1.#INF;
static const float FLT_MAX     = 3.402823466e+38F;
static const float FLT_MIN     = 1.175494351e-38;
static const float FLT_EPSILON = 1.192092896e-07;
static const float pi = 3.14159;
static const float tau = 2 * pi;

PS_Output Pixel_Main(VS_Output input)
{
    PS_Output output;
    output.color = previous_target.Sample(previous_target_sampler, input.uv);
    return output;
}
