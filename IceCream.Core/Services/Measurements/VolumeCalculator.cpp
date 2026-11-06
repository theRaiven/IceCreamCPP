#include "VolumeCalculator.h"
#include <cmath>
#include <stdexcept>

namespace IceCream::Core::Services
{
	double VolumeCalculator::Calculate(const IRadialProfile& profile, int intervals) const
	{
		if (intervals < 2 || intervals % 2 != 0)
		{
			throw std::out_of_range("Количество промежутков должно быть чётным и не меньше двух.");
		}

		double minZ = profile.MinZ();
		double maxZ = profile.MaxZ();
		double height = maxZ - minZ;

		if (!std::isfinite(minZ) || !std::isfinite(maxZ) || !std::isfinite(height) || height <= 0)
		{
			throw std::invalid_argument("Профиль должен иметь конечный положительный диапазон высот.");
		}

		double step = height / intervals;

		double sum = GetSectionArea(profile, minZ) + GetSectionArea(profile, maxZ);

		for (int i = 1; i < intervals; i++)
		{
			double z = minZ + height * (static_cast<double>(i) / intervals);

			int weight = i % 2 == 0 ? 2 : 4;

			sum += weight * GetSectionArea(profile, z);
		}

		double volume = step / 3 * sum;

		if (!std::isfinite(volume))
		{
			throw std::runtime_error("При вычислении объёма произошло переполнение.");
		}

		return volume;
	}

	double VolumeCalculator::Calculate(const Geometry::TriangleMesh& mesh) const
	{
		double volume = 0.0;

		for (size_t i = 0; i < mesh.TriangleCount(); ++i)
		{
			auto [a, b, c] = mesh.GetTriangle(i);

			volume += a.X * (b.Y * c.Z - b.Z * c.Y);
			volume += a.Y * (b.Z * c.X - b.X * c.Z);
			volume += a.Z * (b.X * c.Y - b.Y * c.X);
		}

		return std::abs(volume / 6.0);
	}

	double VolumeCalculator::GetSectionArea(const IRadialProfile& profile, double z)
	{
		double radius = profile.GetRadius(z);

		if (!std::isfinite(radius) || radius < 0)
		{
			throw std::runtime_error("Профиль должен возвращать конечный неотрицательный радиус.");
		}

		return std::acos(-1.0) * radius * radius;
	}
}
