#version 460
//Voxel index list
layout (binding = 0) uniform usampler3D voxel_indices;
//Color Palette for index list
layout (binding = 1) uniform sampler1D  voxel_color_palette;

out vec4 color;

uniform mat4    u_view_from_projection;
uniform mat4    u_world_from_view;
uniform ivec2   u_screen_size;
uniform ivec3   u_voxel_size;
uniform vec3    u_camera_position;

const float FLT_INF     = 1.0 / 0.0;
const float FLT_MIN     = 1.175494351e-38;
const float FLT_EPSILON = 1.192092896e-07;

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

uint GetIndexFromGameVoxelPosition(ivec3 p)
{
    uvec4 voxel_index = texelFetch(voxel_indices, GameVoxelToTexelFetch(p), 0);
    return voxel_index.r;
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

struct RaycastResult {
    uint voxel_index;
    vec3 p;
    float distance_mag;
    vec3 normal;
};

RaycastResult LineCast(const Ray ray, float ray_length)
{
    RaycastResult result;
    result.voxel_index  = 0;
    result.p            = vec3(0);
    result.distance_mag = 0;
    result.normal       = vec3(0);

    vec3 p = ray.origin;
    vec3 line_step = vec3(0);
    line_step.x = ray.direction.x >= 0 ? 1.0 : -1.0;
    line_step.y = ray.direction.y >= 0 ? 1.0 : -1.0;
    line_step.z = ray.direction.z >= 0 ? 1.0 : -1.0;
    vec3 pClose = ray.origin + (line_step / 2);
    vec3 tMax = abs((pClose - ray.origin) / ray.direction);
    vec3 tDelta = abs(1.0 / ray.direction);
    
    while (result.voxel_index == 0)
    {
        if (distance(p, ray.origin) > ray_length)
            break;

        result.normal = vec3(0);
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            p.x += line_step.x;
            tMax.x += tDelta.x;
            result.normal.x = float(-1.0) * line_step.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z)
        {
            p.y += line_step.y;
            tMax.y += tDelta.y;
            result.normal.y = float(-1.0) * line_step.y;
        }
        else 
        {
            p.z += line_step.z;
            tMax.z += tDelta.z;
            result.normal.z = float(-1.0) * line_step.z;
        }

        //ivec3 voxel_p = ivec3(trunc(p).xyz);
        ivec3 voxel_p = ivec3(p);
        result.p = p;  //Vec3IntToVec3(voxel_p);
        if (voxel_p.x > u_voxel_size.x || voxel_p.y > u_voxel_size.y || voxel_p.z > u_voxel_size.z)
            break;
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            continue;
        result.voxel_index = GetIndexFromGameVoxelPosition(voxel_p);
    }
    result.distance_mag = distance(ray.origin, p);
    return result;
}

struct AABB {
    vec3 min;
    vec3 max;
};


RaycastResult RayVsAABB(const Ray ray, const AABB box)
{
    RaycastResult r;
    r.voxel_index  = 0;
    r.p            = vec3(0);
    r.distance_mag = 0;
    r.normal       = vec3(0);
    float tmin = 0;
    float tmax = FLT_INF;

    for (int slab = 0; slab < 3; ++slab)
    {
        if (abs(ray.direction[slab]) < FLT_EPSILON)
        {
            // Ray is parallel to the slab
            if (ray.origin[slab] < box.min[slab] || ray.origin[slab] > box.max[slab])
                return r;
        }
        else
        {
            float ood = 1.0 / ray.direction[slab];
            float t1 = (box.min[slab] - ray.origin[slab]) * ood;
            float t2 = (box.max[slab] - ray.origin[slab]) * ood;
            if (t1 > t2)
            {
                float temp = t1;
                t1 = t2;
                t2 = temp;
            }
            tmin = max(tmin, t1);
            tmax = min(tmax, t2);
            if (tmin > tmax)
                return r;
        }
    }
    r.p = ray.origin + ray.direction * tmin;
    r.distance_mag = tmin;

    const vec3 normals[6] = {
        vec3(  1.0,  0.0,  0.0 ),
        vec3( -1.0,  0.0,  0.0 ),
        vec3(  0.0,  1.0,  0.0 ),
        vec3(  0.0, -1.0,  0.0 ),
        vec3(  0.0,  0.0,  1.0 ),
        vec3(  0.0,  0.0, -1.0 ),
    };

    vec3 v[6];
    v[0] = vec3( box.max.x, r.p.y,      r.p.z );
    v[1] = vec3( box.min.x, r.p.y,      r.p.z );
    v[2] = vec3( r.p.x,     box.max.y,  r.p.z );
    v[3] = vec3( r.p.x,     box.min.y,  r.p.z );
    v[4] = vec3( r.p.x,     r.p.y,      box.max.z );
    v[5] = vec3( r.p.x,     r.p.y,      box.min.z );

    float currentDistance = 0;
    float ClosestDistance = FLT_INF;
    int closestFace = 0;
    for (int i = 0; i < 6; i++)
    {
        currentDistance = distance(r.p, v[i]);
        if (currentDistance < ClosestDistance)
        {
            ClosestDistance = currentDistance;
            closestFace = i;
        }
    }
    r.normal = normals[closestFace];

    r.voxel_index = 1;
    return r;
}

void main()
{

    Ray ray = PixelToRay(ivec2(gl_FragCoord.xy));

    AABB box = AABB(vec3(0), vec3(u_voxel_size.x, u_voxel_size.y, u_voxel_size.z ));

    RaycastResult ray_aabb = RayVsAABB(ray, box);
    if (ray_aabb.voxel_index != 0)
    {
        ray.origin = ray_aabb.p;
#if 1
        RaycastResult ray_voxel = LineCast(ray, 1000.0);
#else
        RaycastResult ray_voxel = LineCast(ray, FLT_INF);
#endif
        if (ray_voxel.voxel_index != 0)
        {
            vec4 voxel_color = texelFetch(voxel_color_palette, int(ray_voxel.voxel_index), 0);
            color.xyz = voxel_color.xyz;
            color.a = 1.0;

            vec3 max_aabb_corner_from_ray;
            if (abs(u_camera_position.x - box.min.x) > abs(u_camera_position.x - box.max.x))
            {
                max_aabb_corner_from_ray.x = box.min.x;
            }
            else
            {
                max_aabb_corner_from_ray.x = box.max.x;
            }
            if (abs(u_camera_position.y - box.min.y) > abs(u_camera_position.y - box.max.y))
            {
                max_aabb_corner_from_ray.y = box.min.y;
            }
            else
            {
                max_aabb_corner_from_ray.y = box.max.y;
            }
            if (abs(u_camera_position.z - box.min.z) > abs(u_camera_position.z - box.max.z))
            {
                max_aabb_corner_from_ray.z = box.min.z;
            }
            else
            {
                max_aabb_corner_from_ray.z = box.max.z;
            }
            float max_voxel_distance = distance(u_camera_position, max_aabb_corner_from_ray);

            gl_FragDepth = ray_voxel.distance_mag / max_voxel_distance;
        }
        else
        {
            discard;
        }
    }
    else
    {
        discard;
    }



#if 0 //Getting texel fetch to work with the indices
    ivec3 voxel_pos = ivec3(6, 3, 9);
    voxel_pos = GameVoxelToTexelFetch(ivec3(9, 3, 10));
    //voxel_pos = MagicaToTexelFetch(ivec3(9, 10, 3));

    uvec4 voxel_index = texelFetch(voxel_indices,         voxel_pos,          0);
    if (voxel_index.r == 0)
        discard;
    vec4 voxel_color = texelFetch(voxel_color_palette,  int(voxel_index.r),  0);
    color.xyz = voxel_color.xyz;
    color.a = 1.0;
#endif
}
