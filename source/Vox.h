#pragma once
#include "Math.h"
#include "Debug.h"
#include "GpuSharedData.h"

#include <string>
#include <vector>

struct Vertex_Voxel;
struct VoxelMesh {
    std::vector<Vec3I> sizes;
    std::vector<std::vector<Vertex_Voxel>> vertices;
    std::vector<std::vector<u32>> indices;
};
#define VOXEL_MAX_SIZE 64
#define VOXEL_PALETTE_MAX 256
//struct Voxels {
//    Uint32Pack e[VOXEL_MAX_SIZE][VOXEL_MAX_SIZE][VOXEL_MAX_SIZE] = {};
//};
struct VoxelBlockData {
    u8 e[VOXEL_MAX_SIZE][VOXEL_MAX_SIZE][VOXEL_MAX_SIZE] = {};
};
enum class Face : u8 {
    Right,
    Left,
    Top,
    Bot,
    Back,
    Front,
    Count,
};
ENUMOPS(Face);
extern Vec3 faceNormals[+Face::Count];

#pragma pack(push, 1)
struct Vertex_Voxel {
    Vec3  p;
    U32Pack rgba;
    u8 n;
    u8 ao;
    //u8 _unused_1;
    //u8 _unused_2;
};
//struct VoxMaterial {
//    float metalness;
//    float roughness;    //Surface Roughness:        Range from 0 to 100
//    float spec;         //Specular reflectivity:    Range from 0 to 100
//    //float ior;      // ????
//    //float att;      // ????
//    float flux;     //Radiant flux:             Range from 1 to 5
//    float emit;
//    float ri;       //Refractive index:         Range from 1.00 to 3.00
//    //float d;        // ????
//    float metal;    //metalness
//    //std::string plastic; //is this in use?
//    U32Pack color;
//};
struct VoxData {
    VoxMaterial                 materials[VOXEL_PALETTE_MAX] = {};
    //U32Pack                     color_palette[VOXEL_PALETTE_MAX];
    std::vector<VoxelBlockData> color_indices;
    Vec3I                       size;
};
#pragma pack(pop)

bool LoadVoxFile(VoxData& out_voxels, const std::string& filePath);
u32 CreateMeshFromVox(std::vector<Vertex_Voxel>& vertices, const VoxData& voxel_data);
