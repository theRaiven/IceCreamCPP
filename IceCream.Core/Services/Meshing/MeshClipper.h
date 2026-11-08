#pragma once

#include "../../Geometry/Meshes/TriangleMesh.h"
#include "../../Expressions/Inequalities/InequalityRegion.h"

#include <cstddef>

namespace IceCream::Core::Services
{
    struct MeshClipperOptions
    {
        int maxSubdivisionDepth = 4;

        double maxEdgeLength = 0.0;

        size_t maxTriangles = 1000000;
    };

    class MeshClipper
    {
    public:
        // Optional provenance maps each output triangle to its input triangle (e.g. for colors).
        Geometry::TriangleMesh Clip(const Geometry::TriangleMesh& mesh, const Inequalities::InequalityRegion& region,
            const MeshClipperOptions& options = {}, std::vector<size_t>* sourceTriangles = nullptr) const;
    };
}
