#pragma once

#include "IRadialProfile.h"

#include <memory>

namespace IceCream::Core::Profiles
{
	/// <summary>
	/// Определяет профиль тела, состоящий из двух радиальных профилей,
	/// соединённых в одной плоскости по координате Z
	/// </summary>
	class TwoPartProfile : public IRadialProfile
	{
	private: 
		std::shared_ptr<const IRadialProfile> first;
		std::shared_ptr<const IRadialProfile> second;

	public:
		/// <summary>
		/// Возвращает первую часть профиля
		/// </summary>
		const IRadialProfile& FirstProfile() const
		{
			return *first;
		}

		/// <summary>
		/// Возвращает вторую часть профиля
		/// </summary>
		const IRadialProfile& SecondProfile() const
		{
			return *second;
		}

		/// <summary>
		/// Минимальная высота всего профиля
		/// </summary>
		double MinZ() const override
		{
			return FirstProfile().MinZ();
		}

		/// <summary>
		/// Максимальная высота всего профиля
		/// </summary>
		double MaxZ() const override
		{
			return SecondProfile().MaxZ();
		}

		/// <summary>
		/// Высота плоскости соединения двух частей
		/// </summary>
		double JoinZ() const
		{
			return FirstProfile().MaxZ();
		}

		/// <summary>
		/// Создаёт профиль тела из двух радиальных профилей
		/// </summary>
		/// <param name="first">Первая часть профиля</param>
		/// <param name="second">Вторая часть профиля</param>
		TwoPartProfile(std::shared_ptr<const IRadialProfile> first, std::shared_ptr<const IRadialProfile> second);

		/// <summary>
		/// Возвращает радиус составного профиля на высоте z
		/// </summary>
		/// <param name="z">Координата Z</param>
		/// <returns>Радиус профиля</returns>
		double GetRadius(double z) const override;

	};
}