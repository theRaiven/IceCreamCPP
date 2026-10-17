#pragma once

#include "../ViewModels/MainViewModel.h"
#include "SceneView.h"
#include "SectionView.h"
#include "ParametersPanel.h"

namespace IceCream::App::Views
{
    /// <summary>
    /// Главное окно GLUT: раскладка 3D-сцены, сечения и панели параметров.
    /// </summary>
    class MainWindow
    {
    private:
        inline static MainWindow* instance_ = nullptr;

        ViewModels::MainViewModel viewModel_;
        SceneView sceneView_;
        SectionView sectionView_;
        ParametersPanel parametersPanel_;
        int windowWidth_ = 1440;
        int windowHeight_ = 820;

        void UpdateLayout();
        void ReloadMesh();
        void ReloadMeshIfChanged();
        std::uint64_t uploadedRevision_ = 0;

        static void Display();
        static void Reshape(int width, int height);
        static void Mouse(int button, int state, int x, int y);
        static void MouseMotion(int x, int y);
        static void MouseWheel(int wheel, int direction, int x, int y);
        static void Keyboard(unsigned char key, int x, int y);
        static void Special(int key, int x, int y);
        static void Close();

    public:
        /// <summary>
        /// Создаёт окно и запускает обработку событий.
        /// </summary>
        int Run(int argc, char** argv);
    };
}
