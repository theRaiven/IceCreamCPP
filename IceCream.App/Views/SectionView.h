#pragma once
#include "Graphics/OverlayText.h"

#include "Graphics/LineRenderer.h"
#include "../ViewModels/MainViewModel.h"

namespace IceCream::App::Views
{
    /// <summary>
    /// Двумерный вид сечения в координатах плоскости.
    /// </summary>
    class SectionView
    {
    private:
        Graphics::LineRenderer lineRenderer_;
        Graphics::OverlayText text_;
        int x_ = 0;
        int y_ = 0;
        int width_ = 1;
        int height_ = 1;

    public:
        void Initialize();
        void SetViewport(int x, int y, int width, int height);
        void Render(const ViewModels::MainViewModel& viewModel) const;
        void Destroy();
    };
}
