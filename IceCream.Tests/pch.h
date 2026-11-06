
#ifndef PCH_H
#define PCH_H

#include "CppUnitTest.h"

#include "../IceCream.Core/Geometry/Primitives/Plane3D.h"
#include "../IceCream.Core/Geometry/Primitives/Point2D.h"
#include "../IceCream.Core/Geometry/Primitives/Point3D.h"
#include "../IceCream.Core/Geometry/Primitives/Vector3D.h"
#include "../IceCream.Core/Geometry/Meshes/TriangleMesh.h"

#include "../IceCream.Core/Models/IceCreamBody.h"
#include "../IceCream.Core/Models/IceCreamParameters.h"

#include "../IceCream.Core/Services/Sections/SectionContour.h"
#include "../IceCream.Core/Services/Sections/SectionResult.h"

#include "../IceCream.Core/Profiles/Basic/FormulaProfile.h"
#include "../IceCream.Core/Profiles/Basic/IRadialProfile.h"
#include "../IceCream.Core/Profiles/Basic/PowerProfile.h"
#include "../IceCream.Core/Profiles/Basic/SphericalCapProfile.h"

#include "../IceCream.Core/Profiles/Composite/IceCreamProfile.h"
#include "../IceCream.Core/Profiles/Composite/TwoPartProfile.h"
#include "../IceCream.Core/Profiles/Composite/CompositeProfile.h"

#include "../IceCream.Core/Services/Meshing/HeightFieldBuilder.h"
#include "../IceCream.Core/Services/Meshing/SurfaceMeshBuilder.h"
#include "../IceCream.Core/Services/Meshing/ImplicitSurfaceBuilder.h"

#include "../IceCream.Core/Services/Sections/MeshSectionCalculator.h"
#include "../IceCream.Core/Services/Measurements/SectionAreaCalculator.h"
#include "../IceCream.Core/Services/Measurements/VolumeCalculator.h"

#endif
