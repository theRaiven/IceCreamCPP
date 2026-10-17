#include "VolumeCalculator.h"

namespace IceCream::Core::Services
{
	double VolumeCalculator::Calculate(const IceCreamProfile& profile, int intervalsPerPart) const
	{
		double baseVolume = Calculate(profile.BaseProfile(), intervalsPerPart);

		double capVolume = Calculate(profile.CapProfile(), intervalsPerPart);

		double volume = baseVolume + capVolume;

		if (!std::isfinite(volume))
		{
			throw std::runtime_error("При вычислении объёма произошло переполнение.");
		}

		return volume;
	}

	double VolumeCalculator::Calculate(const IRadialProfile& profile, int intervals) const
	{
		// а надо ли....
		if (const auto* iceCream = dynamic_cast<const IceCreamProfile*>(&profile))
		{
			return Calculate(*iceCream, intervals);
		}

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
