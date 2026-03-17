#include "edge_line_builder.h"

namespace
{
    static EdgeLineVertex makeVertex(const Mat4& m, const Vec3& p, std::uint32_t ownerEdgeId)
    {
        // Column-major matrix, matching your math3d.cpp
        const float x = m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12];
        const float y = m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13];
        const float z = m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14];
        const float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];

        return EdgeLineVertex{ x, y, z, w, ownerEdgeId };
    }
}

EdgeLineMeshCpu buildEdgeLineMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj)
{
    EdgeLineMeshCpu out;

    for (const EdgePolyline3D& polyline : polylines)
    {
        if (polyline.points.size() < 2)
            continue;

        EdgeLineRange range;
        range.first = static_cast<int>(out.vertices.size());
        range.count = static_cast<int>(polyline.points.size());
        range.closed = polyline.closed;

        for (const Vec3& p : polyline.points)
        {
            out.vertices.push_back(
                makeVertex(viewProj, p, static_cast<std::uint32_t>(polyline.ownerEdgeId)));
        }

        out.ranges.push_back(range);
    }

    return out;
}