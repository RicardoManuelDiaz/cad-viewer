//#include "render_types.h"
//#include "shape_gen.h"
//
//#include <BRepMesh_IncrementalMesh.hxx>
//#include <BRep_Tool.hxx>
//
//#include <Poly_Triangulation.hxx>
//
//#include <TopAbs_ShapeEnum.hxx>
//#include <TopAbs_Orientation.hxx>
//#include <TopExp_Explorer.hxx>
//#include <TopLoc_Location.hxx>
//#include <TopoDS.hxx>
//#include <TopoDS_Face.hxx>
//#include <TopoDS_Shape.hxx>
//
//#include <gp_Pnt.hxx>
//#include <gp_Trsf.hxx>
//#include <gp_Vec.hxx>
//
//#include <cstdint>
//#include <stdexcept>
//#include <vector>
//
//static Vertex makeVertex(const gp_Pnt& p, const gp_Vec& n)
//{
//    return Vertex{
//        static_cast<float>(p.X()),
//        static_cast<float>(p.Y()),
//        static_cast<float>(p.Z()),
//        static_cast<float>(n.X()),
//        static_cast<float>(n.Y()),
//        static_cast<float>(n.Z())
//    };
//}
//
//RenderMesh prepareForOpenGL(const TopoDS_Shape& shape,
//    const double linearDeflection,
//    const double angularDeflection)
//{
//    BRepMesh_IncrementalMesh mesher(
//        shape,
//        linearDeflection,
//        Standard_False,
//        angularDeflection,
//        Standard_True
//    );
//
//    RenderMesh out;
//
//    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
//    {
//        const TopoDS_Face face          = TopoDS::Face(exp.Current());
//
//        TopLoc_Location faceLoc;
//        Handle(Poly_Triangulation) tri  = BRep_Tool::Triangulation(face, faceLoc);
//        if (tri.IsNull())
//        {
//            continue;
//        }
//
//        const gp_Trsf trsf              = faceLoc.Transformation();
//
//        const Standard_Integer nbNodes  = tri->NbNodes();
//        const Standard_Integer nbTris   = tri->NbTriangles();
//
//        if (nbNodes <= 0 || nbTris <= 0)
//        {
//            continue;
//        }
//
//        // Transform triangulation nodes once
//        std::vector<gp_Pnt> worldNodes(static_cast<std::size_t>(nbNodes) + 1);
//        for (Standard_Integer i = 1; i <= nbNodes; ++i)
//        {
//            worldNodes[static_cast<std::size_t>(i)] =
//                tri->Node(i).Transformed(trsf);
//        }
//
//        // Accumulate one normal sum per node
//        std::vector<gp_Vec> nodeNormals(static_cast<std::size_t>(nbNodes) + 1, gp_Vec(0.0, 0.0, 0.0));
//
//        for (Standard_Integer i = 1; i <= nbTris; ++i)
//        {
//            Standard_Integer n1 = 0, n2 = 0, n3 = 0;
//            tri->Triangle(i).Get(n1, n2, n3);
//
//            if (face.Orientation() == TopAbs_REVERSED)
//            {
//                std::swap(n2, n3);
//            }
//
//            const gp_Pnt& p1            = worldNodes[static_cast<std::size_t>(n1)];
//            const gp_Pnt& p2            = worldNodes[static_cast<std::size_t>(n2)];
//            const gp_Pnt& p3            = worldNodes[static_cast<std::size_t>(n3)];
//
//            const gp_Vec e1(p1, p2);
//            const gp_Vec e2(p1, p3);
//            gp_Vec triNormal = e1.Crossed(e2);
//
//            if (triNormal.SquareMagnitude() < 1.0e-24)
//            {
//                continue;
//            }
//
//            // Area-weighted accumulation
//            nodeNormals[static_cast<std::size_t>(n1)] += triNormal;
//            nodeNormals[static_cast<std::size_t>(n2)] += triNormal;
//            nodeNormals[static_cast<std::size_t>(n3)] += triNormal;
//        }
//
//        // Emit one vertex per triangulation node
//        const std::uint32_t base =
//            static_cast<std::uint32_t>(out.vertices.size());
//
//        out.vertices.reserve(out.vertices.size() + static_cast<std::size_t>(nbNodes));
//
//        for (Standard_Integer i = 1; i <= nbNodes; ++i)
//        {
//            gp_Vec n = nodeNormals[static_cast<std::size_t>(i)];
//
//            if (n.SquareMagnitude() < 1.0e-24)
//            {
//                n = gp_Vec(0.0, 0.0, 1.0);
//            }
//            else
//            {
//                n.Normalize();
//            }
//
//            out.vertices.push_back(makeVertex(
//                worldNodes[static_cast<std::size_t>(i)],
//                n));
//        }
//
//        // Reuse those vertices via indices
//        out.indices.reserve(out.indices.size() + static_cast<std::size_t>(nbTris) * 3);
//
//        for (Standard_Integer i = 1; i <= nbTris; ++i)
//        {
//            Standard_Integer n1 = 0, n2 = 0, n3 = 0;
//            tri->Triangle(i).Get(n1, n2, n3);
//
//            if (face.Orientation() == TopAbs_REVERSED)
//            {
//                std::swap(n2, n3);
//            }
//
//            out.indices.push_back(base + static_cast<std::uint32_t>(n1 - 1));
//            out.indices.push_back(base + static_cast<std::uint32_t>(n2 - 1));
//            out.indices.push_back(base + static_cast<std::uint32_t>(n3 - 1));
//        }
//    }
//
//    if (out.vertices.empty())
//    {
//        throw std::runtime_error("No triangulation data was extracted from the shape.");
//    }
//
//    return out;
//}

#include "core/render_types.h"
#include "geometry/shape_gen.h"

#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>

#include <Geom_Surface.hxx>
#include <GeomLProp_SLProps.hxx>

#include <Poly_Triangulation.hxx>

#include <TopAbs_ShapeEnum.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>

#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>
#include <gp_Vec.hxx>

#include <cstdint>
#include <stdexcept>
#include <vector>

static Vertex makeVertex(const gp_Pnt& p, const gp_Vec& n)
{
    return Vertex{
        static_cast<float>(p.X()),
        static_cast<float>(p.Y()),
        static_cast<float>(p.Z()),
        static_cast<float>(n.X()),
        static_cast<float>(n.Y()),
        static_cast<float>(n.Z())
    };
}

RenderMesh prepareForOpenGL(const TopoDS_Shape& shape,
    const double linearDeflection,
    const double angularDeflection)
{
    BRepMesh_IncrementalMesh mesher(
        shape,
        linearDeflection,
        Standard_False,
        angularDeflection,
        Standard_True
    );

    RenderMesh out;

    for (TopExp_Explorer exp(shape, TopAbs_FACE); exp.More(); exp.Next())
    {
        const TopoDS_Face face = TopoDS::Face(exp.Current());

        TopLoc_Location faceLoc;
        Handle(Poly_Triangulation) tri = BRep_Tool::Triangulation(face, faceLoc);
        if (tri.IsNull())
            continue;

        const Standard_Integer nbNodes = tri->NbNodes();
        const Standard_Integer nbTris = tri->NbTriangles();

        if (nbNodes <= 0 || nbTris <= 0)
            continue;

        const gp_Trsf trsf = faceLoc.Transformation();
        const bool reversed = (face.Orientation() == TopAbs_REVERSED);

        // Get the underlying geometric surface so we can evaluate
        // the analytical normal at each UV parameter
        Handle(Geom_Surface) surf = BRep_Tool::Surface(face, faceLoc);

        // GeomLProp_SLProps evaluates surface properties (normal, curvature etc.)
        // at a given UV point. We reuse one instance and just move it per node.
        // The second argument (1) means "compute up to first derivatives" which
        // is all we need for the normal. The third (1e-7) is the resolution
        // used to detect singular points.
        GeomLProp_SLProps props(surf, 1, 1.0e-7);

        const std::uint32_t base = static_cast<std::uint32_t>(out.vertices.size());

        out.vertices.reserve(out.vertices.size() + static_cast<std::size_t>(nbNodes));

        for (Standard_Integer i = 1; i <= nbNodes; ++i)
        {
            // World-space position
            const gp_Pnt worldPt = tri->Node(i).Transformed(trsf);

            // UV parameters stored in the triangulation node
            // (available after BRepMesh_IncrementalMesh with UV storage enabled)
            gp_Vec normal;

            if (tri->HasUVNodes())
            {
                const gp_Pnt2d uv = tri->UVNode(i);
                props.SetParameters(uv.X(), uv.Y());

                if (props.IsNormalDefined())
                {
                    gp_Dir dir = props.Normal();

                    // Reverse if the face orientation is reversed
                    if (reversed)
                        dir.Reverse();

                    normal = gp_Vec(dir.X(), dir.Y(), dir.Z());

                    // The surface normal is in the surface's local space if the
                    // face has a location — transform it to world space
                    if (!faceLoc.IsIdentity())
                    {
                        // Only the rotational part of the transform applies to directions
                        normal = normal.Transformed(trsf);
                    }
                }
                else
                {
                    // Singular point (e.g. cone apex, sphere pole) —
                    // fall back to the position-based heuristic below
                    normal = gp_Vec(0.0, 0.0, 0.0);
                }
            }
            else
            {
                // Triangulation has no UV nodes — fall back to zero
                // (will be caught below and replaced)
                normal = gp_Vec(0.0, 0.0, 0.0);
            }

            // Fallback for degenerate cases: use the world position
            // normalised as a rough outward direction (only correct for
            // origin-centred convex shapes, but better than a zero normal)
            if (normal.SquareMagnitude() < 1.0e-24)
            {
                normal = gp_Vec(worldPt.X(), worldPt.Y(), worldPt.Z());
                if (normal.SquareMagnitude() < 1.0e-24)
                    normal = gp_Vec(0.0, 0.0, 1.0);
                else
                    normal.Normalize();
            }
            else
            {
                normal.Normalize();
            }

            out.vertices.push_back(makeVertex(worldPt, normal));
        }

        // Indices — unchanged logic
        out.indices.reserve(out.indices.size() + static_cast<std::size_t>(nbTris) * 3);

        for (Standard_Integer i = 1; i <= nbTris; ++i)
        {
            Standard_Integer n1 = 0, n2 = 0, n3 = 0;
            tri->Triangle(i).Get(n1, n2, n3);

            if (reversed)
                std::swap(n2, n3);

            out.indices.push_back(base + static_cast<std::uint32_t>(n1 - 1));
            out.indices.push_back(base + static_cast<std::uint32_t>(n2 - 1));
            out.indices.push_back(base + static_cast<std::uint32_t>(n3 - 1));
        }
    }

    if (out.vertices.empty())
        throw std::runtime_error("No triangulation data was extracted from the shape.");

    return out;
}