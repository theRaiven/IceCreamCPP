#pragma once

#include <GL/glew.h>
#include "../../../../IceCream.Core/Geometry/TriangleMesh.h"
#include "../OrbitCamera/LightingSettings.h"
#include <glm/glm.hpp>

namespace IceCream::App::Views::Graphics
{
	/// <summary>
	/// Загружает треугольную сетку в OpenGL и рисует её шейдером.
	/// </summary>
	class MeshRenderer
	{
	private:
		GLuint shaderProgram_ = 0;
		GLuint vao_ = 0;
		GLuint vbo_ = 0;
		GLuint ebo_ = 0;
		GLsizei indexCount_ = 0;
		GLuint shadowShaderProgram_ = 0;
		GLuint shadowFramebuffer_ = 0;
		GLuint shadowDepthTexture_ = 0;

		static constexpr int ShadowMapSize = 2048;

		static glm::mat4 CreateLightSpaceMatrix(const LightingSettings& lighting);

	public:
		void Initialize();
		void Upload(const Core::Geometry::TriangleMesh& mesh);
		void Draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, float joinHeight, const glm::vec4& cutPlane, bool ghost, bool transparentPositive, bool hasParts, const LightingSettings& lighting) const;
		void Destroy();
		void RenderShadowMap(const LightingSettings& lighting) const;
	};
}
