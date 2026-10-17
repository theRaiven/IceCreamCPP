#pragma once

#include "../Geometry/Point3D.h"
#include "../Geometry/Point2D.h"
#include <vector>
#include <utility>

namespace IceCream::Core::Models
{
	using Geometry::Point3D;
	using Geometry::Point2D;

	/// <summary>
	/// Представляет контур с точками в 3D и 2D пространстве, а также информацию о том, замкнут ли контур.
	/// </summary>
	class SectionContour
	{
	private:
		std::vector<Point3D> points3D_;
		std::vector<Point2D> points2D_;
		bool isClosed_;

	public:
		const std::vector<Point3D>& Points3D() const
		{
			return points3D_;
		}

		const std::vector<Point2D>& Points2D() const
		{
			return points2D_;
		}

		bool IsClosed() const
		{
			return isClosed_;
		}

		SectionContour(std::vector<Point3D> points3D, std::vector<Point2D> points2D, bool isClosed);
	};
}
