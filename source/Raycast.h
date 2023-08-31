#pragma once
#include "Math.h"
#include "Vox.h"

struct RaycastResult {
    bool    success         = {};
    Vec3    p               = {};
    float   distance_mag    = {};
    Vec3    normal          = {};
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

RaycastResult Linecast(const Ray& ray, VoxData voxels, float length);
RaycastResult VoxelLinecast(const Ray& ray, VoxData voxels, float length);
[[nodiscard]] RaycastResult RayVsAABB(const Ray& ray, const AABB& box);
[[nodiscard]] Ray MouseToRaycast(const Vec2Int& pixel_pos, const Vec2Int& screen_size, const Vec3& camera_pos, Mat4* perspective, Mat4* view);
