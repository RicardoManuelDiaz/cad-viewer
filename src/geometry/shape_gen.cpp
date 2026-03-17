#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakePrism.hxx>

#include <Geom_Circle.hxx>

#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>

#include <gp.hxx>
#include <gp_Ax2.hxx>
#include <gp_Vec.hxx>

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