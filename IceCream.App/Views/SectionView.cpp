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
		std::vector<float> PackContour2D(const Core::Models::SectionContour& contour, double centerX, double centerY)
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
				vertices.push_back(static_cast<float>(point.X-centerX));
				vertices.push_back(static_cast<float>(point.Y-centerY));
				vertices.push_back(0.0f);
			}

			if (contour.IsClosed())
			{
				vertices.push_back(static_cast<float>(points.front().X-centerX));
				vertices.push_back(static_cast<float>(points.front().Y-centerY));
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
        camera_.SetViewport(width_, height_);
	}

    bool SectionView::Contains(int x, int y, int windowHeight) const
    {
        int top = windowHeight-y_-height_;
        return x >= x_ && x < x_+width_ && y >= top && y < top+height_;
    }
    void SectionView::OnMouseDown(int x, int y)
    {
        dragging_ = true; lastX_ = x; lastY_ = y;
    }
    void SectionView::OnMouseMove(int x, int y)
    {
        if (!dragging_) return;
        camera_.PanPixels(x-lastX_, y-lastY_);
        lastX_ = x; lastY_ = y;
    }
    void SectionView::OnWheel(int direction, int x, int y, int windowHeight)
    {
        camera_.ZoomAt(direction, x-x_, y-(windowHeight-y_-height_));
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
		glDisable(GL_DEPTH_TEST);

		Graphics::SectionBounds content{std::numeric_limits<double>::max(), -std::numeric_limits<double>::max(),
            std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
        bool hasPoints = false;
		for (const auto& contour : viewModel.Section().Contours())
		{
			for (const auto& point : contour.Points2D())
			{
                hasPoints = true;
                content.MinX = std::min(content.MinX, point.X); content.MaxX = std::max(content.MaxX, point.X);
                content.MinY = std::min(content.MinY, point.Y); content.MaxY = std::max(content.MaxY, point.Y);
			}
		}

        if (geometryRevision_ != viewModel.GeometryRevision())
        {
            camera_.ResetView();
            geometryRevision_ = viewModel.GeometryRevision();
        }
        camera_.SetContentBounds(hasPoints ? content : Graphics::SectionBounds{-1.5,1.5,-1.5,1.5});
        auto bounds = camera_.Bounds();
        const double minX = bounds.MinX, maxX = bounds.MaxX, minY = bounds.MinY, maxY = bounds.MaxY;
        const double centerX = camera_.CenterX(), centerY = camera_.CenterY();
        const float halfX = static_cast<float>((maxX-minX)*.5), halfY = static_cast<float>((maxY-minY)*.5);
        // Subtract the camera origin in double precision before uploading floats.
        const glm::mat4 mvp = glm::ortho(-halfX, halfX, -halfY, halfY, -1.0f, 1.0f);

		const double range = std::max(maxX - minX, maxY - minY);
		const double magnitude = std::pow(10.0, std::floor(std::log10(range / 8)));
		const double normalized = range / 8 / magnitude;
		const double step = (normalized <= 1 ? 1 : normalized <= 2 ? 2
		                                      : normalized <= 5   ? 5
		                                                          : 10) *
		                   magnitude;
		std::vector<float> grid;

        std::vector<double> ticksX, ticksY;
        double startX = std::ceil(minX/step)*step, startY = std::ceil(minY/step)*step;
        // Integer iteration stays bounded even at extreme zoom/large coordinates.
        for (int i = 0; i < 64; ++i)
        {
            double x = startX+i*step, y = startY+i*step;
            if (x < maxX)
            {
                ticksX.push_back(x);
                float local = static_cast<float>(x-centerX);
                grid.insert(grid.end(), {local,-halfY,0,local,halfY,0});
            }
            if (y < maxY)
            {
                ticksY.push_back(y);
                float local = static_cast<float>(y-centerY);
                grid.insert(grid.end(), {-halfX,local,0,halfX,local,0});
            }
        }

		lineRenderer_.Draw(grid, mvp, glm::vec3(appearance.Color(ColorRole::Grid)), GL_LINES);
		glLineWidth(1.0f);
		lineRenderer_.Draw(Axis(-halfX, static_cast<float>(-centerY), halfX, static_cast<float>(-centerY)), mvp, glm::vec3(appearance.Color(ColorRole::AxisX)), GL_LINES);
		lineRenderer_.Draw(Axis(static_cast<float>(-centerX), -halfY, static_cast<float>(-centerX), halfY), mvp, glm::vec3(appearance.Color(ColorRole::AxisY)), GL_LINES);

		glLineWidth(2.4f);

		for (const auto& contour : viewModel.Section().Contours())
		{
			lineRenderer_.Draw(PackContour2D(contour, centerX, centerY), mvp, glm::vec3(appearance.Color(ColorRole::Contour)), GL_LINE_STRIP, appearance.Color(ColorRole::Contour).a);
		}

		glLineWidth(3);
		const glm::vec3 seamColor(appearance.Color(ColorRole::Seam));

		for (const auto& trace : viewModel.JoinInSection())
		{
			lineRenderer_.Draw(PackContour2D(trace, centerX, centerY), mvp, seamColor, trace.Points2D().size() == 1 ? GL_POINTS : GL_LINE_STRIP, appearance.Color(ColorRole::Seam).a);
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
					markers.insert(markers.end(), {static_cast<float>(local.X-centerX), static_cast<float>(local.Y-centerY), 0});
				}
			}

			glPointSize(7);
			lineRenderer_.Draw(markers, mvp, seamColor, GL_POINTS);
			glPointSize(1);
		}

		glLineWidth(1);
		char number[24];

        float labelsY = static_cast<float>(std::clamp(-minY/(maxY-minY)*height_+4, 12.0, std::max(12.0, static_cast<double>(height_-112))));
        float labelsX = static_cast<float>(std::clamp(-minX/(maxX-minX)*width_+4, 8.0, std::max(8.0, static_cast<double>(width_-75))));
        double previousLabelEnd = -1;
		for (double x : ticksX)
		{
			if (std::abs(x) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%.15g", x);
            double pixel = (x-minX)/(maxX-minX)*width_+3;
            double end = pixel + std::char_traits<char>::length(number)*appearance.CoordinateSize*.65;
            if (pixel >= previousLabelEnd+8 && end < width_-22)
            {
                text_.Screen(static_cast<float>(pixel), labelsY, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), number);
                previousLabelEnd = end;
            }
		}

		for (double y : ticksY)
		{
			if (std::abs(y) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%.15g", y);
			float pixelY = static_cast<float>((y-minY)/(maxY-minY)*height_+3);

			if (pixelY < height_ - 82 && pixelY > 8)
			{
				text_.Screen(labelsX, pixelY, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), number);
			}
		}

		text_.Screen(static_cast<float>(width_-22), labelsY, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), "u");
		text_.Screen(labelsX, static_cast<float>(height_-100), width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), "v");
        char status[128];
        if (viewModel.HasSectionArea())
            std::snprintf(status,sizeof(status),"Площадь: %.5g",viewModel.SectionArea());
        else
            std::snprintf(status,sizeof(status),"Площадь недоступна: открытый контур");
        text_.Screen(16,height_-32,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),status);
        if (viewModel.Section().IsEmpty()) hints_.Screen(16,height_-62,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),"Плоскость не пересекает фигуру");
        if (appearance.ShowHints) hints_.Screen(16,height_-88,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),"Тянуть: переместить · колесо: масштаб · C/СКМ: вписать");

		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	void SectionView::Destroy()
	{
		text_.Destroy();
        hints_.Destroy();
		lineRenderer_.Destroy();
	}
}
