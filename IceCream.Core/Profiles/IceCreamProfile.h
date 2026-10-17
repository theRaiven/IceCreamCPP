#pragma once

#include "SphericalCapProfile.h"
#include <memory>
#include <algorithm>

namespace IceCream::Core::Profiles
{
	/// <summary>
	/// Определяет профиль мороженного, состоящий из профиля рожка и шапки мороженного
	/// </summary>
	class IceCreamProfile : public IRadialProfile
	{
	private:
		std::shared_ptr<const IRadialProfile> baseProfile_;
		SphericalCapProfile capProfile_;

	public:
		const IRadialProfile& BaseProfile() const
		{
			return *baseProfile_;
		} // профиль рожка мороженого
		const SphericalCapProfile& CapProfile() const
		{
			return capProfile_;
		} // профиль шапки мороженного

		double MinZ() const override
		{
			return BaseProfile().MinZ();
		} // минимальная высота профиля (нижняя точка рожка мороженного)
		double MaxZ() const override
		{
			return CapProfile().MaxZ();
		} // максимальная высота профиля (верхняя точка шапки мороженного)
		double JoinZ() const
		{
			return BaseProfile().MaxZ();
		} // точка присоединения шапки к рожку

		/// <summary>
		/// Создаёт профиль мороженного, состоящий из профиля рожка и шапки мороженного
		/// </summary>
		/// <param name="baseProfile">Профиль рожка мороженого</param>
		/// <param name="capProfile">Профиль шапки мороженного</param>
		IceCreamProfile(std::shared_ptr<const IRadialProfile> baseProfile, const SphericalCapProfile& capProfile);

		/// <summary>
		/// Создаёт профиль мороженного, состоящий из профиля рожка и шапки мороженного
		/// </summary>
		/// <param name="baseProfile">Профиль рожка мороженого</param>
		/// <param name="capHeight">Высота шапки мороженного</param>
		IceCreamProfile(std::shared_ptr<const IRadialProfile> baseProfile, double capHeight);

	private:
		/// <summary>
		/// Создаёт профиль шапки мороженного на основе профиля рожка и высоты шапки
		/// </summary>
		/// <param name="baseProfile">Профиль рожка мороженого</param>
		/// <param name="capHeight">Высота шапки мороженного</param>
		/// <returns></returns>
		static SphericalCapProfile CreateCap(const std::shared_ptr<const IRadialProfile>& baseProfile, double capHeight);

	public:
		/// <summary>
		/// Возвращает радиус профиля мороженного на высоте z.
		/// </summary>
		/// <param name="z">Высота</param>
		/// <returns>Радиус</returns>
		double GetRadius(double z) const override;
	};
}
