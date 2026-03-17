#pragma once

#include "render_types.h"

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <vector>

struct FaceInfo
{
    FaceId id = kInvalidFaceId;
    TopoDS_Face face;
    std::vector<EdgeId> boundaryEdgeIds;
};

struct EdgeInfo
{
    EdgeId id = kInvalidEdgeId;
    TopoDS_Edge edge;
    std::vector<FaceId> adjacentFaceIds;
    VertexId startVertexId = kInvalidVertexId;
    VertexId endVertexId = kInvalidVertexId;
};

struct VertexInfo
{
    VertexId id = kInvalidVertexId;
    TopoDS_Vertex vertex;
    std::vector<EdgeId> incidentEdgeIds;
};

class ShapeTopology
{
public:
    bool build(const TopoDS_Shape& shape);
    void clear();

    const std::vector<FaceInfo>& faces() const { return m_faces; }
    const std::vector<EdgeInfo>& edges() const { return m_edges; }
    const std::vector<VertexInfo>& vertices() const { return m_vertices; }

    const FaceInfo* findFace(FaceId id) const;
    const EdgeInfo* findEdge(EdgeId id) const;
    const VertexInfo* findVertex(VertexId id) const;

private:
    std::vector<FaceInfo> m_faces;
    std::vector<EdgeInfo> m_edges;
    std::vector<VertexInfo> m_vertices;
};