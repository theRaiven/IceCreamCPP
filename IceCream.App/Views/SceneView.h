#pragma once

#include "Graphics/OrbitCamera.h"
#include "Graphics/MeshRenderer.h"
#include "Graphics/LineRenderer.h"
#include "Graphics/OverlayText.h"
#include "../ViewModels/MainViewModel.h"

namespace IceCream::App::Views
{
    /// <summary>
    /// Трёхмерный вид модели мороженого и секущей плоскости.
    /// </summary>
    class SceneView
    {
    private:
        Graphics::OrbitCamera camera_;
        Graphics::MeshRenderer meshRenderer_;
        Graphics::LineRenderer lineRenderer_;
        Graphics::OverlayText text_;
        float extent_ = 3, minZ_ = 0, maxZ_ = 4.2f, centerX_ = 0, centerY_ = 0;
        void DrawCoordinates(const glm::mat4& mvp) const;
        void DrawOrientation(const glm::mat4& view) const;
        int x_ = 0;
        int y_ = 0;
        int width_ = 1;
        int height_ = 1;

    public:
        void Initialize();
        void SetViewport(int x, int y, int width, int height);
        void UploadMesh(const Core::Geometry::TriangleMesh& mesh);
        void Render(const ViewModels::MainViewModel& viewModel) const;
        bool Contains(int windowX, int windowY, int windowHeight) const;
        void OnMouseDown(int x, int y);
        void OnMouseUp();
        void OnPanDown(int x, int y);
        void OnPanUp();
        void OnMouseMove(int x, int y);
        void OnWheel(int direction);
        void ResetCamera();
        void Destroy();
    };
}
