#pragma once

#include "IRadialProfile.h"
#include <cmath>
#include <stdexcept>

namespace IceCream::Core::Profiles
{
	class SphericalCapProfile : public IRadialProfile
	{
	private:
		double baseZ_;
		double baseRadius_;
		double height_;
		double sphereRadius_;
		double centerZ_;
		double maxZ_;

	public:
		double BaseZ() const
		{
			return baseZ_;
		} // высота стыка рожка и мороженого
		double BaseRadius() const
		{
			return baseRadius_;
		} // место стыка рожка и мороженого
		double Height() const
		{
			return height_;
		} // высота самой шапки (шарика мороженного)

		double SphereRadius() const
		{
			return sphereRadius_;
		}

		double CenterZ() const
		{
			return centerZ_;
		}

		double MinZ() const override
		{
			return BaseZ();
		} // минимальная высота шапки мороженного (нижняя точка)
		double MaxZ() const override
		{
			return maxZ_;
		} // максимальная высота шапки мороженного (верхняя точка)

		/// <summary>
		/// Создаёт профиль шапки мороженного в виде сферической крышки
		/// </summary>
		/// <param name="baseZ">Высота основания</param>
		/// <param name="baseRadius">Радиус основания</param>
		/// <param name="height">Высота шапки</param>
		SphericalCapProfile(double baseZ, double baseRadius, double height);

		/// <summary>
		/// Возвращает радиус шапки мороженного на высоте z.
		/// </summary>
		/// <param name="z">Высота</param>
		/// <returns>Радиус</returns>
		double GetRadius(double z) const override;
	};
}
