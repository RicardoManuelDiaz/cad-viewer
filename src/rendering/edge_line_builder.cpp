#include "edge_line_builder.h"

EdgeLineMeshCpu buildEdgeLineMesh(const std::vector<EdgePolyline3D>& polylines)
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
            out.vertices.push_back(EdgeLineVertex{
                p.x, p.y, p.z,
                static_cast<std::uint32_t>(polyline.ownerEdgeId)
                });
        }

        out.ranges.push_back(range);
    }

    return out;
}