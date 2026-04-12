#pragma once

#include "core/render_types.h"

#include <TopoDS_Shape.hxx>

RenderMesh prepareForOpenGL(const TopoDS_Shape& shape, double linearDeflection = 0.10, double angularDeflection = 0.50);