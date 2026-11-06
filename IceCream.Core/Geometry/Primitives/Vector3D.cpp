#include "Vector3D.h"

namespace IceCream::Core::Geometry
{
	Vector3D Vector3D::Normalized() const
	{
		auto length = Length();

		if (!std::isfinite(length) || length == 0)
		{
			throw std::runtime_error("Нельзя нормализовать нулевой или некорректный вектор.");
		}

		return {X / length, Y / length, Z / length};
	}

	double Vector3D::Dot(Vector3D other) const
	{
		return X * other.X + Y * other.Y + Z * other.Z;
	}

	Vector3D Vector3D::Cross(Vector3D other) const
	{
		return {Y * other.Z - Z * other.Y, Z * other.X - X * other.Z, X * other.Y - Y * other.X};
	}

	Vector3D operator+(Vector3D a, Vector3D b)
	{
		return {a.X + b.X, a.Y + b.Y, a.Z + b.Z};
	}

	Vector3D operator-(Vector3D a, Vector3D b)
	{
		return {a.X - b.X, a.Y - b.Y, a.Z - b.Z};
	}

	Vector3D operator-(Vector3D vector)
	{
		return {-vector.X, -vector.Y, -vector.Z};
	}

	Vector3D operator*(Vector3D vector, double scalar)
	{
		return {vector.X * scalar, vector.Y * scalar, vector.Z * scalar};
	}

	Vector3D operator*(double scalar, Vector3D vector)
	{
		return vector * scalar;
	}

	Vector3D operator/(Vector3D vector, double scalar)
	{
		if (scalar == 0)
		{
			throw std::invalid_argument("Деление на ноль недопустимо.");
		}

		return {vector.X / scalar, vector.Y / scalar, vector.Z / scalar};
	}
}
