#pragma once

#include "render_types.h"

EdgeQuadMeshCpu buildEdgeQuadMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    const Viewport& viewport,
    const EdgeStyle& style);