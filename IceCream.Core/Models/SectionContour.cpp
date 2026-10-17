#include "SectionContour.h"

namespace IceCream::Core::Models
{
	SectionContour::SectionContour(std::vector<Point3D> points3D, std::vector<Point2D> points2D, bool isClosed)
	{
		// многа праверак, что он норм
		if (points3D.size() != points2D.size())
		{
			throw std::invalid_argument("Количество точек в 3D и 2D должно совпадать.");
		}

		if (points3D.empty())
		{
			throw std::invalid_argument("Контур должен содержать хотя бы одну точку.");
		}

		if (isClosed && points3D.size() < 3)
		{
			throw std::invalid_argument("Замкнутый контур должен содержать минимум три точки.");
		}

		for (const auto& point : points3D)
		{
			if (!std::isfinite(point.X) || !std::isfinite(point.Y) || !std::isfinite(point.Z))
			{
				throw std::invalid_argument("Координаты должны быть конечными.");
			}
		}

		for (const auto& point : points2D)
		{
			if (!std::isfinite(point.X) || !std::isfinite(point.Y))
			{
				throw std::invalid_argument("Координаты должны быть конечными.");
			}
		}

		points3D_ = std::move(points3D);
		points2D_ = std::move(points2D);
		isClosed_ = isClosed;
	}
}
