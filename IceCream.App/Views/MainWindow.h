#pragma once
#include <GL/glew.h>
#include "../../IceCream.Core/Geometry/TriangleMesh.h"

namespace IceCream::App::Views
{
    /// <summary>
    /// Главное окно приложения.
    /// </summary>
    class MainWindow
    {
    private:
        inline static GLuint shaderProgram_ = 0;
        inline static GLuint vao_ = 0;
        inline static GLuint vbo_ = 0;

        inline static GLuint ebo_ = 0;
        inline static GLsizei indexCount_ = 0;

        inline static float aspectRatio_ = 1.0f;
        
        // камера-мэн
        inline static float cameraYaw_ = -58.0f;
        inline static float cameraPitch_ = 23.0f;
        inline static float cameraDistance_ = 10.0f;

        inline static bool isRotating_ = false;
        inline static int lastMouseX_ = 0;
        inline static int lastMouseY_ = 0;

        /// <summary>
        /// Обрабатывает нажатие и отпускание кнопки мыши.
        /// </summary>
        static void Mouse(int button, int state, int x, int y);

        /// <summary>
        /// Вращает камеру при перемещении мыши с зажатой кнопкой.
        /// </summary>
        static void MouseMotion(int x, int y);

        /// <summary>
        /// Загружает вершины и индексы сетки в OpenGL.
        /// </summary>
        static void InitializeBuffers(const Core::Geometry::TriangleMesh& mesh);

        /// <summary>
        /// Освобождает графические ресурсы перед закрытием окна.
        /// </summary>
        static void Close();
    public:
        /// <summary>
        /// Создаёт окно и запускает обработку событий.
        /// </summary>
        int Run(int argc, char** argv);

    private:
        /// <summary>
        /// Отображает кадр.
        /// </summary>
        static void Display();

        /// <summary>
        /// Обновляет область рисования при изменении размера окна.
        /// </summary>
        static void Reshape(int width, int height);


    };
}