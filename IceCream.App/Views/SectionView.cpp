#include "SectionView.h"

#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace IceCream::App::Views
{
    using ViewModels::ColorRole;
	namespace
	{
		std::vector<float> PackContour2D(const Core::Models::SectionContour& contour)
		{
			std::vector<float> vertices;
			const auto& points = contour.Points2D();

			if (points.empty())
			{
				return vertices;
			}

			vertices.reserve((points.size() + (contour.IsClosed() ? 1 : 0)) * 3);

			for (const auto& point : points)
			{
				vertices.push_back(static_cast<float>(point.X));
				vertices.push_back(static_cast<float>(point.Y));
				vertices.push_back(0.0f);
			}

			if (contour.IsClosed())
			{
				vertices.push_back(static_cast<float>(points.front().X));
				vertices.push_back(static_cast<float>(points.front().Y));
				vertices.push_back(0.0f);
			}

			return vertices;
		}

		std::vector<float> Axis(float x0, float y0, float x1, float y1)
		{
			return {x0, y0, 0.0f, x1, y1, 0.0f};
		}
	}

	void SectionView::Initialize()
	{
		text_.Initialize();
        hints_.Initialize();
		lineRenderer_.Initialize();
	}

	void SectionView::SetViewport(int x, int y, int width, int height)
	{
		x_ = x;
		y_ = y;
		width_ = std::max(width, 1);
		height_ = std::max(height, 1);
	}

	void SectionView::Render(const ViewModels::MainViewModel& viewModel) const
	{
		const auto& appearance = viewModel.Appearance();
        text_.SetSize(appearance.CoordinateSize);
        hints_.SetSize(appearance.HintSize);
        glViewport(x_, y_, width_, height_);
		glEnable(GL_SCISSOR_TEST);
		glScissor(x_, y_, width_, height_);
		const auto background = appearance.Color(ColorRole::Section);
        glClearColor(background.r, background.g, background.b, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);

		float minX = -1.5f;
		float maxX = 1.5f;
		float minY = -1.5f;
		float maxY = 1.5f;

		for (const auto& contour : viewModel.Section().Contours())
		{
			for (const auto& point : contour.Points2D())
			{
				minX = std::min(minX, static_cast<float>(point.X));
				maxX = std::max(maxX, static_cast<float>(point.X));
				minY = std::min(minY, static_cast<float>(point.Y));
				maxY = std::max(maxY, static_cast<float>(point.Y));
			}
		}

		const float pad = 0.35f;
		minX -= pad;
		maxX += pad;
		minY -= pad;
		maxY += pad;

		const float aspect = static_cast<float>(width_) / static_cast<float>(height_);
		float spanX = maxX - minX;
		float spanY = maxY - minY;

		if (spanX / spanY < aspect)
		{
			const float extra = spanY * aspect - spanX;
			minX -= extra * 0.5f;
			maxX += extra * 0.5f;
		}
		else
		{
			const float extra = spanX / aspect - spanY;
			minY -= extra * 0.5f;
			maxY += extra * 0.5f;
		}

		const glm::mat4 mvp = glm::ortho(minX, maxX, minY, maxY, -1.0f, 1.0f);

		const float range = std::max(maxX - minX, maxY - minY);
		const float magnitude = std::pow(10.0f, std::floor(std::log10(range / 8)));
		const float normalized = range / 8 / magnitude;
		const float step = (normalized <= 1 ? 1 : normalized <= 2 ? 2
		                                      : normalized <= 5   ? 5
		                                                          : 10) *
		                   magnitude;
		std::vector<float> grid;

		for (float x = std::ceil(minX / step) * step; x < maxX; x += step)
		{
			grid.insert(grid.end(), {x, minY, 0, x, maxY, 0});
		}

		for (float y = std::ceil(minY / step) * step; y < maxY; y += step)
		{
			grid.insert(grid.end(), {minX, y, 0, maxX, y, 0});
		}

		lineRenderer_.Draw(grid, mvp, glm::vec3(appearance.Color(ColorRole::Grid)), GL_LINES);
		glLineWidth(1.0f);
		lineRenderer_.Draw(Axis(minX, 0.0f, maxX, 0.0f), mvp, glm::vec3(appearance.Color(ColorRole::AxisX)), GL_LINES);
		lineRenderer_.Draw(Axis(0.0f, minY, 0.0f, maxY), mvp, glm::vec3(appearance.Color(ColorRole::AxisY)), GL_LINES);

		glLineWidth(2.4f);

		for (const auto& contour : viewModel.Section().Contours())
		{
			lineRenderer_.Draw(PackContour2D(contour), mvp, glm::vec3(appearance.Color(ColorRole::Contour)), GL_LINE_STRIP, appearance.Color(ColorRole::Contour).a);
		}

		glLineWidth(3);
		const glm::vec3 seamColor(appearance.Color(ColorRole::Seam));

		for (const auto& trace : viewModel.JoinInSection())
		{
			lineRenderer_.Draw(PackContour2D(trace), mvp, seamColor, trace.Points2D().size() == 1 ? GL_POINTS : GL_LINE_STRIP, appearance.Color(ColorRole::Seam).a);
		}
		// Отмечаем пересечения плоскости с рёбрами поверхности, стенок и основания.
		if (viewModel.IsHeightField() || viewModel.IsIntegralBody())
		{
			std::vector<float> markers;

			for (const auto& edge : viewModel.SurfaceEdges())
			{
				for (size_t i = 1; i < edge.size(); ++i)
				{
					auto a = edge[i - 1], b = edge[i];
					double da = viewModel.SectionPlane().SignedDistanceTo(a), db = viewModel.SectionPlane().SignedDistanceTo(b);

					if (da * db > 0 || std::abs(da - db) < 1e-12)
					{
						continue;
					}

					auto point = a + (b - a) * (da / (da - db));
					auto local = viewModel.SectionPlane().ToLocal(point);
					markers.insert(markers.end(), {static_cast<float>(local.X), static_cast<float>(local.Y), 0});
				}
			}

			glPointSize(7);
			lineRenderer_.Draw(markers, mvp, seamColor, GL_POINTS);
			glPointSize(1);
		}

		glLineWidth(1);
		char number[24];

		for (float x = std::ceil(minX / step) * step; x < maxX; x += step)
		{
			if (std::abs(x) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%g", x);
			text_.Screen((x - minX) / (maxX - minX) * width_ + 3, (0 - minY) / (maxY - minY) * height_ + 4, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), number);
		}

		for (float y = std::ceil(minY / step) * step; y < maxY; y += step)
		{
			if (std::abs(y) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%g", y);
			float pixelY = (y - minY) / (maxY - minY) * height_ + 3;

			if (pixelY < height_ - 82 && pixelY > 8)
			{
				text_.Screen((0 - minX) / (maxX - minX) * width_ + 4, pixelY, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), number);
			}
		}

		text_.Screen(width_ - 22, (0 - minY) / (maxY - minY) * height_ + 5, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), "u");
		text_.Screen((0 - minX) / (maxX - minX) * width_ + 5, height_ - 80, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), "v");
        char status[128];
        std::snprintf(status,sizeof(status),"Площадь: %.5g",viewModel.SectionArea());
        text_.Screen(16,height_-32,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),status);
        if (viewModel.Section().IsEmpty()) hints_.Screen(16,height_-62,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),"Плоскость не пересекает фигуру");
        if (appearance.ShowHints) hints_.Screen(16,height_-88,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),"u, v — локальные координаты плоскости");

		glEnable(GL_DEPTH_TEST);
	}

	void SectionView::Destroy()
	{
		text_.Destroy();
        hints_.Destroy();
		lineRenderer_.Destroy();
	}
}
