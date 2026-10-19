#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

namespace IceCream::App::Views::Graphics
{
	/// <summary>
	/// Рисует ломаные линии: контур сечения и рамку секущей плоскости.
	/// </summary>
	class LineRenderer
	{
	private:
		GLuint shaderProgram_ = 0;
		GLuint vao_ = 0;
		GLuint vbo_ = 0;

	public:
		void Initialize();
		void Draw(const std::vector<float>& vertices, const glm::mat4& mvp, const glm::vec3& color, GLenum mode) const;
		void Destroy();
	};
}
