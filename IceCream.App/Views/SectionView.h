#pragma once
#include "Graphics/OrbitCamera/OverlayText.h"

#include "Graphics/Render/LineRenderer.h"
#include "Graphics/OrbitCamera/SectionCamera.h"
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
        Graphics::OverlayText hints_;
        Graphics::OverlayText measurements_;
        mutable Graphics::SectionCamera camera_;
        mutable std::uint64_t geometryRevision_ = ~std::uint64_t(0);
        bool dragging_ = false;
        int lastX_ = 0, lastY_ = 0;
		int x_ = 0;
		int y_ = 0;
		int width_ = 0;
		int height_ = 0;
        int dpi_ = USER_DEFAULT_SCREEN_DPI;
        mutable int plotHeight_ = 1;

	public:
		void Initialize();
		void SetViewport(int x, int y, int width, int height, int dpi);
        static std::string MeasurementText(const ViewModels::MainViewModel& viewModel, bool compact = false);
		void Render(const ViewModels::MainViewModel& viewModel, bool showMeasurement) const;
        bool Contains(int x, int y, int windowHeight) const;
        void OnMouseDown(int x, int y);
        void OnMouseUp() { dragging_ = false; }
        void OnMouseMove(int x, int y);
        void OnWheel(int direction, int x, int y, int windowHeight);
        void ResetView() { camera_.ResetView(); }
		void Destroy();
	};
}
