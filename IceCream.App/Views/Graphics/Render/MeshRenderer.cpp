#include "MeshRenderer.h"
#include "../ShaderSource/ShaderSource.h"

#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace IceCream::App::Views::Graphics
{
	void MeshRenderer::Initialize()
	{
		shaderProgram_ = CreateShaderProgram();
	}

	void MeshRenderer::Upload(const Core::Geometry::TriangleMesh& mesh)
	{
		std::vector<float> vertices;
		vertices.reserve(mesh.Vertices().size() * 3);

		for (const auto& point : mesh.Vertices())
		{
			vertices.push_back(static_cast<float>(point.X));
			vertices.push_back(static_cast<float>(point.Y));
			vertices.push_back(static_cast<float>(point.Z));
		}

		std::vector<GLuint> indices;
		indices.reserve(mesh.Indices().size());

		for (int index : mesh.Indices())
		{
			indices.push_back(static_cast<GLuint>(index));
		}

		indexCount_ = static_cast<GLsizei>(indices.size());

		if (vao_ == 0)
		{
			glGenVertexArrays(1, &vao_);
			glGenBuffers(1, &vbo_);
			glGenBuffers(1, &ebo_);
		}

		glBindVertexArray(vao_);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_DYNAMIC_DRAW);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void MeshRenderer::Draw(const glm::mat4& model, const glm::mat4& view, const glm::mat4& projection, float joinHeight, const glm::vec4& cutPlane, bool ghost, bool transparentPositive, bool hasParts, const LightingSettings& lighting) const 
	{
		if (vao_ == 0 || indexCount_ == 0)
		{
			return;
		}

		glUseProgram(shaderProgram_);
		//свет
		glUniform1i(glGetUniformLocation(shaderProgram_, "uLightType"), static_cast<int>(lighting.Type));

		glUniform1f(glGetUniformLocation(shaderProgram_, "uAmbientIntensity"), lighting.AmbientIntensity);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uEmissionIntensity"), lighting.EmissionIntensity);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uLightIntensity"), lighting.LightIntensity);

		glUniform3f(glGetUniformLocation(shaderProgram_, "uLightPosition"), lighting.Position.x, lighting.Position.y, lighting.Position.z);
		glUniform3f(glGetUniformLocation(shaderProgram_, "uLightDirection"), lighting.Direction.x, lighting.Direction.y, lighting.Direction.z);

		glUniform1f(glGetUniformLocation(shaderProgram_, "uSpotAngle"), lighting.SpotAngle);
		glUniform1f(glGetUniformLocation(shaderProgram_, "uSpotExponent"), lighting.SpotExponent);
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

		glBindVertexArray(0);
		glUseProgram(0);
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

		ebo_ = 0;
		vbo_ = 0;
		vao_ = 0;
		shaderProgram_ = 0;
		indexCount_ = 0;
	}
}
