#pragma once

#include "../Profiles/IceCreamProfile.h"

namespace IceCream::Core::Services
{
	using Profiles::IRadialProfile;
	using Profiles::IceCreamProfile;

	class VolumeCalculator
	{
	public:
		/// <summary>
		/// Вычисляет объём всего мороженого.
		/// Каждая часть интегрируется отдельно.
		/// </summary>
		/// <param name="profile">Профиль мороженого.</param>
		/// <param name="intervalsPerPart">
		/// Чётное положительное количество промежутков для каждой части.
		/// </param>
		double Calculate(const IceCreamProfile& profile, int intervalsPerPart = 1000) const;

		/// <summary>
		/// Разбиваем высоту на чётное количество промежутков n, по шаге h = (maxZ - minZ) / n
		/// Вычисляем площади кругов S0, ... , Sn
		/// и складываем с весами V = h/3 * (S0 + 4*S1 + 2*S2 + 4*S3 + ... + 4*S_(n-1) + Sn)
		/// </summary>
		/// <param name="profile"> Профиль мороженого. </param>
		/// <param name="intervals"> Чётное положительное количество промежутков. </param>
		/// <returns> Объём мороженого. </returns>
		double Calculate(const IRadialProfile& profile, int intervals) const;

	private:
		static double GetSectionArea(const IRadialProfile& profile, double z);
	};
}
