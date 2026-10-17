#include "MainWindow.h"
#include "Graphics/ShaderSource.h"
#include "../ViewModels/MainViewModel.h"

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

namespace IceCream::App::Views
{
    int MainWindow::Run(int argc, char** argv)
    {
        constexpr int InitialWidth = 1280;
        constexpr int InitialHeight = 720;

        ViewModels::MainViewModel viewModel;

        const auto& mesh = viewModel.Body().Mesh();

        std::cout << "Vertices: " << mesh.Vertices().size() << '\n';
        std::cout << "Triangles: " << mesh.TriangleCount() << '\n';

        glutInit(&argc, argv);

        // OpenGL 3.3 Core.
        glutInitContextVersion(3, 3);
        glutInitContextProfile(GLUT_CORE_PROFILE);

        // Цветовой буфер и двойная буферизация.
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

        glutInitWindowSize(InitialWidth, InitialHeight);
        glutCreateWindow("IceCream");

        // При закрытии окна возвращаемся из главного цикла.
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

        // Загружаем функции OpenGL после создания контекста.
        glewExperimental = GL_TRUE;
        GLenum glewResult = glewInit();

        glEnable(GL_DEPTH_TEST);

        if (glewResult != GLEW_OK)
        {
            std::cerr << "GLEW error: " << glewGetErrorString(glewResult) << '\n';

            glutDestroyWindow(glutGetWindow());

            return 1;
        }

        shaderProgram_ = CreateShaderProgram();

        glUseProgram(shaderProgram_);

        const GLint joinHeightLocation = glGetUniformLocation(shaderProgram_, "uJoinHeight");
        const float joinHeight = static_cast<float>(viewModel.Body().Parameters().BaseHeight);

        glUniform1f(joinHeightLocation, joinHeight);

        glUseProgram(0);

        InitializeBuffers(mesh);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glClearColor(0.12f, 0.15f, 0.20f, 1.0f);

        // Регистрируем обработчики.
        glutDisplayFunc(Display);
        glutReshapeFunc(Reshape);
        glutMouseFunc(Mouse);
        glutMotionFunc(MouseMotion);
        glutCloseFunc(Close);

        Reshape(InitialWidth, InitialHeight);
        glutMainLoop();
        
        return 0;
    }
    /// <summary>
    /// Отображает кадр.
    /// </summary>
    void MainWindow::Display()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Перемещаем центр тела в начало координат.
        // При высоте рожка 3 и высоте шапки 1 середина находится на Z = 2.
        const glm::mat4 model = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(0.0f, 0.0f, -2.0f));

        // Камера смотрит на начало координат. Ось Z направлена вверх.
        const float yaw = glm::radians(cameraYaw_);
        const float pitch = glm::radians(cameraPitch_);

        const glm::vec3 cameraPosition(
            cameraDistance_ * glm::cos(pitch) * glm::cos(yaw),
            cameraDistance_ * glm::cos(pitch) * glm::sin(yaw),
            cameraDistance_ * glm::sin(pitch));

        const glm::mat4 view = glm::lookAt(
            cameraPosition,
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f));

        // Перспектива: удалённые части выглядят меньше.
        const glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            aspectRatio_,
            0.1f,
            100.0f);

        glUseProgram(shaderProgram_);

        const GLint modelLocation = glGetUniformLocation(shaderProgram_, "uModel");
        const GLint viewLocation = glGetUniformLocation(shaderProgram_, "uView");
        const GLint projectionLocation = glGetUniformLocation(shaderProgram_, "uProjection");

        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(vao_);

        glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);

        glBindVertexArray(0);
        glUseProgram(0);

        glutSwapBuffers();
    }
    /// <summary>
    /// Обновляет область рисования при изменении размера окна.
    /// </summary>
    void MainWindow::Reshape(int width, int height)
    {
        if (width < 1)
        {
            width = 1;
        }

        if (height < 1)
        {
            height = 1;
        }

        aspectRatio_ = static_cast<float>(width) / height;

        glViewport(0, 0, width, height);
        glutPostRedisplay();
    }

    void MainWindow::InitializeBuffers(const Core::Geometry::TriangleMesh& mesh)
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

        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);

        glBindVertexArray(vao_);

        // Загружаем координаты вершин.
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        // Каждая вершина содержит три числа: X, Y, Z.
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);

        glEnableVertexAttribArray(0);

        // Загружаем номера вершин, образующих треугольники.
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void MainWindow::Close()
    {
        glDeleteBuffers(1, &ebo_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
        glDeleteProgram(shaderProgram_);

        ebo_ = 0;
        vbo_ = 0;
        vao_ = 0;
        shaderProgram_ = 0;
        indexCount_ = 0;
    }

    void MainWindow::Mouse(int button, int state, int x, int y)
    {
        if (button == GLUT_LEFT_BUTTON)
        {
            isRotating_ = (state == GLUT_DOWN);

            lastMouseX_ = x;
            lastMouseY_ = y;
        }
    }

    void MainWindow::MouseMotion(int x, int y)
    {
        if (!isRotating_)
        {
            return;
        }

        const int deltaX = x - lastMouseX_;
        const int deltaY = y - lastMouseY_;

        lastMouseX_ = x;
        lastMouseY_ = y;

        constexpr float Sensitivity = 0.4f;

        cameraYaw_ -= deltaX * Sensitivity;
        cameraPitch_ += deltaY * Sensitivity;

        if (cameraPitch_ > 89.0f)
        {
            cameraPitch_ = 89.0f;
        }

        if (cameraPitch_ < -89.0f)
        {
            cameraPitch_ = -89.0f;
        }

        glutPostRedisplay();
    }
}