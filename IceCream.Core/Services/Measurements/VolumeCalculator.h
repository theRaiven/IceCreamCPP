#pragma once

#include "../../Profiles/Composite/IceCreamProfile.h"
#include "../../Profiles/Composite/TwoPartProfile.h"
#include "../../Profiles/Basic/IRadialProfile.h"
#include "../../Geometry/Meshes/TriangleMesh.h"

namespace IceCream::Core::Services
{
	using Profiles::IRadialProfile;
	using Profiles::IceCreamProfile;
	using Profiles::TwoPartProfile;

	class VolumeCalculator
	{
		
	public:
		/// <summary>
		/// Разбиваем высоту на чётное количество промежутков n, по шаге h = (maxZ - minZ) / n
		/// Вычисляем площади кругов S0, ... , Sn
		/// и складываем с весами V = h/3 * (S0 + 4*S1 + 2*S2 + 4*S3 + ... + 4*S_(n-1) + Sn)
		/// </summary>
		/// <param name="profile"> Профиль мороженого. </param>
		/// <param name="intervals"> Чётное положительное количество промежутков. </param>
		/// <returns> Объём мороженого. </returns>
		double Calculate(const IRadialProfile& profile, int intervals = 1000) const;

		double Calculate(const Geometry::TriangleMesh& mesh) const;
	private:
		static double GetSectionArea(const IRadialProfile& profile, double z);
	};
}
