#pragma once

#include "core/render_types.h"

// Expands a set of 3D polylines into a screen-space quad mesh
//  - viewProj    : combined view-projection matrix (column-major)
//  - viewportW/H : framebuffer dimensions in pixels
//  - style       : line width, join type, miter limit
EdgeQuadMeshCpu buildEdgeQuadMesh(
    const std::vector<EdgePolyline3D>& polylines,
    const Mat4& viewProj,
    int viewportW, int viewportH,
    const EdgeStyle& style,
    float contentScaleX = 1.0f,
    float contentScaleY = 1.0f);