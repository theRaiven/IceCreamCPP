#include "TwoPartProfile.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
using namespace std;

namespace IceCream::Core::Profiles
{
	TwoPartProfile::TwoPartProfile(shared_ptr<const IRadialProfile> firstBody,	shared_ptr<const IRadialProfile> secondBody): first(firstBody), second(secondBody)
	{
		if (!first)
		{
			throw invalid_argument("Первая часть профиля не должна быть nullptr.");
		}

		if (!second)
		{
			throw invalid_argument("Вторая часть профиля не должна быть nullptr.");
		}

		double firstMaxZ = FirstProfile().MaxZ();
		double secondMinZ = SecondProfile().MinZ();

		double heightEPS = 1e-10 * max(1.0, max(abs(firstMaxZ), abs(secondMinZ)));

		if (abs(firstMaxZ - secondMinZ) > heightEPS)
		{
			throw invalid_argument("Первая и вторая части профиля должны соединяться на одной высоте.");
		}

		double firstRadius = FirstProfile().GetRadius(firstMaxZ);
		double secondRadius = SecondProfile().GetRadius(secondMinZ);

		if (!isfinite(firstRadius) || !isfinite(secondRadius))
		{
			throw invalid_argument("Радиусы частей профиля на стыке должны быть конечными числами.");
		}

		double radiusEPS = 1e-10 * max(1.0,	max(abs(firstRadius), abs(secondRadius)));

		if (abs(firstRadius - secondRadius) > radiusEPS)
		{
			throw invalid_argument("Радиусы первой и второй частей профиля на стыке должны совпадать.");
		}

		if (firstRadius <= radiusEPS)
		{
			throw invalid_argument("Радиус профиля на стыке должен быть положительным.");
		}
	}

	double TwoPartProfile::GetRadius(double z) const
	{
		if (!isfinite(z))
		{
			throw out_of_range("Высота должна быть конечным числом.");
		}

		if (z < MinZ() || z > MaxZ())
		{
			throw out_of_range("Высота выходит за пределы составного профиля.");
		}

		if (z <= JoinZ())
		{
			return FirstProfile().GetRadius(z);
		}

		return SecondProfile().GetRadius(z);
	}
}