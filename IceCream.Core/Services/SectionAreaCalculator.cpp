#include "SectionAreaCalculator.h"

namespace IceCream::Core::Services
{
	double SectionAreaCalculator::Calculate(const SectionResult& section) const
	{
		std::vector<const SectionContour*> closedContours;

		for (const SectionContour& contour : section.Contours())
		{
			if (contour.IsClosed())
			{
				closedContours.push_back(&contour);
				continue;
			}

			// Точка или одиночный отрезок имеют нулевую площадь.
			if (contour.Points2D().size() <= 2)
			{
				continue;
			}

			throw std::runtime_error(
				"Сечение содержит незамкнутую цепочку. "
				"Нельзя достоверно вычислить его площадь.");
		}

		double totalArea = 0;

		for (size_t i = 0; i < closedContours.size(); i++)
		{
			const std::vector<Point2D>& points = closedContours[i]->Points2D();

			double area = CalculatePolygonArea(points);

			// Считаем, сколько других контуров содержит этот
			int nestingDepth = 0;

			for (size_t j = 0; j < closedContours.size(); j++)
			{
				if (i == j)
				{
					continue;
				}

				if (IsInside(points[0], closedContours[j]->Points2D()))
				{
					nestingDepth++;
				}
			}

			// Внешняя область: плюс.
			// Отверстие: минус.
			// Остров внутри отверстия: снова плюс.
			if (nestingDepth % 2 == 0)
			{
				totalArea += area;
			}
			else
			{
				totalArea -= area;
			}
		}

		if (!std::isfinite(totalArea) || totalArea < 0)
		{
			throw std::runtime_error("Не удалось получить корректную площадь сечения.");
		}

		return totalArea;
	}

	double SectionAreaCalculator::CalculatePolygonArea(const std::vector<Point2D>& points)
	{
		// Сдвиг к первой вершине уменьшает влияние больших абсолютных значений координат
		Point2D origin = points[0];

		double doubledArea = 0;

		for (size_t i = 0; i < points.size(); i++)
		{
			size_t next = (i + 1) % points.size();

			double x1 = points[i].X - origin.X;
			double y1 = points[i].Y - origin.Y;

			double x2 = points[next].X - origin.X;
			double y2 = points[next].Y - origin.Y;

			doubledArea += x1 * y2 - x2 * y1;
		}

		return std::abs(doubledArea) / 2;
	}

	bool SectionAreaCalculator::IsInside(Point2D point, const std::vector<Point2D>& polygon)
	{
		bool inside = false;

		for (size_t i = 0; i < polygon.size(); i++)
		{
			size_t next = (i + 1) % polygon.size();

			Point2D a = polygon[i];
			Point2D b = polygon[next];

			// Ребро должно пересекать горизонтальный уовень точки
			if ((a.Y > point.Y) == (b.Y > point.Y))
			{
				continue;
			}

			double t = (point.Y - a.Y) / (b.Y - a.Y);

			double intersectionX = a.X + t * (b.X - a.X);

			// Считаем пересечения луча, направленного вправо.
			if (intersectionX > point.X)
			{
				inside = !inside;
			}
		}

		return inside;
	}
}
