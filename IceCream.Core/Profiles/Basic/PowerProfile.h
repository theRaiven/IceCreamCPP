#pragma once

#include "IRadialProfile.h"
#include <cmath>
#include <stdexcept>

namespace IceCream::Core::Profiles
{
	/// <summary>
	/// Определяет степенной профиль рожка мороженого
	/// </summary>
	// в дальнейшем оно будет использоваться для формулы из интернетов r(z) = R(z/H)^n
	// где R - радиус верхнего края TopRadius, H - высота рожка Height, n - показатель степени Exponent
	class PowerProfile : public IRadialProfile
	{
	private:
		double height_;
		double topRadius_;
		double exponent_;

	public:
		double Height() const
		{
			return height_;
		}

		double TopRadius() const
		{
			return topRadius_;
		}

		double Exponent() const
		{
			return exponent_;
		}

		double MinZ() const override
		{
			return 0;
		}

		double MaxZ() const override
		{
			return Height();
		}

		/// <summary>
		/// Создаёт степенной профиль рожка
		/// </summary>
		/// <param name="height">Высота рожка.</param>
		/// <param name="topRadius">Радиус верхнего края.</param>
		/// <param name="exponent">
		/// Показатель степени: 1 — конус, 0.5 — параболоид, чем меньше показатель, тем более выпуклый профиль, чем боьлше - тем более похож на воронку
		/// </param>
		PowerProfile(double height, double topRadius, double exponent);

		/// <summary>
		/// Возвращает радиус рожка на высоте z.
		/// </summary>
		double GetRadius(double z) const override;
	};
}
