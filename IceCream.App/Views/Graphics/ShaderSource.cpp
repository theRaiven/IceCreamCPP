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

in float Height;
uniform float uJoinHeight;
uniform int uHasParts;
in vec3 ObjectPosition;
uniform vec4 uCutPlane;
uniform int uGhostPass;
uniform float uTransparentSide;

out vec4 FragColor;

void main()
{
    float side = dot(uCutPlane, vec4(ObjectPosition, 1.0)) * uTransparentSide;
    if (uGhostPass == 0 && side > 0.000001) discard;
    if (uGhostPass == 1 && side <= 0.000001) discard;
    // Два направления вдоль поверхности треугольника.
    vec3 tangentX = dFdx(WorldPosition);
    vec3 tangentY = dFdy(WorldPosition);

    // Перпендикуляр к поверхности, направленный к видимой стороне.
    vec3 normal = normalize(cross(tangentX, tangentY));

    // Направление от поверхности к источнику света.
    vec3 lightDirection = normalize(vec3(-3.0, -4.0, 6.0));

    // Чем сильнее поверхность повёрнута к свету, тем она ярче.
    float diffuse = max(dot(normal, lightDirection), 0.0);

    // Небольшая постоянная подсветка сохраняет детали тёмной стороны.
    float brightness = 0.25 + 0.75 * diffuse;

    vec3 baseColor;

    if (uHasParts == 0) baseColor = vec3(0.32, 0.70, 0.88);
    else if (Height >= uJoinHeight)
    {
        // Фисташковое мороженое.
        baseColor = vec3(0.60, 0.85, 0.35);
    }
    else
    {
        // Золотистый вафельный рожок.
        baseColor = vec3(0.76, 0.5, 0.3);
    }

    FragColor = vec4(baseColor * brightness, uGhostPass == 1 ? 0.16 : 1.0);
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