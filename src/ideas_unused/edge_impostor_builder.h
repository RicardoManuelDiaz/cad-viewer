#pragma once

#include "render_types.h"

#include <cstdint>
#include <vector>

struct EdgeImpostorVertex
{
    float clipX, clipY, clipZ, clipW;

    float lineAxPx, lineAyPx;
    float lineBxPx, lineByPx;

    float startNxPx, startNyPx;
    float endNxPx, endNyPx;

    float halfWidthPx;

    std::uint32_t ownerEdgeId;
};

struct EdgeImpostorMeshCpu
{
    std::vector<EdgeImpostorVertex> vertices;
    std::vector<std::uint32_t> indices;

    void clear()
    {
        vertices.clear();
        indices.clear();
    }

    bool empty() const
    {
        return vertices.empty() || indices.empty();
    }
};

EdgeImpostorMeshCpu buildEdgeImpostorMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    const Viewport& viewport,
    float widthPx,
    float miterLimitPx = 4.0f,
    float depthBiasNdc = 1.0e-4f);