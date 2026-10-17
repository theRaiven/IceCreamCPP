#pragma once

#include "../Models/SectionResult.h"

namespace IceCream::Core::Services
{
	using Models::SectionResult;
	using Models::SectionContour;
	using Geometry::Point2D;

	class SectionAreaCalculator
	{
	public:
		/// <summary>
		/// Вычисляет площадь сечения.
		/// Контуры должны быть простыми, без взаимных пересечений и касаний.
		/// Вложенные контуры учитываются как отверстия.
		/// </summary>
		double Calculate(const SectionResult& section) const;

	private:
		static double CalculatePolygonArea(const std::vector<Point2D>& points);

		/// <summary>
		/// Проверяет, находится ли точка внутри многоугольника.
		/// Предполагается, что точка не лежит на его границе.
		/// </summary>
		static bool IsInside(Point2D point, const std::vector<Point2D>& polygon);
	};
}
