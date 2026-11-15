#pragma once

#include "../Meshing/IntegralBodyBuilder.h"

#include <array>
#include <string>
#include <vector>
using namespace std;

namespace IceCream::Core::Services::SaveData
{
	enum class FigureType
	{
		Explicit = 0,
		Implicit = 1,
		Inequalities = 2,
	};

	struct FigureData
	{
		int Version = 1;
		FigureType Type = FigureType::Explicit;

		// явнре задание и интригалы
		vector<IntegralPart> Parts;

		// неявное задание
		string Equation;

		// ограничения и неравенства
		string Constraints;

		// область построеия 
		array<double, 3> Min = { -4,-4, -4 };
		array<double, 3> Max = { 4, 4, 4 };

		// разрешение сетки
		array<int, 3> Cells = { 100, 100, 100 };

		// цвет поверхности
		array<float, 4> Color = { 0.32f, 0.70f, 0.88f, 1.0f };

		// Автоматическое определение границ
		bool AutoBounds = false;
	};
}