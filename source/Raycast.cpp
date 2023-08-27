#include "Raycast.h"
#include "Misc.h"
#include "Vox.h"

//http://www.cs.yorku.ca/~amana/research/grid.pdf
RaycastResult LineCast(const Ray& ray, VoxData voxels, float length)
{
    assert(length >= 0.0f);
    assert(length <= 10000.0f);
    RaycastResult result = {};

    Vec3 p = ray.origin;
    Vec3 step = {};
    step.x = ray.direction.x >= 0 ? 1.0f : -1.0f;
    step.y = ray.direction.y >= 0 ? 1.0f : -1.0f;
    step.z = ray.direction.z >= 0 ? 1.0f : -1.0f;
    Vec3 pClose = ray.origin + (step / 2);
    Vec3 tMax = Abs((pClose - ray.origin) / ray.direction);
    Vec3 tDelta = Abs(1.0f / ray.direction);
    
    u32 index = 0;
    while (!index) 
    {
        if (Distance(p, ray.origin) > length)
            break;

        result.normal = {};
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            p.x += step.x;
            tMax.x += tDelta.x;
            result.normal.x = float(-1.0) * step.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            result.normal.y = float(-1.0) * step.y;
        }
        else 
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            result.normal.z = float(-1.0) * step.z;
        }

        Vec3Int voxel_p = Vec3ToVec3Int(p);
        result.p = p;  //Vec3IntToVec3(voxel_p);
        assert(voxels.color_indices.size() == 1);
        if (voxel_p.x > voxels.size.x || voxel_p.y > voxels.size.y || voxel_p.z > voxels.size.z)
            break;
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            continue;
        index = voxels.color_indices[0].e[voxel_p.x][voxel_p.y][voxel_p.z];
        //g_chunks->GetBlock(blockType, result.p);
    }
    result.distance_mag = Distance(ray.origin, p);
    result.success      = index;
    return result;
}

RaycastResult RayVsAABB(const Ray& ray, const AABB& box)
{
    RaycastResult r = {};
    float tmin = 0;
    float tmax = FLT_MAX;

    for (i32 slab = 0; slab < 3; ++slab)
    {
        if (::fabs(ray.direction.e[slab]) < FLT_EPSILON)
        {
            // Ray is parallel to the slab
            if (ray.origin.e[slab] < box.min.e[slab] || ray.origin.e[slab] > box.max.e[slab])
                return r;
        }
        else
        {
            float ood = 1.0f / ray.direction.e[slab];
            float t1 = (box.min.e[slab] - ray.origin.e[slab]) * ood;
            float t2 = (box.max.e[slab] - ray.origin.e[slab]) * ood;
            if (t1 > t2)
            {
                std::swap(t1, t2);
            }
            tmin = Max(tmin, t1);
            tmax = Min(tmax, t2);
            if (tmin > tmax)
                return r;
        }
    }
    r.p = ray.origin + ray.direction * tmin;
    r.distance_mag = tmin;

    const static Vec3 normals[] = {
        {  1,  0,  0 },
        { -1,  0,  0 },
        {  0,  1,  0 },
        {  0, -1,  0 },
        {  0,  0,  1 },
        {  0,  0, -1 },
    };

    Vec3  v[6];
    v[0] = { box.max.x, r.p.y,      r.p.z };
    v[1] = { box.min.x, r.p.y,      r.p.z };
    v[2] = { r.p.x,     box.max.y,  r.p.z };
    v[3] = { r.p.x,     box.min.y,  r.p.z };
    v[4] = { r.p.x,     r.p.y,      box.max.z };
    v[5] = { r.p.x,     r.p.y,      box.min.z };

    float currentDistance = 0;
    float ClosestDistance = FLT_MAX;
    i32 closestFace = 0;
    for (i32 i = 0; i < arrsize(v); i++)
    {
        currentDistance = Distance(r.p, v[i]);
        if (currentDistance < ClosestDistance)
        {
            ClosestDistance = currentDistance;
            closestFace = i;
        }
    }
    r.normal = normals[closestFace];

    //one of these will be zero maybe I just add instead of branching?
    //if (r.distance)
    //    actualMove = NormalizeZero(r.p - ray.origin) * r.distance;
    //else
    //    actualMove = v[closestFace] - r.p;

    r.success = true;
    return r;
}

Ray MouseToRaycast(const Vec2Int& pixel_pos, const Vec2Int& screen_size, const Vec3& camera_pos, Mat4* projection_from_view, Mat4* view_from_world)
{
    //To Normalized Device Coordinates
    //The top left of the monitor is the origin: { -1, -1 }
    //The bot right of the monitor is { 1, 1 }
    float x = (2.0f * pixel_pos.x) / screen_size.x - 1.0f;
    float y = 1.0f - (2.0f * (screen_size.y - pixel_pos.y)) / screen_size.y;;
    float z = 1.0f;
    Vec3 ray_nds = { x, y, z };
    Vec4 ray_clip = { ray_nds.x, ray_nds.y, +1.0, 1.0 }; // z may need to be either positive or negative

    //From Clip to View
    Mat4 view_from_projection;
    gb_mat4_inverse(&view_from_projection, projection_from_view);
    Vec4 ray_view = view_from_projection * ray_clip;
    ray_view = { ray_view.x, ray_view.y, -1.0, 0.0 };

    //From View to World
    Mat4 world_from_view;
    gb_mat4_inverse(&world_from_view, view_from_world);
    Vec3 ray_world = (world_from_view * ray_view).xyz;

    //Normalize
    Vec3 ray_world_n;
    gb_vec3_norm0(&ray_world_n, ray_world);
    Ray ray = {
        .origin = camera_pos,
        .direction = ray_world_n,
    };
    return ray;
}
