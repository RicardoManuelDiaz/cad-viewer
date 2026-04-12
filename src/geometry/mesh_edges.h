#pragma once

//#include "render_types.h"

//EdgeMesh buildTriangleEdgeMesh(const RenderMesh& mesh);

#include "core/render_types.h"

#include <vector>

void cleanupEdgePolyline(EdgePolyline3D& polyline, float eps = 1e-6f);
void cleanupEdgePolylines(std::vector<EdgePolyline3D>& polylines, float eps = 1e-6f);