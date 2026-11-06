#include "SphericalCapProfile.h"

namespace IceCream::Core::Profiles
{
	SphericalCapProfile::SphericalCapProfile(double baseZ, double baseRadius, double height)
	{
		if (!std::isfinite(baseZ))
		{
			throw std::out_of_range("Высота основания должна быть конечным числом.");
		}

		if (!std::isfinite(baseRadius) || baseRadius <= 0)
		{
			throw std::out_of_range("Радиус основания должен быть конечным положительным числом.");
		}

		if (!std::isfinite(height) || height <= 0)
		{
			throw std::out_of_range("Высота шапки должна быть конечным положительным числом.");
		}

		baseZ_      = baseZ;
		baseRadius_ = baseRadius;
		height_     = height;

		maxZ_ = baseZ + height;

		sphereRadius_ = (baseRadius * baseRadius + height * height) / (2 * height); // (a^2 + h^2) / 2h
		centerZ_ = maxZ_ - sphereRadius_;

		if (!std::isfinite(maxZ_) || maxZ_ <= baseZ_ || !std::isfinite(sphereRadius_) || sphereRadius_ <= 0 || !std::isfinite(centerZ_))
		{
			throw std::invalid_argument("Параметры приводят к некорректным размерам сферы.");
		}
	}

	double SphericalCapProfile::GetRadius(double z) const
	{
		if (!std::isfinite(z))
		{
			throw std::out_of_range("Высота должна быть конечным числом.");
		}

		double esp = 1e-10 * Height();

		if (z < MinZ() - esp || z > MaxZ() + esp)
		{
			throw std::out_of_range("Высота z должна находиться в пределах шапки.");
		}

		if (std::abs(z - MinZ()) <= esp)
		{
			return BaseRadius();
		}

		if (std::abs(z - MaxZ()) <= esp)
		{
			return 0;
		}

		double t = (z - BaseZ()) / Height();

		return std::sqrt(1 - t) * std::sqrt(BaseRadius() * BaseRadius() + Height() * Height() * t);
	}
}
