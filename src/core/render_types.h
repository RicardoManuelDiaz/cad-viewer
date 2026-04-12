#pragma once

#include "math3d.h"

#include <cstdint>
#include <vector>



struct Vertex
{
    float px, py, pz;
    float nx, ny, nz;
};

struct RenderMesh
{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct EdgeVertex
{
    float px, py, pz;
};

struct EdgeMesh
{
    std::vector<EdgeVertex> vertices;
};

// ----------------------------------------

using FaceId    = int;
using EdgeId    = int;
using VertexId  = int;

constexpr FaceId   kInvalidFaceId   = -1;
constexpr EdgeId   kInvalidEdgeId   = -1;
constexpr VertexId kInvalidVertexId = -1;

struct Viewport
{
    int width   = 0;
    int height  = 0;
};

enum class LineJoin
{
    Bevel,
    Miter
};

struct EdgeStyle
{
    float widthPx       = 2.0f;
    float miterLimit    = 4.0f;
    LineJoin join       = LineJoin::Miter;
};

// One topological edge -> one ordered 3D polyline
struct EdgePolyline3D
{
    EdgeId ownerEdgeId = kInvalidEdgeId;
    bool closed = false;
    std::vector<Vec3> points;
};

// CPU-side quad mesh already expressed in clip-space
struct EdgeQuadVertex
{
    float clipX, clipY, clipZ, clipW;
    float coverage;
    std::uint32_t ownerEdgeId;
};

struct EdgeQuadMeshCpu
{
    std::vector<EdgeQuadVertex> vertices;
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
