#include "Raycast.h"
#include "Misc.h"
#include "Vox.h"

//Vec3Int GetVoxelPosFromRayPos(Vec3& p, const Vec3& ray_direction)
//{
//#if 1
//    const float epsilon = 0.00001f;
//    {
//        float rounded = roundf(p.x);
//        if (p.x - rounded <= epsilon)
//            p.x = rounded;
//    }
//    {
//        float rounded = roundf(p.y);
//        if (p.y - rounded <= epsilon)
//            p.y = rounded;
//    }
//    {
//        float rounded = roundf(p.z);
//        if (p.z - rounded <= epsilon)
//            p.z = rounded;
//    }
//#endif
//    Vec3Int voxel_p = Vec3ToVec3Int(Floor(p));
//    voxel_p.x = (ray_direction.x < 0) ? voxel_p.x - 1 : voxel_p.x;
//    voxel_p.y = (ray_direction.y < 0) ? voxel_p.y - 1 : voxel_p.y;
//    voxel_p.z = (ray_direction.z < 0) ? voxel_p.z - 1 : voxel_p.z;
//    return voxel_p;
//}

//RaycastResult LineCast(const Ray& ray, VoxData voxels, float length)
//{
//    assert(length >= 0.0f);
//    RaycastResult result = {};
//
//    Vec3 p = ray.origin;
//    Vec3 step = {};
//    step.x = ray.direction.x >= 0 ? 1.0f : -1.0f;
//    step.y = ray.direction.y >= 0 ? 1.0f : -1.0f;
//    step.z = ray.direction.z >= 0 ? 1.0f : -1.0f;
//
//    //Vec3 r = 1 - Fract(ray.origin);
//    Vec3 fract = Abs(ray.origin - Floor(ray.origin));
//    Vec3 r;
//#if 0
//    r.x = (ray.direction.x < 0) ? fract.x : 1 - fract.x;
//    r.y = (ray.direction.y < 0) ? fract.y : 1 - fract.y;
//    r.z = (ray.direction.z < 0) ? fract.z : 1 - fract.z;
//#else
//    r = fract;
//#endif
//    //Vec3 s = Abs(ray.direction);
//    Vec3 s = ray.direction;
//    Vec3 t;
//    
//    u32 index = 0;
//    while (!index)
//    {
//        t = Abs(r / ray.direction);
//
//        if (Distance(p, ray.origin) > length)
//            break;
//
//        if (t.x < t.y && t.x < t.z)
//        {
//            //x
//            r.y = r.y - s.y * r.x;
//            r.z = r.z - s.z * r.x;
//            p += s * r.x;
//            r.x = 1;
//            result.normal.x = -step.x;
//#if 0
//            float rounded = roundf(p.x);
//            if (p.x - rounded <= 0.0001f)
//                p.x = rounded;
//#endif
//        }
//        else if (t.y < t.x && t.y < t.z)
//        {
//            //y
//            r.x = r.x - s.x * r.y;
//            r.z = r.z - s.z * r.y;
//            p += s * r.y;
//            r.y = 1;
//            result.normal.y = -step.y;
//#if 0
//            float rounded = roundf(p.y);
//            if (p.y - rounded <= 0.0001f)
//                p.y = rounded;
//#endif
//        }
//        else
//        {
//            //z
//            r.x = r.x - s.x * r.z;
//            r.y = r.y - s.y * r.z;
//            p += s * r.z;
//            r.z = 1;
//            result.normal.z = -step.z;
//#if 0
//            float rounded = roundf(p.z);
//            if (p.z - rounded <= 0.0001f)
//                p.z = rounded;
//#endif
//        }
//        //Vec3 epsilon_check = p - Abs(p);
//        //Vec3Int voxel_p = Vec3ToVec3Int(Floor(p));
//        Vec3 new_p = p;
//        Vec3Int voxel_p = GetVoxelPosFromRayPos(new_p, ray.direction);
//        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
//            continue;
//        if (voxel_p.x >= voxels.size.x || voxel_p.y >= voxels.size.y || voxel_p.z >= voxels.size.z)
//            continue;
//
//        index = voxels.color_indices[0].e[voxel_p.x][voxel_p.y][voxel_p.z];
//        if (index)
//            p = new_p;
//    }
//    result.p = p;
//    result.success = index;
//
//
//    return result;
//}

RaycastResult Linecast(const Ray& ray, VoxData voxels, float length)
{
    assert(length >= 0.0f);
    RaycastResult result = {};

    Vec3 p = ray.origin;
    Vec3 step = {};
    step.x = ray.direction.x >= 0 ? 1.0f : -1.0f;
    step.y = ray.direction.y >= 0 ? 1.0f : -1.0f;
    step.z = ray.direction.z >= 0 ? 1.0f : -1.0f;
    const Vec3 pClose = Floor((Round(ray.origin + (step / 2))));
    Vec3 tMax = Abs((pClose - ray.origin) / ray.direction);
    const Vec3 tDelta = Abs(1.0f / ray.direction);
    Vec3Int voxel_p;

    
    //u32 index = 0;
    while (!result.success) 
    {
        if (Distance(p, ray.origin) > length)
            return result;

        result.normal = {};
        if (tMax.x < tMax.y && tMax.x < tMax.z)
        {
            p.x += step.x;
            tMax.x += tDelta.x;
            result.normal.x = -step.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            result.normal.y = -step.y;
        }
        else 
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            result.normal.z = -step.z;
        }

        voxel_p = ToVec3Int(Floor(p));
        assert(voxels.color_indices.size() == 1);
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            continue;
        if (voxel_p.x >= voxels.size.x || voxel_p.y >= voxels.size.y || voxel_p.z >= voxels.size.z)
            continue;
        
        result.success = voxels.color_indices[0].e[voxel_p.x][voxel_p.y][voxel_p.z];

    }
    
    u32 comp = (result.normal.x ? 0 : (result.normal.y ? 1 : 2));
    float voxel = ray.direction.e[comp] < 0 ? float(voxel_p.e[comp]) + 1.0f : float(voxel_p.e[comp]);
    float t = (voxel - ray.origin.e[comp]) / ray.direction.e[comp];
    result.p = ray.origin + ray.direction * t;

    result.distance_mag = Distance(ray.origin, p);
    //result.success      = index;
    return result;
}

//http://www.cs.yorku.ca/~amana/research/grid.pdf
RaycastResult VoxelLinecast(const Ray& ray, VoxData voxels, float length)
{
    assert(length >= 0.0f);
    RaycastResult result = {};

    Vec3 p = ray.origin;
    Vec3 step = {};
    step.x = ray.direction.x >= 0 ? 1.0f : -1.0f;
    step.y = ray.direction.y >= 0 ? 1.0f : -1.0f;
    step.z = ray.direction.z >= 0 ? 1.0f : -1.0f;
    const Vec3 pClose = Floor((Round(ray.origin + (step / 2))));
    Vec3 tMax = Abs((pClose - ray.origin) / ray.direction);
    const Vec3 tDelta = Abs(1.0f / ray.direction);

    
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
            result.normal.x = -step.x;
        }
        else if (tMax.y < tMax.x && tMax.y < tMax.z)
        {
            p.y += step.y;
            tMax.y += tDelta.y;
            result.normal.y = -step.y;
        }
        else 
        {
            p.z += step.z;
            tMax.z += tDelta.z;
            result.normal.z = -step.z;
        }

        Vec3Int voxel_p = ToVec3Int(Floor(p));
        assert(voxels.color_indices.size() == 1);
        if (voxel_p.x < 0 || voxel_p.y < 0 || voxel_p.z < 0)
            continue;
        if (voxel_p.x >= voxels.size.x || voxel_p.y >= voxels.size.y || voxel_p.z >= voxels.size.z)
            continue;
        
        index = voxels.color_indices[0].e[voxel_p.x][voxel_p.y][voxel_p.z];
        volatile u32 test = 0;
        result.p = ToVec3(voxel_p);
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

Ray MouseToRaycast(const Vec2Int& pixel_pos, const Vec2Int& screen_size, const Vec3& camera_pos, Mat4* view_from_projection, Mat4* world_from_view)
{
    VALIDATE_V(view_from_projection,    {});
    VALIDATE_V(world_from_view,         {});
    //To Normalized Device Coordinates
    //The top left of the monitor is the origin: { -1, -1 }
    //The bot right of the monitor is { 1, 1 }
    float x = (2.0f * pixel_pos.x) / screen_size.x - 1.0f;
    float y = 1.0f - (2.0f * (screen_size.y - pixel_pos.y)) / screen_size.y;;
    float z = 1.0f;
    Vec3 ray_nds = { x, y, z };
    Vec4 ray_clip = { ray_nds.x, ray_nds.y, +1.0, 1.0 }; // z may need to be either positive or negative

    //From Clip to View
    Vec4 ray_view = *view_from_projection * ray_clip;
    ray_view = { ray_view.x, ray_view.y, -1.0, 0.0 };

    //From View to World
    Vec3 ray_world = (*world_from_view * ray_view).xyz;

    //Normalize
    Vec3 ray_world_n;
    gb_vec3_norm0(&ray_world_n, ray_world);
    Ray ray = {
        .origin = camera_pos,
        .direction = ray_world_n,
    };
    return ray;
}
