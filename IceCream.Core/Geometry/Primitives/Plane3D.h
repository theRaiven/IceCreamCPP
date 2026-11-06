#pragma once

#include "Point2D.h"
#include "Point3D.h"

namespace IceCream::Core::Geometry
{
	/// <summary>
	/// Секущая плоскость в трехмерном пространстве, определяемая точкой и нормалью.
	/// </summary>
	class Plane3D
	{
	private:
		Point3D origin_;
		Vector3D normal_;
		Vector3D axisU_;
		Vector3D axisV_;

	public:
		const Point3D& Origin() const
		{
			return origin_;
		}

		const Vector3D& Normal() const
		{
			return normal_;
		}

		/// <summary>
		/// Первая единичная ось внутри плоскости.
		/// </summary>
		const Vector3D& AxisU() const
		{
			return axisU_;
		}

		/// <summary>
		/// Вторая единичная ось внутри плоскости.
		/// </summary>
		const Vector3D& AxisV() const
		{
			return axisV_;
		}

		/// <summary>
		/// Инициализирует новую плоскость в трехмерном пространстве с заданной точкой и нормалью.
		/// </summary>
		/// <param name="origin">Точка на плоскости.</param>
		/// <param name="normal">Нормаль плоскости.</param>
		/// <exception cref="ArgumentException"></exception>
		Plane3D(Point3D origin, Vector3D normal);

		/// <summary>
		/// Вычисляет знаковое расстояние от точки до плоскости
		/// отрицательно - точка находится по направлению нормали,
		/// положительно - в противоположном направлении
		/// </summary>
		/// <param name="point">Точка, для которой вычисляется расстояние.</param>
		/// <returns>Знаковое расстояние от точки до плоскости.</returns>
		double SignedDistanceTo(Point3D point) const;

		/// <summary>
		/// Проецирует точку на плоскость
		/// </summary>
		/// <param name="point">Точка, которую нужно спроецировать.</param>
		/// <returns>Ближайшая точка на плоскости.</returns>
		Point3D Project(Point3D point) const;

		/// <summary>
		/// Создает плоскость, проходящую через заданное значение координаты X
		/// </summary>
		/// <param name="x">Значение координаты X.</param>
		/// <returns>Плоскость, проходящая через заданное значение координаты X.</returns>
		static Plane3D AtX(double x);

		/// <summary>
		/// Создает плоскость, проходящую через заданное значение координаты X
		/// </summary>
		/// <param name="x">Значение координаты X.</param>
		/// <returns>Плоскость, проходящая через заданное значение координаты X.</returns>
		static Plane3D AtY(double y);

		/// <summary>
		/// Создает плоскость, проходящую через заданное значение координаты X
		/// </summary>
		/// <param name="x">Значение координаты X.</param>
		/// <returns>Плоскость, проходящая через заданное значение координаты X.</returns>
		static Plane3D AtZ(double z);

		/// <summary>
		/// Возвращает локальные координаты проекции точки на плоскость.
		/// </summary>
		Point2D ToLocal(Point3D point) const;

		/// <summary>
		/// Преобразует локальные координаты в точку на плоскости.
		/// </summary>
		Point3D ToGlobal(Point2D point) const;
	};
}
