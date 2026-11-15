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

	void SectionView::SetViewport(int x, int y, int width, int height, int dpi)
	{
		x_ = x;
		y_ = y;
		width_ = width;
		height_ = height;
        dpi_ = dpi;
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
        const int localY = y - (windowHeight - y_ - plotHeight_);
        camera_.ZoomAt(direction, x - x_, std::clamp(localY, 0, plotHeight_));
    }

    std::string SectionView::MeasurementText(const ViewModels::MainViewModel& viewModel, bool compact)
    {
        if (!viewModel.HasSectionArea())
            return compact ? "Площадь недоступна" : "Площадь недоступна: открытый контур";

        char status[128];
        std::snprintf(status, sizeof(status), "Площадь: %.5g", viewModel.SectionArea());
        return status;
    }

	void SectionView::Render(const ViewModels::MainViewModel& viewModel, bool showMeasurement) const
	{
        if (width_ == 0 || height_ == 0)
            return;
		const auto& appearance = viewModel.Appearance();
        text_.SetSize(MulDiv(appearance.CoordinateSize, dpi_, 96));
        hints_.SetSize(MulDiv(appearance.HintSize, dpi_, 96));
        Graphics::OverlayHeader header(width_, height_, dpi_);
        const glm::vec3 textColor(appearance.Color(ColorRole::Text));
        if (showMeasurement)
        {
            measurements_.SetSize(MulDiv(appearance.TextSize + 2, dpi_, 96));
            header.Add(measurements_, textColor, MeasurementText(viewModel).c_str(), 2);
        }
        else if (!viewModel.HasSectionArea())
            header.Add(hints_, textColor, "Открытый контур", 2);
        if (viewModel.Section().IsEmpty())
            header.Add(hints_, textColor, "Плоскость не пересекает фигуру", 2);
        if (appearance.ShowHints)
            header.Add(hints_, textColor, "Тянуть: переместить · колесо: масштаб · C/СКМ: вписать", 2);
        plotHeight_ = height_ - header.Height();
        camera_.SetViewport(width_, plotHeight_);
        glViewport(x_, y_, width_, plotHeight_);
		glEnable(GL_SCISSOR_TEST);
		glScissor(x_, y_, width_, height_);
		const auto background = appearance.Color(ColorRole::Section);
        glClearColor(background.r, background.g, background.b, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
        glScissor(x_, y_, width_, plotHeight_);

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

        const int margin = MulDiv(6, dpi_, 96);
        const int rowHeight = text_.Height() + margin;
        const float labelsY = static_cast<float>(text_.Height() - text_.Ascent() + margin);
        const float labelsX = static_cast<float>(margin);
        const int uX = width_ - text_.Measure("u") - margin;
        double previousLabelEnd = margin;
        const double highestCoordinate = std::max({std::abs(minX), std::abs(maxX), std::abs(minY), std::abs(maxY)});
        const int precision = std::clamp(static_cast<int>(std::ceil(std::log10(highestCoordinate / step + 1))) + 2, 4, 15);
		for (double x : ticksX)
		{
			if (std::abs(x) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%.*g", precision, x);
            const int textWidth = text_.Measure(number);
            const double pixel = (x - minX) / (maxX - minX) * width_ - textWidth * .5;
            const double end = pixel + textWidth;
            if (pixel >= previousLabelEnd + margin && end < uX - margin)
            {
                text_.Screen(static_cast<float>(pixel), labelsY, width_, plotHeight_, textColor, number);
                previousLabelEnd = end;
            }
		}

        float previousLabelY = labelsY;
		for (double y : ticksY)
		{
			if (std::abs(y) < step * .01f)
			{
				continue;
			}

			std::snprintf(number, sizeof(number), "%.*g", precision, y);
			const float pixelY = static_cast<float>((y - minY) / (maxY - minY) * plotHeight_ - text_.Ascent() / 2);

			if (pixelY >= previousLabelY + rowHeight && pixelY < plotHeight_ - text_.Ascent() - rowHeight &&
                text_.Measure(number) + 2 * margin < width_)
			{
				text_.Screen(labelsX, pixelY, width_, plotHeight_, textColor, number);
                previousLabelY = pixelY;
			}
		}

        if (plotHeight_ >= 2 * rowHeight)
        {
            text_.Screen(static_cast<float>(uX), labelsY, width_, plotHeight_, textColor, "u");
            text_.Screen(labelsX, static_cast<float>(plotHeight_ - text_.Ascent() - margin), width_, plotHeight_, textColor, "v");
        }
        glScissor(x_, y_, width_, height_);
        header.Draw(x_, y_, height_);
		glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	void SectionView::Destroy()
	{
		text_.Destroy();
        hints_.Destroy();
        measurements_.Destroy();
		lineRenderer_.Destroy();
	}
}
