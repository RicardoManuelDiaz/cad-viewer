#include <cmath>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepPrimAPI_MakeCone.hxx>
#include <BRepPrimAPI_MakeTorus.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRepOffsetAPI_MakePipe.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepOffsetAPI_ThruSections.hxx>

#include <BRepAlgoAPI_Cut.hxx>

#include <Geom_Circle.hxx>
#include <Geom_TrimmedCurve.hxx>

#include <GC_MakeArcOfCircle.hxx>

#include <TopExp_Explorer.hxx>

#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

#include <gp.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Vec.hxx>
#include <gp_Pnt.hxx>
#include <gp_Dir.hxx>

TopoDS_Shape makeExtrudedCircle(const double radius, const double height)
{
	Handle(Geom_Circle) circle	= new Geom_Circle(gp_Ax2(gp::Origin(), gp::DZ()), radius);

    BRepBuilderAPI_MakeEdge     edgeMaker(circle);
    TopoDS_Edge circleEdge      = edgeMaker.Edge();

    BRepBuilderAPI_MakeWire     wireMaker(circleEdge);
    TopoDS_Wire circleWire      = wireMaker.Wire();

    BRepBuilderAPI_MakeFace     faceMaker(circleWire, Standard_True);
    TopoDS_Face profileFace     = faceMaker.Face();

    BRepPrimAPI_MakePrism       prismMaker(profileFace, gp_Vec(0.0, 0.0, height));

    return prismMaker.Shape();
}

TopoDS_Shape makeBox(const double sizeX, const double sizeY, const double sizeZ)
{
    return BRepPrimAPI_MakeBox(sizeX, sizeY, sizeZ).Shape();
}

TopoDS_Shape makeSphere(const double radius)
{
    return BRepPrimAPI_MakeSphere(radius).Shape();
}

TopoDS_Shape makeCone(const double radiusBottom, const double radiusTop, const double height)
{
    return BRepPrimAPI_MakeCone(radiusBottom, radiusTop, height).Shape();
}

TopoDS_Shape makeTorus(const double majorRadius, const double minorRadius)
{
    return BRepPrimAPI_MakeTorus(majorRadius, minorRadius).Shape();
}

TopoDS_Shape makeExtrudedPolygon(const int numSides, const double radius, const double height)
{
    BRepBuilderAPI_MakePolygon polyMaker;

    for (int i = 0; i < numSides; ++i)
    {
        const double angle = (2.0 * M_PI * i) / numSides;
        polyMaker.Add(gp_Pnt(
            radius * std::cos(angle),
            radius * std::sin(angle),
            0.0));
    }

    polyMaker.Close();

    BRepBuilderAPI_MakeFace faceMaker(polyMaker.Wire(), Standard_True);
    BRepPrimAPI_MakePrism   prismMaker(faceMaker.Face(), gp_Vec(0.0, 0.0, height));

    return prismMaker.Shape();
}

TopoDS_Shape makeHollowCylinder(
    const double outerRadius,
    const double innerRadius,
    const double height)
{
    const TopoDS_Shape outer = BRepPrimAPI_MakeCylinder(outerRadius, height).Shape();
    const TopoDS_Shape inner = BRepPrimAPI_MakeCylinder(innerRadius, height).Shape();

    return BRepAlgoAPI_Cut(outer, inner).Shape();
}

TopoDS_Shape makeBoxWithHole(
    const double boxSize,
    const double holeRadius)
{
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(
        gp_Pnt(-boxSize * 0.5, -boxSize * 0.5, 0.0),
        boxSize, boxSize, boxSize).Shape();

    // Cylinder centred on the box, running full height along Z
    const gp_Ax2 cylAx(
        gp_Pnt(0.0, 0.0, -1.0),   // start slightly below box bottom
        gp_Dir(0.0, 0.0, 1.0));

    const TopoDS_Shape cyl =
        BRepPrimAPI_MakeCylinder(cylAx, holeRadius, boxSize + 2.0).Shape();

    return BRepAlgoAPI_Cut(box, cyl).Shape();
}

TopoDS_Shape makeRevolvedProfile(
    const double innerR,
    const double outerR,
    const double height,
    const double chamfer)
{
    // Profile points in the XZ plane (X = radius, Z = height)
    // Wound counter-clockwise so the face normal points outward
    const gp_Pnt p0(innerR, 0.0, 0.0);
    const gp_Pnt p1(outerR, 0.0, 0.0);
    const gp_Pnt p2(outerR, 0.0, height - chamfer);
    const gp_Pnt p3(outerR - chamfer, 0.0, height);
    const gp_Pnt p4(innerR, 0.0, height);

    BRepBuilderAPI_MakeWire wireMaker;
    wireMaker.Add(BRepBuilderAPI_MakeEdge(p0, p1).Edge());
    wireMaker.Add(BRepBuilderAPI_MakeEdge(p1, p2).Edge());
    wireMaker.Add(BRepBuilderAPI_MakeEdge(p2, p3).Edge());
    wireMaker.Add(BRepBuilderAPI_MakeEdge(p3, p4).Edge());
    wireMaker.Add(BRepBuilderAPI_MakeEdge(p4, p0).Edge());

    BRepBuilderAPI_MakeFace faceMaker(wireMaker.Wire(), Standard_True);

    // Revolve 360° around the Z axis
    const gp_Ax1 zAxis(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1));

    return BRepPrimAPI_MakeRevol(faceMaker.Face(), zAxis).Shape();
}

TopoDS_Shape makeFilletBox(
    const double boxSize,
    const double filletRadius)
{
    const TopoDS_Shape box = BRepPrimAPI_MakeBox(
        gp_Pnt(-boxSize * 0.5, -boxSize * 0.5, 0.0),
        boxSize, boxSize, boxSize).Shape();

    BRepFilletAPI_MakeFillet filleter(box);

    // Add every edge of the box to the fillet operation
    for (TopExp_Explorer ex(box, TopAbs_EDGE); ex.More(); ex.Next())
        filleter.Add(filletRadius, TopoDS::Edge(ex.Current()));

    filleter.Build();

    return filleter.Shape();
}
