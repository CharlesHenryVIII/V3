#ifdef __cplusplus
#pragma once
#endif

#define SLOT_CB_COMMON 0
//Voxel Draw Call
#define SLOT_VOXEL_INDICES 0
#define SLOT_VOXEL_INDICES_SAMPLER 0
#define SLOT_RANDOM_TEXTURE 2
#define SLOT_RANDOM_TEXTURE_SAMPLER 2
#define SLOT_VOXEL_MATERIALS 3
//Cube Draw call
#define SLOT_CUBE_TEXTURE 0
#define SLOT_CUBE_TEXTURE_SAMPLER 0

#ifdef __cplusplus

#define STRUCT_PREFIX   struct
#define STRUCT_SUFFIX(...)
#define TEXTURE_REGISTER(...)
#define SAMPLER_REGISTER(...)
#define STRUCT_PACK_START _Pragma("pack(push, 1)")
#define STRUCT_PACK_END _Pragma("pack(pop)")

#else
#define STRUCT_PREFIX   cbuffer
#define STRUCT_SUFFIX(slot)     : register(b##slot)
#define TEXTURE_REGISTER(slot)  : register(t##slot)
#define SAMPLER_REGISTER(slot)  : register(s##slot)
#define STRUCT_PACK_START
#define STRUCT_PACK_END

#define U32Pack uint
#define Vec2  float2
#define Vec3  float3
#define Vec4  float4
#define Mat2  float2x2
#define Mat3  float3x3
#define Mat4  float4x4
#define Quat  float4
#define Vec2d double2
#define Vec3d double3
#define Vec4d double4
#define Mat2d double2x2
#define Mat3d double3x3
#define Mat4d double4x4
#define Quatd double4
#define Vec2I int2
#define Vec3I int3
#define Vec4I int4
#define Vec2U uint2
#define Vec3U uint3
#define Vec4U uint4
#define Mat2I int2x2
#define Mat3I int3x3
#define Mat4I int4x4



#endif

STRUCT_PREFIX CB_Common STRUCT_SUFFIX(SLOT_CB_COMMON) {
    Mat4 projection_from_view;
    Mat4 view_from_world;
    Mat4 view_from_projection;
    Mat4 world_from_view;
    Vec2I screen_size;
    Vec2I random_texture_size;
    Vec3I voxel_size;
    float total_time;
    Vec3 camera_position;
    float _pad0;
};

STRUCT_PACK_START
struct VoxMaterial {
    float metalness;
    float roughness;    //Surface Roughness:        Range from 0 to 100
    float spec;         //Specular reflectivity:    Range from 0 to 100
    float flux;         //Radiant flux:             Range from 1 to 5
    float emit;
    float ri;           //Refractive index:         Range from 1.00 to 3.00
    float metal;        //metalness
    U32Pack color;
};
STRUCT_PACK_END
