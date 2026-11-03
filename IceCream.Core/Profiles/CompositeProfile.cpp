#include "CompositeProfile.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace std;

namespace IceCream::Core::Profiles
{
	CompositeProfile::CompositeProfile(vector<shared_ptr<const IRadialProfile>> profiles) : profiles(move(profiles))
	{
		if (this->profiles.empty())
		{
			throw invalid_argument("Состовной профиль должен содержать хотя бы одну часть.");
		}

		for (const auto& profile : this->profiles)
		{
			if (!profile)
			{
				throw invalid_argument("Часть составного профиля не должна быть пустой.");
			}

			if (!isfinite(profile->MinZ()) || !isfinite(profile->MaxZ()) || profile->MinZ() >= profile->MaxZ())
			{
				throw invalid_argument("Части профиля должны иметь корректный диапазон высот.");
			}
		}

		for (size_t i = 0; i + 1 < this->profiles.size(); ++i)
		{
			const auto& first = *this->profiles[i]; // неудачный нейминг
			const auto& second = *this->profiles[i + 1];

			double firstMaxZ = first.MaxZ();
			double secondMinZ = second.MinZ();

			double heigthEPS = 1e-10 * max(1.0, max(abs(firstMaxZ), abs(secondMinZ))); // на случай, если профили очень маленькие или очень большие, то относительная погрешность

			if(abs(firstMaxZ - secondMinZ) > heigthEPS)
			{
				throw invalid_argument("Соседние части составного профиля должны быть соединены по одной высоте.");
			}

			double firstRadius = first.GetRadius(firstMaxZ);
			double secondRadius = second.GetRadius(secondMinZ);


			if (!isfinite(firstRadius) || !isfinite(secondRadius))
			{
				throw invalid_argument("Радиусы частей профиля на стыке должны быть конечными.");
			}

			double radiusEPS = 1e-10 * max(1.0, max(abs(firstRadius), abs(secondRadius))); // тоже что и с heightEPS, но для радиусов

			if (abs(firstRadius - secondRadius) > radiusEPS)
			{
				throw invalid_argument("Радиусы соседних частей профиля на стыке должны совпадать.");
			}
			if (firstRadius <= radiusEPS)
			{
				throw invalid_argument("Радиус профиля на стыке должен быть положительным.");
			}
		}
	}

	const IRadialProfile& CompositeProfile::Profile(size_t index) const
	{
		if (index >= profiles.size())
		{
			throw out_of_range("Индекс части профиля выходит за границы.");
		}

		return *profiles[index];
	}

	double CompositeProfile::MinZ() const
	{
		return profiles.front()->MinZ();
	}

	double CompositeProfile::MaxZ() const
	{
		return profiles.back()->MaxZ();
	}

	double CompositeProfile::GetRadius(double z) const
	{
		if (!isfinite(z))
		{
			throw out_of_range("Высота должна быть конечным числом.");
		}

		if (z < MinZ() || z > MaxZ())
		{
			throw out_of_range("Высота выходит за пределы составного профиля.");
		}

		for (size_t i = 0; i < profiles.size(); i++)
		{
			const auto& profile = *profiles[i];

			if (z <= profile.MaxZ() || i + 1 == profiles.size())
			{
				return profile.GetRadius(z);
			}
		}

		throw runtime_error("Не удалось определить часть составного профиля.");
	}
}