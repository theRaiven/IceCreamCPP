#pragma once

#include <cmath>
#include <stdexcept>

namespace IceCream::Core::Geometry
{
	/// <summary>
	/// Представляет трехмерный вектор с координатами X, Y и Z.
	/// </summary>
	/// <param name="X">Координата X.</param>
	/// <param name="Y">Координата Y.</param>
	/// <param name="Z">Координата Z.</param>
	struct Vector3D
	{
		double X = 0;
		double Y = 0;
		double Z = 0;

		static Vector3D Zero()
		{
			return {0, 0, 0};
		}

		static Vector3D UnitX()
		{
			return {1, 0, 0};
		}

		static Vector3D UnitY()
		{
			return {0, 1, 0};
		}

		static Vector3D UnitZ()
		{
			return {0, 0, 1};
		}

		double LengthSquared() const
		{
			return X * X + Y * Y + Z * Z;
		}

		double Length() const
		{
			return std::sqrt(LengthSquared());
		}

		/// <summary>
		/// Нормализует вектор
		/// </summary>
		/// <returns>возвращая новый вектор с той же направленностью, но длиной равной 1</returns>
		Vector3D Normalized() const;

		/// <summary>
		/// Вычисляет скалярное произведение (dot product) текущего вектора с другим вектором.
		/// </summary>
		/// <param name="other">Другой вектор.</param>
		/// <returns>Скалярное произведение.</returns>
		double Dot(Vector3D other) const;

		/// <summary>
		/// Вычисляет векторное произведение (cross product) текущего вектора с другим вектором.
		/// </summary>
		/// <param name="other">Другой вектор.</param>
		/// <returns>Векторное произведение.</returns>
		Vector3D Cross(Vector3D other) const;
	};

	Vector3D operator+(Vector3D a, Vector3D b);
	Vector3D operator-(Vector3D a, Vector3D b);
	Vector3D operator-(Vector3D vector);
	Vector3D operator*(Vector3D vector, double scalar);
	Vector3D operator*(double scalar, Vector3D vector);
	Vector3D operator/(Vector3D vector, double scalar);
}
