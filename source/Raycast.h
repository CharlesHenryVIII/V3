#pragma once
#include "Math.h"
#include "Vox.h"

struct RaycastResult {
    u32     success         = {};
    Vec3    p               = {};
    float   distance_mag    = {};
    Vec3    normal          = {};
};

struct Ray {
    Vec3 origin;
    Vec3 direction;
};

Vec3 ReflectRay(const Vec3& dir, const Vec3& normal);
RaycastResult Linecast(const Ray& ray, VoxData voxels, float length);
RaycastResult VoxelLinecast(const Ray& ray, VoxData voxels, float length);
[[nodiscard]] RaycastResult RayVsAABB(const Ray& ray, const AABB& box);
[[nodiscard]] Ray MouseToRaycast(const Vec2I& pixel_pos, const Vec2I& screen_size, const Vec3& camera_pos, const Mat4& perspective, const Mat4& view);
