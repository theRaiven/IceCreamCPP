#include "Point3D.h"

namespace IceCream::Core::Geometry
{
	double Point3D::DistanceTo(Point3D other) const
	{
		auto dx = X - other.X;
		auto dy = Y - other.Y;
		auto dz = Z - other.Z;

		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	Vector3D operator-(Point3D a, Point3D b)
	{
		return {a.X - b.X, a.Y - b.Y, a.Z - b.Z};
	}

	Point3D operator+(Point3D point, Vector3D vector)
	{
		return {point.X + vector.X, point.Y + vector.Y, point.Z + vector.Z};
	}

	Point3D operator-(Point3D point, Vector3D vector)
	{
		return {point.X - vector.X, point.Y - vector.Y, point.Z - vector.Z};
	}
}
