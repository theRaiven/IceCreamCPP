#pragma once

#include "../../Geometry/Meshes/TriangleMesh.h"
#include "../../Expressions/Parsing/CompiledExpression.h"

namespace IceCream::Core::Services
{
    class ImplicitSurfaceBuilder
    {
    public:
        Geometry::TriangleMesh Build(const Expressions::CompiledExpression& expression, Geometry::Point3D min, Geometry::Point3D max, int cellsX = 32, int cellsY = 32, int cellsZ = 32) const;
    };
}