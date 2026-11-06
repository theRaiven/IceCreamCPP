#include "PowerProfile.h"

namespace IceCream::Core::Profiles
{
	PowerProfile::PowerProfile(double height, double topRadius, double exponent)
	{
		if (!std::isfinite(height)    || height <= 0)
		{
			throw std::out_of_range("Высота должна быть конечным положительным числом.");
		}

		if (!std::isfinite(topRadius) || topRadius <= 0)
		{
			throw std::out_of_range("Радиус должен быть конечным положительным числом.");
		}

		if (!std::isfinite(exponent)  || exponent <= 0)
		{
			throw std::out_of_range("Показатель степени должен быть конечным положительным числом.");
		}

		height_ = height;
		topRadius_ = topRadius;
		exponent_ = exponent;
	}

	double PowerProfile::GetRadius(double z) const
	{
		if (!std::isfinite(z) || z < MinZ() || z > Height())
		{
			throw std::out_of_range("Координата Z должна быть конечным числом в диапазоне [0, Height].");
		}

		return TopRadius() * std::pow(z / Height(), Exponent());
	}
}
