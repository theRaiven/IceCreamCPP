#include "ShaderSource.h"

#include <iostream>
#include <cstdlib>

// Шейдеры
const char* VertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out float Height;
out vec3 WorldPosition;
out vec3 ObjectPosition;

void main()
{
	ObjectPosition = aPosition;
	Height = aPosition.z;
	vec4 position = uModel * vec4(aPosition, 1.0);

	WorldPosition = position.xyz;
	gl_Position = uProjection * uView * position;
}
)";
const char* FragmentShaderSource = R"(
#version 330 core

in vec3 WorldPosition;
in vec3 ObjectPosition;
in float Height;

uniform float uJoinHeight;
uniform int uHasParts;

uniform vec4 uCutPlane;
uniform int uGhostPass;
uniform float uTransparentSide;

uniform int uLightType;

uniform float uAmbientIntensity;
uniform float uEmissionIntensity;
uniform float uLightIntensity;

uniform vec3 uLightPosition;
uniform vec3 uLightDirection;

uniform float uSpotAngle;
uniform float uSpotExponent;

out vec4 FragColor;

void main()
{
	// Отсекаем часть фигуры для текущего прохода.
	float side = dot(uCutPlane, vec4(ObjectPosition, 1.0)) * uTransparentSide;

	if (uGhostPass == 0 && side > 0.000001)
	{
		discard;
	}

	if (uGhostPass == 1 && side <= 0.000001)
	{
		discard;
	}

	// Поверхность и источник света задаём в координатах модели.
	vec3 tangentX = dFdx(ObjectPosition);
	vec3 tangentY = dFdy(ObjectPosition);
	vec3 normalVector = cross(tangentX, tangentY);
	vec3 normal = normalVector / max(length(normalVector), 0.000001);

	vec3 lightDirection = vec3(0.0);
	float lightFactor = 1.0;

	if (uLightType == 0)
	{
		// Направленный источник.
		// uLightDirection направлен от источника, а нам нужен вектор к нему.
		lightDirection = -uLightDirection / max(length(uLightDirection), 0.000001);
	}
	else
	{
		// Точечный источник или прожектор.
		vec3 toLight = uLightPosition - ObjectPosition;
		float distanceToLight = length(toLight);

		lightDirection = toLight / max(distanceToLight, 0.000001);

		if (uLightType == 2)
		{
			// Проверяем, находится ли точка внутри конуса прожектора.
			vec3 spotDirection = uLightDirection / max(length(uLightDirection), 0.000001);
			float angleCos = dot(-lightDirection, spotDirection);
			float cutoffCos = cos(radians(clamp(uSpotAngle, 0.1, 89.9)));

			if (angleCos < cutoffCos)
			{
				lightFactor = 0.0;
			}
			else
			{
				// Чем больше показатель, тем уже яркая область у оси.
				lightFactor = pow(max(angleCos, 0.0), max(uSpotExponent, 0.0));
			}
		}
	}

	// Освещённость зависит от угла между нормалью и направлением к свету.
	float diffuse = max(dot(normal, lightDirection), 0.0);
	float brightness = uAmbientIntensity + uLightIntensity * diffuse * lightFactor;

	vec3 baseColor;

	if (uHasParts == 0)
	{
		baseColor = vec3(0.32, 0.70, 0.88);
	}
	else if (Height >= uJoinHeight)
	{
		// Фисташковое мороженое.
		baseColor = vec3(0.60, 0.85, 0.35);
	}
	else
	{
		// Вафельный рожок.
		baseColor = vec3(0.76, 0.5, 0.3);
	}

	// Собственная яркость материала не зависит от внешнего света.
	vec3 color = baseColor * brightness + baseColor * uEmissionIntensity;

	float alpha = 1.0;

	if (uGhostPass == 1)
	{
		alpha = 0.16;
	}

	FragColor = vec4(color, alpha);
}
)";

/// <summary>
/// Компилирует шейдер
/// </summary>
/// <param name="shaderType">Тип шейдера</param>
/// <param name="source">Источник кода шейдера</param>
/// <returns>Идентификатор скомпилированного шейдера</returns>
GLuint CompileShader(GLenum shaderType, const char* source)
{
	GLuint shader = glCreateShader(shaderType);

	glShaderSource(shader, 1, &source, nullptr);
	glCompileShader(shader);

	GLint success;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		char log[1024];

		glGetShaderInfoLog(shader, sizeof(log), nullptr, log);

		std::cerr << "Ошибка компиляции шейдера: " << log << std::endl;

		std::exit(EXIT_FAILURE);
	}

	return shader;
}

/// <summary>
/// Создает программу шейдеров
/// </summary>
/// <returns>Идентификатор программы шейдеров</returns>
GLuint CreateShaderProgram()
{
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, VertexShaderSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, FragmentShaderSource);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint success;

	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success)
	{
		char log[1024];

		glGetProgramInfoLog(program, sizeof(log), nullptr, log);

		std::cerr << "Ошибка линковки программы: " << log << std::endl;

		std::exit(EXIT_FAILURE);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

const char* LineVertexShaderSource = R"(
#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 uMvp;

void main()
{
	gl_Position = uMvp * vec4(aPosition, 1.0);
}
)";

const char* LineFragmentShaderSource = R"(
#version 330 core

uniform vec3 uColor;

out vec4 FragColor;

void main()
{
	FragColor = vec4(uColor, 1.0);
}
)";

GLuint CreateLineShaderProgram()
{
	GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, LineVertexShaderSource);
	GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, LineFragmentShaderSource);

	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);

	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success)
	{
		char log[1024];
		glGetProgramInfoLog(program, sizeof(log), nullptr, log);
		std::cerr << "Ошибка линковки программы линий: " << log << std::endl;
		std::exit(EXIT_FAILURE);
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}
