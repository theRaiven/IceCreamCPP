#include "SectionResult.h"

namespace IceCream::Core::Models
{
	SectionResult::SectionResult(Plane3D plane, std::vector<SectionContour> contours) : plane_(plane), contours_(std::move(contours)) { }
}
