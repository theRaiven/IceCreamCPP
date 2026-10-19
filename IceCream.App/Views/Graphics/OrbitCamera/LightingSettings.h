#pragma once

#include <glm/glm.hpp>

namespace IceCream::App::Views::Graphics
{
	enum class LightType
	{
		Directional,
		Point,
		Spot
	};

	struct LightingSettings
	{
		LightType Type = LightType::Directional;

		// Интенсивность фоновой подсветки.
		float AmbientIntensity = 0.25f;

		// Собственная яркость материала.
		float EmissionIntensity = 0.0f;

		// Интенсивность выбранного источника.
		float LightIntensity = 0.75f;

		// Положение точечного источника или прожектора.
		glm::vec3 Position = glm::vec3(-3.0f, -4.0f, 6.0f);

		// Направление распространения света: от источника к сцене.
		glm::vec3 Direction = glm::vec3(3.0f, 4.0f, -6.0f);

		// Угол между осью прожектора и границей конуса, в градусах.
		float SpotAngle = 30.0f;

		// Чем больше значение, тем сильнее свет сосредоточен у оси.
		float SpotExponent = 8.0f;
	};
}