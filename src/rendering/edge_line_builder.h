#pragma once

#include "render_types.h"

#include <cstdint>
#include <vector>

struct EdgeLineVertex
{
    float clipX, clipY, clipZ, clipW;
    std::uint32_t ownerEdgeId;
};

struct EdgeLineRange
{
    int first = 0;
    int count = 0;
    bool closed = false;
};

struct EdgeLineMeshCpu
{
    std::vector<EdgeLineVertex> vertices;
    std::vector<EdgeLineRange> ranges;

    void clear()
    {
        vertices.clear();
        ranges.clear();
    }

    bool empty() const
    {
        return vertices.empty() || ranges.empty();
    }
};

EdgeLineMeshCpu buildEdgeLineMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj);