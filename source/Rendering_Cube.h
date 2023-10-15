#pragma once
#include "Math.h"

inline void AddCubeToRender(Vec3 p, Color color, float scale) {};
inline void AddCubeToRender(Vec3 p, Color color, Vec3  scale) {};
inline void RenderTransparentCubes(const Mat4& projection_from_view, const Mat4& view_from_model) {};
inline void RenderOpaqueCubes      (const Mat4& projection_from_view, const Mat4& view_from_model) {};
