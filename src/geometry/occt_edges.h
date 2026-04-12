#pragma once
#include "core/render_types.h"

#include "geometry/shape_topology.h"

#include <vector>

std::vector<EdgePolyline3D> extractOcctEdgePolylines(const ShapeTopology& topology);