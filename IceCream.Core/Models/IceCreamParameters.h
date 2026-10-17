#pragma once

namespace IceCream::Core::Models
{
	/// <summary>
	/// Параметры мороженного, состоящие из высоты рожка, радиуса соединения, показателя степени основания и высоты шапки
	/// </summary>
	/// <param name="BaseHeight"> Высота рожка </param>
	/// <param name="JoinRadius"> Радиус соединения </param>
	/// <param name="BaseExponent"> Показатель степени основания </param>
	/// <param name="CapHeight"> Высота шапки </param>
	struct IceCreamParameters
	{
		double BaseHeight;
		double JoinRadius;
		double BaseExponent;
		double CapHeight;
	};
}
