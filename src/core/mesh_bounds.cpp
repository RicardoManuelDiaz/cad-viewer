#include "mesh_bounds.h"

#include <cmath>
#include <stdexcept>

namespace
{
    Vec3 minVec3(const Vec3& a, const Vec3& b)
    {
        return {
            (a.x < b.x) ? a.x : b.x,
            (a.y < b.y) ? a.y : b.y,
            (a.z < b.z) ? a.z : b.z
        };
    }

    Vec3 maxVec3(const Vec3& a, const Vec3& b)
    {
        return {
            (a.x > b.x) ? a.x : b.x,
            (a.y > b.y) ? a.y : b.y,
            (a.z > b.z) ? a.z : b.z
        };
    }
}

Bounds3 computeMeshBounds(const RenderMesh& mesh)
{
    if (mesh.vertices.empty())
    {
        throw std::runtime_error("Cannot compute bounds of an empty mesh.");
    }

    Bounds3 bounds{};
    bounds.min = { mesh.vertices[0].px, mesh.vertices[0].py, mesh.vertices[0].pz };
    bounds.max = bounds.min;

    for (const Vertex& v : mesh.vertices)
    {
        const Vec3 p{ v.px, v.py, v.pz };
        bounds.min = minVec3(bounds.min, p);
        bounds.max = maxVec3(bounds.max, p);
    }

    return bounds;
}

Vec3 computeBoundsCenter(const Bounds3& bounds)
{
    return {
        0.5f * (bounds.min.x + bounds.max.x),
        0.5f * (bounds.min.y + bounds.max.y),
        0.5f * (bounds.min.z + bounds.max.z)
    };
}

float computeBoundsRadius(const Bounds3& bounds, const Vec3& center)
{
    const Vec3 dx{
        bounds.max.x - center.x,
        bounds.max.y - center.y,
        bounds.max.z - center.z
    };

    return std::sqrt(dx.x * dx.x + dx.y * dx.y + dx.z * dx.z);
}