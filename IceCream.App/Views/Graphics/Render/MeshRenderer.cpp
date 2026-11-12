#include "MeshRenderer.h"
#include "../ShaderSource/ShaderSource.h"
#include "MeshNormals.h"

#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <array>
#include <limits>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace IceCream::App::Views::Graphics
{
	void MeshRenderer::Initialize()
	{
		shaderProgram_ = CreateShaderProgram();

		shadowShaderProgram_ = CreateShadowShaderProgram();

		glGenFramebuffers(1, &shadowFramebuffer_);

		glGenTextures(1, &shadowDepthTexture_);

		glBindTexture(GL_TEXTURE_2D, shadowDepthTexture_);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, ShadowMapSize, ShadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

		float borderColor[] =
		{
			1.0f,
			1.0f,
			1.0f,
			1.0f
		};

		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer_);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTexture_, 0);

		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			throw std::runtime_error("Не удалось создать буфер теней.");
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void MeshRenderer::Upload(const Core::Geometry::TriangleMesh& mesh, const std::vector<std::array<float, 4>>& colors)
	{
		hasColors_ = !colors.empty();

		if (hasColors_ && colors.size() != mesh.TriangleCount())
		{
			throw std::invalid_argument("Количество цветов не соответствует количеству треугольников.");
		}

		const auto& points = mesh.Vertices();
		const auto& sourceIndices = mesh.Indices();
		const auto cornerNormals = CalculateCornerNormals(mesh, colors);

		if (sourceIndices.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max()))
		{
			throw std::length_error("Слишком много индексов для отрисовки сетки.");
		}

		// Для каждого угла треугольника сохраняем собственную нормаль.
		// На гладких рёбрах они совпадают, на острых — могут различаться.
		// Формат вершины: позиция (3), цвет (4), нормаль (3).
		std::vector<float> vertices;
		std::vector<GLuint> indices;
		vertices.reserve(sourceIndices.size() * 10);
		indices.reserve(sourceIndices.size());

		for (size_t i = 0; i < sourceIndices.size(); ++i)
		{
			const auto& p = points[sourceIndices[i]];
			const auto& normal = cornerNormals[i];
			const std::array<float, 4> color = hasColors_ ? colors[i / 3] : std::array<float, 4>{ 1, 1, 1, 1 };

			vertices.insert(vertices.end(), {
				static_cast<float>(p.X), static_cast<float>(p.Y), static_cast<float>(p.Z),
				color[0], color[1], color[2], color[3],
				normal.x, normal.y, normal.z
				});
			indices.push_back(static_cast<GLuint>(i));
		}

		indexCount_ = static_cast<GLsizei>(indices.size());

		if (!vao_)
		{
			glGenVertexArrays(1, &vao_);
			glGenBuffers(1, &vbo_);
			glGenBuffers(1, &ebo_);
		}

		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 10 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 10 * sizeof(float), reinterpret_cast<void*>(7 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void MeshRenderer::Draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, float joinHeight, const glm::vec4& cutPlane, bool ghost, bool transparentPositive, bool hasParts, const LightingSettings& lighting, const ViewModels::AppearanceSettings& appearance) const
	{
		if (vao_ == 0 || indexCount_ == 0)
		{
			return;
		}

		glUseProgram(shaderProgram_);
		glUniform1i(glGetUniformLocation(shaderProgram_, "uHasColors"), hasColors_ ? 1 : 0);
		glUniform4fv(glGetUniformLocation(shaderProgram_, "uBodyColor"), 1, glm::value_ptr(appearance.Color(ViewModels::ColorRole::Body)));
		glUniform4fv(glGetUniformLocation(shaderProgram_, "uConeColor"), 1, glm::value_ptr(appearance.Color(ViewModels::ColorRole::Cone)));
		glUniform4fv(glGetUniformLocation(shaderProgram_, "uCapColor"), 1, glm::value_ptr(appearance.Color(ViewModels::ColorRole::Cap)));
		//свет
		glUniform1i(glGetUniformLocation(shaderProgram_, "uLightType"), static_cast<int>(lighting.Type));

		glUniform1f(glGetUniformLocation(shaderProgram_, "uAmbientIntensity"), lighting.AmbientIntensity);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uEmissionIntensity"), lighting.EmissionIntensity);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uLightIntensity"), lighting.LightIntensity);

		glUniform3f(glGetUniformLocation(shaderProgram_, "uLightPosition"), lighting.Position.x, lighting.Position.y, lighting.Position.z);
		glUniform3f(glGetUniformLocation(shaderProgram_, "uLightDirection"), lighting.Direction.x, lighting.Direction.y, lighting.Direction.z);

		glUniform1f(glGetUniformLocation(shaderProgram_, "uSpotAngle"), lighting.SpotAngle);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uSpotExponent"), lighting.SpotExponent);

		bool useShadow = lighting.Type == LightType::Spot && glm::length(lighting.Direction) >= 0.000001f;

		glm::mat4 lightSpaceMatrix = CreateLightSpaceMatrix(lighting);

		glUniform1i(glGetUniformLocation(shaderProgram_, "uUseShadow"), useShadow ? 1 : 0);

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		glActiveTexture(GL_TEXTURE0);

		glBindTexture(GL_TEXTURE_2D, shadowDepthTexture_);

		glUniform1i(glGetUniformLocation(shaderProgram_, "uShadowMap"), 0);
		// == 

		glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uView"), 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniform1f(glGetUniformLocation(shaderProgram_, "uJoinHeight"), joinHeight);
		glUniform1i(glGetUniformLocation(shaderProgram_, "uHasParts"), hasParts ? 1 : 0);

		glUniform4fv(glGetUniformLocation(shaderProgram_, "uCutPlane"), 1, glm::value_ptr(cutPlane));
		glUniform1i(glGetUniformLocation(shaderProgram_, "uGhostPass"), ghost ? 1 : 0);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uTransparentSide"), transparentPositive ? 1.0f : -1.0f);
		glBindVertexArray(vao_);
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		if (ghost)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			// Сначала рисуем задние грани прозрачной части, затем передние.
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
			glCullFace(GL_BACK);
			glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}
		else
		{
			glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
		}

		glDisable(GL_BLEND);
		glBindVertexArray(0);
		glUseProgram(0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void MeshRenderer::Destroy()
	{
		if (ebo_ != 0)
		{
			glDeleteBuffers(1, &ebo_);
		}

		if (vbo_ != 0)
		{
			glDeleteBuffers(1, &vbo_);
		}

		if (vao_ != 0)
		{
			glDeleteVertexArrays(1, &vao_);
		}

		if (shaderProgram_ != 0)
		{
			glDeleteProgram(shaderProgram_);
		}

		if (shadowDepthTexture_ != 0)
		{
			glDeleteTextures(1, &shadowDepthTexture_);
		}

		if (shadowFramebuffer_ != 0)
		{
			glDeleteFramebuffers(1, &shadowFramebuffer_);
		}

		if (shadowShaderProgram_ != 0)
		{
			glDeleteProgram(shadowShaderProgram_);
		}
		ebo_ = 0;
		vbo_ = 0;
		vao_ = 0;
		shaderProgram_ = 0;
		indexCount_ = 0;
		shadowDepthTexture_ = 0;
		shadowFramebuffer_ = 0;
		shadowShaderProgram_ = 0;
	}

	glm::mat4 MeshRenderer::CreateLightSpaceMatrix(const LightingSettings& lighting)
	{
		glm::vec3 direction = lighting.Direction;

		if (glm::length(direction) < 0.000001f)
		{
			return glm::mat4(1.0f);
		}

		direction = glm::normalize(direction);

		glm::vec3 up(0.0f, 0.0f, 1.0f);

		if (std::abs(glm::dot(direction, up)) > 0.99f)
		{
			up = glm::vec3(0.0f, 1.0f, 0.0f);
		}

		glm::mat4 lightView = glm::lookAt(lighting.Position, lighting.Position + direction, up);

		float angle = glm::clamp(lighting.SpotAngle, 0.1f, 89.0f);

		glm::mat4 lightProjection = glm::perspective(glm::radians(angle * 2.0f), 1.0f, 0.05f, 250.0f);

		return lightProjection * lightView;
	}

	void MeshRenderer::RenderShadowMap(const LightingSettings& lighting) const
	{
		if (lighting.Type != LightType::Spot || vao_ == 0 || indexCount_ == 0 || glm::length(lighting.Direction) < 0.000001f)
		{
			return;
		}

		GLint oldFramebuffer;
		GLint oldViewport[4];

		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldFramebuffer);

		glGetIntegerv(GL_VIEWPORT, oldViewport);

		glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer_);

		glViewport(0, 0, ShadowMapSize, ShadowMapSize);

		glClear(GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		glUseProgram(shadowShaderProgram_);

		glm::mat4 lightSpaceMatrix = CreateLightSpaceMatrix(lighting);

		glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram_, "uLightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		glBindVertexArray(vao_);

		glEnable(GL_POLYGON_OFFSET_FILL);

		glPolygonOffset(2.0f, 4.0f);

		glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

		glDisable(GL_POLYGON_OFFSET_FILL);

		glDisable(GL_BLEND);
		glBindVertexArray(0);
		glUseProgram(0);

		glBindFramebuffer(GL_FRAMEBUFFER, oldFramebuffer);

		glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
	}
}
