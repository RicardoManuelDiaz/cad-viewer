#include "mesh_edges.h"

#include <algorithm>

namespace
{
    static float distSq(const Vec3& a, const Vec3& b)
    {
        const Vec3 d        = a - b;
        return dot(d, d);
    }
}

void cleanupEdgePolyline(EdgePolyline3D& polyline, float eps)
{
    if (polyline.points.size() < 2)
        return;

    const float epsSq       = eps * eps;

    std::vector<Vec3> clean;
    clean.reserve(polyline.points.size());
    clean.push_back(polyline.points.front());

    for (std::size_t i = 1; i < polyline.points.size(); ++i)
    {
        if (distSq(polyline.points[i], clean.back()) > epsSq)
            clean.push_back(polyline.points[i]);
    }

    polyline.points         = std::move(clean);
}

void cleanupEdgePolylines(std::vector<EdgePolyline3D>& polylines, float eps)
{
    for (EdgePolyline3D& p : polylines)
        cleanupEdgePolyline(p, eps);

    polylines.erase(
        std::remove_if(polylines.begin(), polylines.end(),
            [](const EdgePolyline3D& p)
            {
                return p.points.size() < 2;
            }),
        polylines.end());
}