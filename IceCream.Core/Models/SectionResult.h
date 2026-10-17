#pragma once

#include "SectionContour.h"
#include "../Geometry/Plane3D.h"

namespace IceCream::Core::Models
{
	using Geometry::Plane3D;

	/// <summary>
	/// Представляет результат сечения, содержащий плоскость сечения и список контуров.
	/// </summary>
	class SectionResult
	{
	private:
		Plane3D plane_;
		std::vector<SectionContour> contours_;

	public:
		const Plane3D& Plane() const
		{
			return plane_;
		}

		const std::vector<SectionContour>& Contours() const
		{
			return contours_;
		}

		bool IsEmpty() const
		{
			return Contours().empty();
		}

		SectionResult(Plane3D plane, std::vector<SectionContour> contours);
	};
}
