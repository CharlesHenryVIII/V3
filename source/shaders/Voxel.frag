#version 460
//Voxel index list
layout (binding = 0) uniform usampler3D voxel_indices;
//Color Palette for index list
layout (binding = 1) uniform sampler1D  voxel_color_palette;

out vec4 color;

uniform mat4    u_view_from_projection;
uniform mat4    u_world_from_view;
uniform ivec2   u_screen_size;
uniform vec3    u_camera_position;

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

struct Ray {
    vec3 origin;
    vec3 direction;
};

Ray PixelToRay(ivec2 pixel)
{
    float x = (2.0 * pixel.x) / u_screen_size.x - 1.0;
    float y = 1.0 - (2.0 * (u_screen_size.y - pixel.y)) / u_screen_size.y;;
    float z = 1.0;
    vec3 ray_nds = vec3( x, y, z );
    vec4 ray_clip = vec4( ray_nds.xy, +1.0, 1.0 );

    //From Clip to View
    vec4 ray_view = u_view_from_projection * ray_clip;
    ray_view = vec4( ray_view.xy, -1.0, 0.0 );

    //From View to World
    vec3 ray_world = (u_world_from_view * ray_view).xyz;

    //Normalize
    vec3 ray_world_n = normalize(ray_world);
    Ray ray;
    ray.origin      = u_camera_position;
    ray.direction   = ray_world_n;
    return ray;
}


void main()
{
    Ray ray = PixelToRay(ivec2(gl_FragCoord.xy));

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
