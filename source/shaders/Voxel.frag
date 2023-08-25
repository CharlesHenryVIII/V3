#version 460
//Voxel index list
layout (binding = 0) uniform usampler3D voxel_indices;
//Color Palette for index list
layout (binding = 1) uniform sampler1D  voxel_color_palette;

out vec4 color;

uniform mat4 u_perspective;
uniform mat4 u_view;

ivec3 MagicaToTexelFetch(ivec3 a)
{
    //NOTE(CSH):  magica voxel to texelFetch translation:
    // x -> z 
    // y -> x
    // z -> y
    // 9, 6, 3 -> 6, 3, 9
    return ivec3(a.y, a.z, a.x);
}

ivec3 GameVoxelToTexelFetch(ivec3 a)
{
    //To go from game voxel coordinates to texelFetch:
    // x -> z
    // y -> y
    // z -> x
    return ivec3(a.z, a.y, a.x);
}

void main()
{
    ivec3 voxel_pos = ivec3(6, 3, 9);
    voxel_pos = GameVoxelToTexelFetch(ivec3(9, 3, 10));
    //voxel_pos = MagicaToTexelFetch(ivec3(9, 10, 3));

    uvec4 voxel_index = texelFetch(voxel_indices,         voxel_pos,          0);
    if (voxel_index.r == 0)
        discard;
    vec4 voxel_color = texelFetch(voxel_color_palette,  int(voxel_index.r),  0);
    color.xyz = voxel_color.xyz;
    color.a = 1.0;
}
