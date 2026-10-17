#include "LineRenderer.h"
#include "ShaderSource.h"

#include <glm/gtc/type_ptr.hpp>

namespace IceCream::App::Views::Graphics
{
    void LineRenderer::Initialize()
    {
        shaderProgram_ = CreateLineShaderProgram();

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void LineRenderer::Draw(const std::vector<float>& vertices, const glm::mat4& mvp, const glm::vec3& color, GLenum mode) const
    {
        if (vertices.size() < (mode == GL_POINTS ? 3u : 6u))
        {
            return;
        }

        glUseProgram(shaderProgram_);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uMvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3fv(glGetUniformLocation(shaderProgram_, "uColor"), 1, glm::value_ptr(color));

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(mode, 0, static_cast<GLsizei>(vertices.size() / 3));
        glBindVertexArray(0);
        glUseProgram(0);
    }

    void LineRenderer::Destroy()
    {
        if (vbo_ != 0) glDeleteBuffers(1, &vbo_);
        if (vao_ != 0) glDeleteVertexArrays(1, &vao_);
        if (shaderProgram_ != 0) glDeleteProgram(shaderProgram_);

        vbo_ = 0;
        vao_ = 0;
        shaderProgram_ = 0;
    }
}
