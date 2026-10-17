#include "Point2D.h"

namespace IceCream::Core::Geometry
{
	double Point2D::DistanceTo(Point2D other) const
	{
		auto dx = X - other.X;
		auto dy = Y - other.Y;

		return std::sqrt(dx * dx + dy * dy);
	}
}
