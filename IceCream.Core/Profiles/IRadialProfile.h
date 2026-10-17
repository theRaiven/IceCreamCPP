#pragma once

namespace IceCream::Core::Profiles
{
	class IRadialProfile
	{
	public:
		virtual ~IRadialProfile() = default;

		/// <summary>
		/// Минимальное значение Z
		/// </summary>
		virtual double MinZ() const = 0;

		/// <summary>
		/// Максимальное значение Z
		/// </summary>
		virtual double MaxZ() const = 0;

		/// <summary>
		/// Возвращает радиус профиля в зависимости от координаты Z
		/// </summary>
		/// <param name="z">Координата Z</param>
		/// <returns>Радиус профиля</returns>
		virtual double GetRadius(double z) const = 0;
	};
}
