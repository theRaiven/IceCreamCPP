#pragma once

#include <cmath>

namespace IceCream::Core::Geometry
{
	struct Point2D
	{
		double X = 0;
		double Y = 0;

		double DistanceTo(Point2D other) const;
	};
}
