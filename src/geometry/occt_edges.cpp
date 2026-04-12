#include "occt_edges.h"

#include <BRep_Tool.hxx>
#include <BRepTools.hxx>

#include <Poly_PolygonOnTriangulation.hxx>
#include <Poly_Triangulation.hxx>

#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Vertex.hxx>

#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

namespace
{
    static Vec3 toVec3(const gp_Pnt& p)
    {
        return Vec3{
            static_cast<float>(p.X()),
            static_cast<float>(p.Y()),
            static_cast<float>(p.Z())
        };
    }

    static float distSq(const Vec3& a, const Vec3& b)
    {
        const Vec3 d = a - b;
        return dot(d, d);
    }

    static bool extractPolylineOnFace(
        const EdgeInfo& edgeInfo,
        const FaceInfo& faceInfo,
        EdgePolyline3D& out)
    {
        TopLoc_Location faceLoc;
        Handle(Poly_Triangulation) tri      = BRep_Tool::Triangulation(faceInfo.face, faceLoc);
        if (tri.IsNull())
            return false;

        TopLoc_Location edgeLoc;
        Handle(Poly_PolygonOnTriangulation) poly =
            BRep_Tool::PolygonOnTriangulation(edgeInfo.edge, tri, edgeLoc);

        if (poly.IsNull())
            return false;

        const gp_Trsf faceTrsf              = faceLoc.Transformation();
        const TColStd_Array1OfInteger& idx  = poly->Nodes();

        if (idx.Length() < 2)
            return false;

        out.ownerEdgeId = edgeInfo.id;
        out.closed = false;
        out.points.clear();
        out.points.reserve(idx.Length());

        for (Standard_Integer i = idx.Lower(); i <= idx.Upper(); ++i)
        {
            const gp_Pnt p = tri->Node(idx(i)).Transformed(faceTrsf);
            out.points.push_back(toVec3(p));
        }

        if (out.points.size() < 2)
            return false;

        // Detect closed loop by geometry
        constexpr float closeTolSq = 1.0e-6f;
        if (out.points.size() >= 3 &&
            distSq(out.points.front(), out.points.back()) <= closeTolSq)
        {
            out.closed = true;
            out.points.pop_back(); // remove duplicated closing point
        }
        else
        {
            // Detect closed topological edge (same start/end vertex)
            TopoDS_Vertex v0, v1;
            TopExp::Vertices(edgeInfo.edge, v0, v1, true);

            if (!v0.IsNull() && !v1.IsNull() && v0.IsSame(v1))
            {
                out.closed = true;
            }
        }

        return out.points.size() >= 2;
    }
}

std::vector<EdgePolyline3D> extractOcctEdgePolylines(const ShapeTopology& topology)
{
    std::vector<EdgePolyline3D> out;
    out.reserve(topology.edges().size());

    for (const EdgeInfo& edgeInfo : topology.edges())
    {
        EdgePolyline3D polyline;
        bool found = false;

        for (FaceId faceId : edgeInfo.adjacentFaceIds)
        {
            const FaceInfo* faceInfo = topology.findFace(faceId);
            if (!faceInfo)
                continue;

            // Skip this face if the edge is a seam on it - will appear twice with opposite orientations and polygon data is ambiguous
            //  - try next adjacent face
            if (BRepTools::IsReallyClosed(edgeInfo.edge, faceInfo->face))
                continue;

            if (extractPolylineOnFace(edgeInfo, *faceInfo, polyline))
            {
                found = true;
                break;
            }
        }

        if (found && polyline.points.size() >= 2)
            out.push_back(std::move(polyline));
    }

    return out;
}