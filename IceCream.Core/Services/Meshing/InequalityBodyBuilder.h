#pragma once

#include "../../Expressions/Inequalities/InequalityRegion.h"
#include "../../Geometry/Meshes/TriangleMesh.h"

namespace IceCream::Core::Services
{
    // Строит границу области по системе неравенств.
    // Для вертикальных тел с плоскими верхом и низом строит сетку по общему контуру.
    // В остальных случаях использует Marching Cubes.
    class InequalityBodyBuilder
    {
    public:
        Geometry::TriangleMesh Build(const Inequalities::InequalityRegion& region,
            Geometry::Point3D min, Geometry::Point3D max,
            int cellsX = 40, int cellsY = 40, int cellsZ = 40) const;
    };
}
