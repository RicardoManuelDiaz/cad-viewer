#include "shape_topology.h"

#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>

bool ShapeTopology::build(const TopoDS_Shape& shape)
{
    clear();

    if (shape.IsNull())
        return false;

    TopTools_IndexedMapOfShape faceMap;
    TopTools_IndexedMapOfShape edgeMap;
    TopTools_IndexedMapOfShape vertexMap;

    TopExp::MapShapes(shape, TopAbs_FACE, faceMap);
    TopExp::MapShapes(shape, TopAbs_EDGE, edgeMap);
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertexMap);

    m_faces.resize(faceMap.Extent());
    m_edges.resize(edgeMap.Extent());
    m_vertices.resize(vertexMap.Extent());

    for (int i = 1; i <= faceMap.Extent(); ++i)
    {
        FaceInfo& f = m_faces[i - 1];
        f.id = i - 1;
        f.face = TopoDS::Face(faceMap(i));

        for (TopExp_Explorer ex(f.face, TopAbs_EDGE); ex.More(); ex.Next())
        {
            const int edgeIndex = edgeMap.FindIndex(ex.Current());
            if (edgeIndex > 0)
                f.boundaryEdgeIds.push_back(edgeIndex - 1);
        }
    }

    TopTools_IndexedDataMapOfShapeListOfShape edgeToFaces;
    TopTools_IndexedDataMapOfShapeListOfShape vertexToEdges;

    TopExp::MapShapesAndAncestors(shape, TopAbs_EDGE, TopAbs_FACE, edgeToFaces);
    TopExp::MapShapesAndAncestors(shape, TopAbs_VERTEX, TopAbs_EDGE, vertexToEdges);

    for (int i = 1; i <= edgeMap.Extent(); ++i)
    {
        EdgeInfo& e = m_edges[i - 1];
        e.id = i - 1;
        e.edge = TopoDS::Edge(edgeMap(i));

        TopoDS_Vertex v0, v1;
        TopExp::Vertices(e.edge, v0, v1, true);

        if (!v0.IsNull())
            e.startVertexId = vertexMap.FindIndex(v0) - 1;
        if (!v1.IsNull())
            e.endVertexId = vertexMap.FindIndex(v1) - 1;

        if (edgeToFaces.Contains(e.edge))
        {
            const TopTools_ListOfShape& faces = edgeToFaces.FindFromKey(e.edge);
            for (TopTools_ListIteratorOfListOfShape it(faces); it.More(); it.Next())
            {
                const int faceIndex = faceMap.FindIndex(it.Value());
                if (faceIndex > 0)
                    e.adjacentFaceIds.push_back(faceIndex - 1);
            }
        }
    }

    for (int i = 1; i <= vertexMap.Extent(); ++i)
    {
        VertexInfo& v = m_vertices[i - 1];
        v.id = i - 1;
        v.vertex = TopoDS::Vertex(vertexMap(i));

        if (vertexToEdges.Contains(v.vertex))
        {
            const TopTools_ListOfShape& edges = vertexToEdges.FindFromKey(v.vertex);
            for (TopTools_ListIteratorOfListOfShape it(edges); it.More(); it.Next())
            {
                const int edgeIndex = edgeMap.FindIndex(it.Value());
                if (edgeIndex > 0)
                    v.incidentEdgeIds.push_back(edgeIndex - 1);
            }
        }
    }

    return true;
}

void ShapeTopology::clear()
{
    m_faces.clear();
    m_edges.clear();
    m_vertices.clear();
}

const FaceInfo* ShapeTopology::findFace(FaceId id) const
{
    if (id < 0 || id >= static_cast<FaceId>(m_faces.size()))
        return nullptr;
    return &m_faces[id];
}

const EdgeInfo* ShapeTopology::findEdge(EdgeId id) const
{
    if (id < 0 || id >= static_cast<EdgeId>(m_edges.size()))
        return nullptr;
    return &m_edges[id];
}

const VertexInfo* ShapeTopology::findVertex(VertexId id) const
{
    if (id < 0 || id >= static_cast<VertexId>(m_vertices.size()))
        return nullptr;
    return &m_vertices[id];
}