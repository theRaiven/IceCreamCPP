#ifndef PCH_H
#define PCH_H

#include "CppUnitTest.h"

#include "../IceCream.Core/Geometry/Plane3D.h"
#include "../IceCream.Core/Geometry/Point2D.h"
#include "../IceCream.Core/Geometry/Point3D.h"
#include "../IceCream.Core/Geometry/TriangleMesh.h"
#include "../IceCream.Core/Geometry/Vector3D.h"

#include "../IceCream.Core/Models/IceCreamBody.h"
#include "../IceCream.Core/Models/IceCreamParameters.h"
#include "../IceCream.Core/Models/SectionContour.h"
#include "../IceCream.Core/Models/SectionResult.h"

#include "../IceCream.Core/Profiles/FormulaProfile.h"
#include "../IceCream.Core/Profiles/IceCreamProfile.h"
#include "../IceCream.Core/Profiles/IRadialProfile.h"
#include "../IceCream.Core/Profiles/PowerProfile.h"
#include "../IceCream.Core/Profiles/SphericalCapProfile.h"
#include "../IceCream.Core/Profiles/TwoPartProfile.h"
#include "../IceCream.Core/Profiles/CompositeProfile.h"

#include "../IceCream.Core/Services/HeightFieldBuilder.h"
#include "../IceCream.Core/Services/MeshSectionCalculator.h"
#include "../IceCream.Core/Services/SectionAreaCalculator.h"
#include "../IceCream.Core/Services/SurfaceMeshBuilder.h"
#include "../IceCream.Core/Services/VolumeCalculator.h"

#endif