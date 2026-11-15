#pragma once

#include "Graphics/OrbitCamera/OrbitCamera.h"
#include "Graphics/OrbitCamera/OverlayText.h"
#include "Graphics/Render/MeshRenderer.h"
#include "Graphics/Render/LineRenderer.h"
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
        Graphics::OverlayText hints_;
        Graphics::OverlayText measurements_;

		float extent_ = 3;
		float minZ_ = 0;
		float maxZ_ = 4.2f;
		float centerX_ = 0;
		float centerY_ = 0;
		void DrawCoordinates(const glm::mat4& mvp, const ViewModels::AppearanceSettings& appearance, const glm::mat3& rotation) const;
		void DrawOrientation(const glm::mat4& view, const ViewModels::AppearanceSettings& appearance) const;
		int x_ = 0;
		int y_ = 0;
		int width_ = 0;
		int height_ = 0;
        int dpi_ = USER_DEFAULT_SCREEN_DPI;
        mutable int plotHeight_ = 1;

	public:
		void Initialize();
		void SetViewport(int x, int y, int width, int height, int dpi);
		void UploadMesh(const Core::Geometry::TriangleMesh& mesh, const std::vector<std::array<float,4>>& colors = {});
        static std::string MeasurementText(const ViewModels::MainViewModel& viewModel, bool compact = false);
		void Render(const ViewModels::MainViewModel& viewModel, bool showMeasurement) const;
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
