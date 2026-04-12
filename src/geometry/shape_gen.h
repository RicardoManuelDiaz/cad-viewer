#pragma once
#include <TopoDS_Shape.hxx>

TopoDS_Shape makeExtrudedCircle(double radius, double height);
TopoDS_Shape makeBox(double sizeX, double sizeY, double sizeZ);
TopoDS_Shape makeSphere(double radius);
TopoDS_Shape makeCone(double radiusBottom, double radiusTop, double height);
TopoDS_Shape makeTorus(double majorRadius, double minorRadius);
TopoDS_Shape makeExtrudedPolygon(int numSides, double radius, double height);
TopoDS_Shape makeHollowCylinder(const double outerRadius, const double innerRadius, const double height);
TopoDS_Shape makeBoxWithHole(const double boxSize, const double holeRadius);
TopoDS_Shape makeRevolvedProfile(const double innerR, const double outerR, const double height, const double chamfer);
TopoDS_Shape makeFilletBox(const double boxSize, const double filletRadius);
