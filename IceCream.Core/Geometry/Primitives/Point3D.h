#pragma once

#include "Vector3D.h"

namespace IceCream::Core::Geometry
{
	struct Point3D
	{
		double X = 0;
		double Y = 0;
		double Z = 0;

		static Point3D Origin()
		{
			return {0, 0, 0};
		}

		double DistanceTo(Point3D other) const;
	};

	Vector3D operator-(Point3D a, Point3D b);
	Point3D operator+(Point3D point, Vector3D vector);
	Point3D operator-(Point3D point, Vector3D vector);
}
