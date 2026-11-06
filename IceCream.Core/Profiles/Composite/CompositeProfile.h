#pragma once

#include "../Basic/IRadialProfile.h"

#include <memory>
#include <vector>

namespace IceCream::Core::Profiles
{
	/// <summary>
	/// радиальный профиль состоящих из нескольких соедененных псоелдовательно профилей
	/// </summary>
	class CompositeProfile : public IRadialProfile
	{
	private:
		std::vector<std::shared_ptr<const IRadialProfile>> profiles;
	
	public:
		/// <summary>
		/// создает составной профиль
		/// </summary>
		explicit CompositeProfile(std::vector<std::shared_ptr<const IRadialProfile>> profiles);

		size_t ProfileCount() const
		{
			return profiles.size();
		}

		const IRadialProfile& Profile(size_t index) const;

		double MinZ() const override;
		double MaxZ() const override;

		double GetRadius(double z) const override;
	};
}