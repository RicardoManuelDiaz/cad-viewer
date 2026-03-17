#pragma once

#include "math3d.h"
#include "render_types.h"

struct Bounds3
{
    Vec3 min;
    Vec3 max;
};

Bounds3 computeMeshBounds(const RenderMesh& mesh);
Vec3 computeBoundsCenter(const Bounds3& bounds);
float computeBoundsRadius(const Bounds3& bounds, const Vec3& center);