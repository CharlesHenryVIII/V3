#pragma once
#include "Math.h"

void AddCubeToRender(Vec3 p, Color color, float scale);
void AddCubeToRender(Vec3 p, Color color, Vec3  scale);
void RenderTransparentCubes (const Mat4& projection_from_view, const Mat4& view_from_model);
void RenderOpaqueCubes      (const Mat4& projection_from_view, const Mat4& view_from_model);
