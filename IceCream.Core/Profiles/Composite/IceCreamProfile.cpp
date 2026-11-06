#include "IceCreamProfile.h"

namespace IceCream::Core::Profiles
{
	IceCreamProfile::IceCreamProfile(std::shared_ptr<const IRadialProfile> baseProfile, const SphericalCapProfile& capProfile)
		: baseProfile_(baseProfile), capProfile_(CreateCap(baseProfile, capProfile.Height()))
	{
		double baseHeight = baseProfile->MaxZ() - baseProfile->MinZ();
		double heightESP = 1e-10 * std::max(baseHeight, capProfile.Height());

		if (std::abs(baseProfile->MaxZ() - capProfile.MinZ()) > heightESP)
		{
			throw std::invalid_argument("Верх рожка и основание шапки должны находиться на одной высоте.");
		}

		double joinRadius = baseProfile->GetRadius(baseProfile->MaxZ());
		double radiusESP = 1e-10 * std::max(std::abs(joinRadius), capProfile.BaseRadius());

		if (!std::isfinite(joinRadius) || joinRadius <= 0 || std::abs(joinRadius - capProfile.BaseRadius()) > radiusESP)
		{
			throw std::invalid_argument("Радиусы рожка и шапки на стыке должны совпадать.");
		}
	}

	IceCreamProfile::IceCreamProfile(std::shared_ptr<const IRadialProfile> baseProfile, double capHeight)
		: IceCreamProfile(baseProfile, CreateCap(baseProfile, capHeight)) {}

	SphericalCapProfile IceCreamProfile::CreateCap(const std::shared_ptr<const IRadialProfile>& baseProfile, double capHeight)
	{
		if (!baseProfile)
		{
			throw std::invalid_argument("baseProfile не должен быть nullptr.");
		}

		return {baseProfile->MaxZ(), baseProfile->GetRadius(baseProfile->MaxZ()), capHeight};
	}

	double IceCreamProfile::GetRadius(double z) const
	{
		if (!std::isfinite(z))
		{
			throw std::out_of_range("Высота должна быть конечным числом.");
		}

		if (z < JoinZ())
		{
			return BaseProfile().GetRadius(z);
		}
		else if (z <= MaxZ())
		{
			return CapProfile().GetRadius(z);
		}
		else
		{
			throw std::out_of_range("Высота выходит за пределы профиля мороженного.");
		}
	}
}
