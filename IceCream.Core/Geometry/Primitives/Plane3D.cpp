#include "Plane3D.h"

namespace IceCream::Core::Geometry
{
	Plane3D::Plane3D(Point3D origin, Vector3D normal)
	{
		if (!std::isfinite(origin.X) || !std::isfinite(origin.Y) || !std::isfinite(origin.Z))
		{
			throw std::invalid_argument("Координаты точки должны быть конечными.");
		}

		origin_ = origin;
		normal_ = normal.Normalized();

		// выбираем вспомогательное направление, которое не будет почти параллельно нормали.
		Vector3D reference = std::abs(Normal().X) < 0.9 ? Vector3D::UnitX() : Vector3D::UnitY(); // пока условно. нужно подумать как изменять

		// проецируем вспомогательное направление на плоскость.
		axisU_ = (reference - Normal() * reference.Dot(Normal())).Normalized();

		// получаем вторую ось, перпендикулярную первой и нормали.
		axisV_ = Normal().Cross(AxisU()).Normalized();
	}

	double Plane3D::SignedDistanceTo(Point3D point) const
	{
		return Normal().Dot(point - Origin());
	}

	Point3D Plane3D::Project(Point3D point) const
	{
		return point - Normal() * SignedDistanceTo(point);
	}

	Plane3D Plane3D::AtX(double x)
	{
		return {{x, 0, 0}, Vector3D::UnitX()};
	}

	Plane3D Plane3D::AtY(double y)
	{
		return {{0, y, 0}, Vector3D::UnitY()};
	}

	Plane3D Plane3D::AtZ(double z)
	{
		return {{0, 0, z}, Vector3D::UnitZ()};
	}

	Point2D Plane3D::ToLocal(Point3D point) const
	{
		Vector3D offset = point - Origin();

		return {offset.Dot(AxisU()), offset.Dot(AxisV())};
	}

	Point3D Plane3D::ToGlobal(Point2D point) const
	{
		return Origin() + AxisU() * point.X + AxisV() * point.Y;
	}
}
