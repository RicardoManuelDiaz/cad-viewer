#pragma once
#include "render_types.h"

#include "shape_topology.h"

#include <vector>

std::vector<EdgePolyline3D> extractOcctEdgePolylines(const ShapeTopology& topology);