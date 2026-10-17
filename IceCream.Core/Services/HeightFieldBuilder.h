#pragma once
#include "../Geometry/TriangleMesh.h"
#include <string>
#include <vector>
namespace IceCream::Core::Services
{
    struct HeightFieldBounds { double MinX=-1, MaxX=1, MinY=-1, MaxY=1; };
    struct HeightFieldSolid {
        Geometry::TriangleMesh Mesh;
        double Volume;
        double BaseZ;
        std::vector<std::vector<Geometry::Point3D>> Edges;
    };
    class HeightFieldBuilder {
    public:
        HeightFieldSolid Build(const std::string& formula,const HeightFieldBounds& bounds,int segments=64) const;
    };
}
