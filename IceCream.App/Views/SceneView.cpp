#include "SceneView.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cstdio>

namespace IceCream::App::Views
{
    using ViewModels::ColorRole;
	namespace
	{
		std::vector<float> PackPoints(const std::vector<Core::Geometry::Point3D>& points, bool closed)
		{
			std::vector<float> vertices;

			if (points.empty())
			{
				return vertices;
			}

			vertices.reserve((points.size() + (closed ? 1 : 0)) * 3);

			for (const auto& point : points)
			{
				vertices.push_back(static_cast<float>(point.X));
				vertices.push_back(static_cast<float>(point.Y));
				vertices.push_back(static_cast<float>(point.Z));
			}

			if (closed)
			{
				vertices.push_back(static_cast<float>(points.front().X));
				vertices.push_back(static_cast<float>(points.front().Y));
				vertices.push_back(static_cast<float>(points.front().Z));
			}

			return vertices;
		}

		std::vector<float> PlaneFrame(const Core::Geometry::Plane3D& plane, float size)
		{
			const auto& origin = plane.Origin();
			const auto& u = plane.AxisU();
			const auto& v = plane.AxisV();

			const Core::Geometry::Point3D corners[4] =
			    {
			        origin + u * (-size) + v * (-size),
			        origin + u * (size) + v * (-size),
			        origin + u * (size) + v * (size),
			        origin + u * (-size) + v * (size)};

			std::vector<float> vertices;
			vertices.reserve(15);

			for (int i = 0; i < 5; ++i)
			{
				const auto& point = corners[i % 4];
				vertices.push_back(static_cast<float>(point.X));
				vertices.push_back(static_cast<float>(point.Y));
				vertices.push_back(static_cast<float>(point.Z));
			}

			return vertices;
		}
	}

	void SceneView::Initialize()
	{
		text_.Initialize();
        hints_.Initialize();
		meshRenderer_.Initialize();
		lineRenderer_.Initialize();
	}

	void SceneView::SetViewport(int x, int y, int width, int height, int dpi)
	{
		x_ = x;
		y_ = y;
		width_ = width;
		height_ = height;
        dpi_ = dpi;
	}

	void SceneView::UploadMesh(const Core::Geometry::TriangleMesh& mesh, const std::vector<std::array<float,4>>& colors)
	{
		meshRenderer_.Upload(mesh, colors);
		float minX = 0, maxX = 0, minY = 0, maxY = 0;
		minZ_ = maxZ_ = static_cast<float>(mesh.Vertices().front().Z);

		for (auto p : mesh.Vertices())
		{
			minX = std::min(minX, static_cast<float>(p.X));
			maxX = std::max(maxX, static_cast<float>(p.X));
			minY = std::min(minY, static_cast<float>(p.Y));
			maxY = std::max(maxY, static_cast<float>(p.Y));
			minZ_ = std::min(minZ_, static_cast<float>(p.Z));
			maxZ_ = std::max(maxZ_, static_cast<float>(p.Z));
		}

		centerX_ = (minX + maxX) * .5f;
		centerY_ = (minY + maxY) * .5f;
		extent_ = std::max(2.0f, std::ceil(std::max({std::abs(minX), std::abs(maxX), std::abs(minY), std::abs(maxY)}) + 1));
	}

    std::string SceneView::MeasurementText(const ViewModels::MainViewModel& viewModel, bool compact)
    {
        if (!viewModel.HasVolume())
            return compact ? "Объём недоступен" : "Объём недоступен: сетка не замкнута";

        char status[128];
        std::snprintf(status, sizeof(status), viewModel.UsesMeshVolume() ? "Объём по сетке: %.5g" : "Интеграл / объём: %.5g",
            viewModel.Volume());
        return status;
    }

	void SceneView::Render(const ViewModels::MainViewModel& viewModel, bool showMeasurement) const
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
        else if (!viewModel.HasVolume())
            header.Add(hints_, textColor, "Сетка не замкнута", 2);
        if (appearance.ShowHints)
            header.Add(hints_, textColor, "ЛКМ: вращать · ПКМ: перемещать · колесо: масштаб", 2);
        if (appearance.ShowConstraintSurfaces)
        {
            const auto& guides = viewModel.ConstraintSurfaces();
            for (size_t i = 0; i < guides.size(); ++i)
            {
                const int rows = header.AvailableRows(hints_);
                if (rows == 0) break;
                if (rows == 1 && i + 1 < guides.size())
                {
                    header.Add(hints_, textColor, ("Ещё ограничений: " + std::to_string(guides.size() - i)).c_str());
                    break;
                }
                const auto& guide = guides[i];
                std::string label = std::to_string(i + 1) + ": " + guide.Condition;
                if (guide.Undefined) label += " (не определена)";
                else if (guide.Triangles.empty()) label += " (вне области показа)";
                header.Add(hints_, guide.Color, label.c_str());
            }
        }
        plotHeight_ = height_ - header.Height();
        glViewport(x_, y_, width_, plotHeight_);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glScissor(x_, y_, width_, height_);
		const auto background = appearance.Color(ColorRole::Scene);
        glClearColor(background.r, background.g, background.b, 1);
		glStencilMask(0xff);
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glScissor(x_, y_, width_, plotHeight_);

		const glm::mat4 rotation(viewModel.SurfaceRotation());
        const glm::vec3 center = glm::vec3(viewModel.SurfaceRotation() * glm::dvec3(centerX_,centerY_,viewModel.CenterZ()));
        const glm::mat4 model = glm::translate(
		    glm::mat4(1.0f),
		    -center) * rotation;

		// Масштабируем сцену, чтобы поверхность и сетка помещались в кадре.
		const float fit = std::max({1.0f, (maxZ_ - minZ_) / 5.0f, extent_ / 3.5f});
		const glm::mat4 view = camera_.ViewMatrix() * glm::scale(glm::mat4(1), glm::vec3(1.0f / fit));
        const float aspect = static_cast<float>(width_) / static_cast<float>(plotHeight_);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
        // Keep the same field of view along the shorter side.
        projection[0][0] *= std::min(aspect, 1.0f);
        projection[1][1] *= std::min(aspect, 1.0f);
		const glm::mat4 mvp = projection * view * model;
		DrawCoordinates(projection * view * glm::translate(glm::mat4(1), -glm::vec3(viewModel.SurfaceRotation() * glm::dvec3(centerX_,centerY_,viewModel.CenterZ()))), appearance, glm::mat3(rotation));
		const float joinHeight = static_cast<float>(viewModel.Parameters().BaseHeight);

		const auto& plane = viewModel.SectionPlane();
		const auto& normal = plane.Normal();
		const auto& origin = plane.Origin();
		const glm::vec4 cutPlane(static_cast<float>(normal.X), static_cast<float>(normal.Y),
		                         static_cast<float>(normal.Z), static_cast<float>(-normal.X * origin.X - normal.Y * origin.Y - normal.Z * origin.Z));

        glDisable(GL_SCISSOR_TEST);
		meshRenderer_.RenderShadowMap(viewModel.Lighting());
        glEnable(GL_SCISSOR_TEST);

		meshRenderer_.Draw(model, view, projection, joinHeight, cutPlane, false, viewModel.TransparentPositive(), viewModel.HasPartJoin(), viewModel.Lighting(), appearance);

		// Трафарет позволяет закрасить невыпуклые сечения с отверстиями.
		glEnable(GL_STENCIL_TEST);
		glStencilMask(1);
		glStencilFunc(GL_ALWAYS, 0, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INVERT);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);
		glDisable(GL_DEPTH_TEST);
		float minU = 0, maxU = 0, minV = 0, maxV = 0;
		bool hasArea = false;

		for (const auto& contour : viewModel.Section().Contours())
		{
			if (!contour.IsClosed() || contour.Points2D().size() < 3)
			{
				continue;
			}

			hasArea = true;

			for (const auto& point : contour.Points2D())
			{
				minU = std::min(minU, static_cast<float>(point.X));
				maxU = std::max(maxU, static_cast<float>(point.X));
				minV = std::min(minV, static_cast<float>(point.Y));
				maxV = std::max(maxV, static_cast<float>(point.Y));
			}

			lineRenderer_.Draw(PackPoints(contour.Points3D(), false), mvp, glm::vec3(1), GL_TRIANGLE_FAN);
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_TEST);
		glStencilMask(0);
		glStencilFunc(GL_EQUAL, 1, 1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		if (hasArea)
		{
			std::vector<Core::Geometry::Point3D> corners = {
			    plane.ToGlobal({minU - .01, minV - .01}), plane.ToGlobal({maxU + .01, minV - .01}),
			    plane.ToGlobal({maxU + .01, maxV + .01}), plane.ToGlobal({minU - .01, maxV + .01})};
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(1.0f, 1.0f);
			lineRenderer_.Draw(PackPoints(corners, false), mvp, glm::vec3(appearance.Color(ColorRole::Contour)), GL_TRIANGLE_FAN);
			glDisable(GL_POLYGON_OFFSET_FILL);
		}

		glDisable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		meshRenderer_.Draw(model, view, projection, joinHeight, cutPlane, true, viewModel.TransparentPositive(), viewModel.HasPartJoin(), viewModel.Lighting(), appearance);

		if (appearance.ShowConstraintSurfaces)
		{
			std::vector<std::pair<float, const ViewModels::ConstraintSurface*>> guides;
			for (const auto& guide : viewModel.ConstraintSurfaces())
				if (!guide.Triangles.empty()) guides.emplace_back((view * model * glm::vec4(guide.Center, 1)).z, &guide);
			std::sort(guides.begin(), guides.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
			glDepthMask(GL_FALSE);
			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(-1.f, -1.f);
			for (const auto& item : guides)
				lineRenderer_.Draw(item.second->Triangles, mvp, item.second->Color, GL_TRIANGLES, .18f);
			glDisable(GL_POLYGON_OFFSET_FILL);
			glDepthMask(GL_TRUE);
		}

		glDepthFunc(GL_LEQUAL);
		glLineWidth(1.0f);
		lineRenderer_.Draw(PlaneFrame(plane, std::max({extent_, (maxZ_ - minZ_) * .6f, 2.0f}) + .5f),
		                   mvp, glm::vec3(.45f, .53f, .60f), GL_LINE_STRIP);
		glLineWidth(2.5f);

		for (const auto& contour : viewModel.Section().Contours())
		{
			lineRenderer_.Draw(PackPoints(contour.Points3D(), contour.IsClosed()), mvp, glm::vec3(appearance.Color(ColorRole::Contour)), GL_LINE_STRIP, appearance.Color(ColorRole::Contour).a);
		}
		// Границы частей рисуем отдельно от контура сечения.
		glLineWidth(3.0f);
		const glm::vec3 seamColor(appearance.Color(ColorRole::Seam));

		if (viewModel.HasPartJoin())
		{
			auto ring = viewModel.JoinBoundary();

			for (auto& p : ring)
			{
				p.X *= 1.001;
				p.Y *= 1.001;
			}

			lineRenderer_.Draw(PackPoints(ring, true), mvp, seamColor, GL_LINE_STRIP, appearance.Color(ColorRole::Seam).a);

			for (const auto& trace : viewModel.JoinInSection())
			{
				lineRenderer_.Draw(PackPoints(trace.Points3D(), trace.IsClosed()), mvp, seamColor,
				                   trace.Points3D().size() == 1 ? GL_POINTS : GL_LINE_STRIP, appearance.Color(ColorRole::Seam).a);
			}
		}

		if (viewModel.IsHeightField() || viewModel.IsIntegralBody())
		{
			glPointSize(6);

			for (const auto& edge : viewModel.SurfaceEdges())
			{
				lineRenderer_.Draw(PackPoints(edge, false), mvp, seamColor, GL_LINE_STRIP, appearance.Color(ColorRole::Seam).a);
			}
		}

		glLineWidth(1.0f);
		glDepthFunc(GL_LESS);
		glDisable(GL_DEPTH_TEST);
		DrawOrientation(view, appearance);
        glScissor(x_, y_, width_, height_);
        header.Draw(x_, y_, height_);
        glDisable(GL_SCISSOR_TEST);
		glEnable(GL_DEPTH_TEST);
	}

	void SceneView::DrawCoordinates(const glm::mat4& mvp, const ViewModels::AppearanceSettings& appearance, const glm::mat3& rotation) const
	{
        float extent=2, minZ=0, maxZ=1;
        for(float x : {-extent_,extent_}) for(float y : {-extent_,extent_}) for(float z : {minZ_,maxZ_}) {
            auto p=rotation*glm::vec3(x,y,z);extent=std::max({extent,std::abs(p.x),std::abs(p.y)});minZ=std::min(minZ,p.z);maxZ=std::max(maxZ,p.z);
        }
        extent=std::ceil(extent);
		const float step = std::max(1.0f, std::ceil(extent / 8));
		std::vector<float> grid;
		const float floorZ = std::min(0.0f, minZ);

		for (float t = -extent; t <= extent + .01f; t += step)
		{
			grid.insert(grid.end(), {t, -extent, floorZ, t, extent, floorZ,
			                         -extent, t, floorZ, extent, t, floorZ});
		}

		lineRenderer_.Draw(grid, mvp, glm::vec3(appearance.Color(ColorRole::Grid)), GL_LINES);
		const glm::vec3 colors[] = {glm::vec3(appearance.Color(ColorRole::AxisX)), glm::vec3(appearance.Color(ColorRole::AxisY)), glm::vec3(appearance.Color(ColorRole::AxisZ))};
		const float zEnd = std::max(1.0f, maxZ + .6f);
		glLineWidth(2);
		lineRenderer_.Draw({-extent, 0, 0, extent, 0, 0, extent, 0, 0, extent - .22f, .10f, 0,
		                    extent, 0, 0, extent - .22f, -.10f, 0},
		                   mvp, colors[0], GL_LINES);
		lineRenderer_.Draw({0, -extent, 0, 0, extent, 0, 0, extent, 0, .10f, extent - .22f, 0,
		                    0, extent, 0, -.10f, extent - .22f, 0},
		                   mvp, colors[1], GL_LINES);
		lineRenderer_.Draw({0, 0, std::min(0.0f, minZ), 0, 0, zEnd, 0, 0, zEnd, .10f, 0, zEnd - .22f,
		                    0, 0, zEnd, -.10f, 0, zEnd - .22f},
		                   mvp, colors[2], GL_LINES);
        std::vector<glm::vec4> labelBounds;
        const float gap = static_cast<float>(MulDiv(2, dpi_, 96));
        auto label = [&](const glm::vec3& at, const glm::vec3& color, const char* value)
        {
            const glm::vec4 clip = mvp * glm::vec4(at, 1);
            if (clip.w <= 0 || std::abs(clip.z) > clip.w)
                return;
            const glm::vec2 pixel = (glm::vec2(clip) / clip.w * .5f + .5f) * glm::vec2(width_, plotHeight_);
            const float top = pixel.y + text_.Ascent();
            const glm::vec4 bounds(pixel.x, top - text_.Height(), pixel.x + text_.Measure(value), top);
            if (bounds.x < 0 || bounds.y < 0 || bounds.z > width_ || bounds.w > plotHeight_)
                return;
            for (const auto& other : labelBounds)
                if (bounds.x < other.z + gap && bounds.z + gap > other.x &&
                    bounds.y < other.w + gap && bounds.w + gap > other.y)
                    return;
            labelBounds.push_back(bounds);
            text_.World(at, mvp, color, value);
        };
        label({extent, 0, 0}, colors[0], "X");
        label({0, extent, 0}, colors[1], "Y");
        label({0, 0, zEnd}, colors[2], "Z");
        label({.06f, .06f, .03f}, glm::vec3(appearance.Color(ColorRole::Text)), "0");
		char number[24];

		for (float t = -extent + step; t < extent; t += step)
		{
			if (std::abs(t) < .01f)
			{
				continue;
			}

			lineRenderer_.Draw({t, -.06f, 0, t, .06f, 0, -.06f, t, 0, .06f, t, 0}, mvp, glm::vec3(appearance.Color(ColorRole::Text)), GL_LINES);
			std::snprintf(number, sizeof(number), "%g", t);
			label({t, -.17f, 0}, colors[0], number);
			label({-.17f, t, 0}, colors[1], number);
		}

		const float zStep = std::max(1.0f, std::ceil((zEnd - floorZ) / 10));

		for (float z = std::ceil(floorZ / zStep) * zStep; z < zEnd; z += zStep)
		{
			lineRenderer_.Draw({-.07f, 0, z, .07f, 0, z}, mvp, colors[2], GL_LINES);
            if (std::abs(z) >= .01f)
            {
                std::snprintf(number, sizeof(number), "%g", z);
                label({.10f, 0, z}, colors[2], number);
            }
		}

		glLineWidth(1);
	}

	void SceneView::DrawOrientation(const glm::mat4& view, const ViewModels::AppearanceSettings& appearance) const
	{
        const float scale = dpi_ / 96.0f;
        const char* title = "Ориентация";
        const char* names[] = {"X", "Y", "Z"};
        const float left = 8 * scale, bottom = 8 * scale, padding = 8 * scale;
        const float arm = 27 * scale, labelOffset = 3 * scale;
        const float descent = static_cast<float>(hints_.Height() - hints_.Ascent());
        const float nameWidth = static_cast<float>(std::max({hints_.Measure(names[0]), hints_.Measure(names[1]),
            hints_.Measure(names[2])}));
        const float axesWidth = 2 * arm + labelOffset + nameWidth;
        const float contentWidth = std::max(axesWidth, static_cast<float>(hints_.Measure(title)));
        const float right = left + 2 * padding + contentWidth;
        const float originX = left + padding + arm + (contentWidth - axesWidth) * .5f;
        const float originY = bottom + padding + arm + descent;
        const float axesTop = originY + arm + labelOffset + hints_.Ascent();
        const float titleY = axesTop + padding + descent;
        const float top = titleY + hints_.Ascent() + padding;
        if (width_ < 240 * scale || plotHeight_ < std::max(170 * scale, top + padding))
            return;

        const glm::mat4 ortho = glm::ortho(0.0f, static_cast<float>(width_), 0.0f, static_cast<float>(plotHeight_));
        lineRenderer_.Draw({left, bottom, 0, right, bottom, 0, right, top, 0, left, top, 0},
            ortho, glm::vec3(appearance.Color(ColorRole::Panel)), GL_TRIANGLE_FAN);
        hints_.Screen(left + padding, titleY, width_, plotHeight_, glm::vec3(appearance.Color(ColorRole::Text)), title);
		const glm::vec3 colors[] = {glm::vec3(appearance.Color(ColorRole::AxisX)), glm::vec3(appearance.Color(ColorRole::AxisY)), glm::vec3(appearance.Color(ColorRole::AxisZ))};
		const glm::mat3 rotation(view);
		glLineWidth(2.5f);

		for (int i = 0; i < 3; ++i)
		{
			glm::vec3 axis(0);
			axis[i] = 1;
			glm::vec3 direction = glm::normalize(rotation * axis);
			float x = originX + direction.x * arm, y = originY + direction.y * arm;
			lineRenderer_.Draw({originX, originY, 0, x, y, 0}, ortho, colors[i], GL_LINES);
			hints_.Screen(x + labelOffset, y + labelOffset, width_, plotHeight_, colors[i], names[i]);
		}

		glLineWidth(1);
	}

	bool SceneView::Contains(int windowX, int windowY, int windowHeight) const
	{
		const int openGlY = windowHeight - 1 - windowY;

		return windowX >= x_ && windowX < x_ + width_ && openGlY >= y_ && openGlY < y_ + height_;
	}

	void SceneView::OnMouseDown(int x, int y)
	{
		camera_.BeginRotate(x, y);
	}

	void SceneView::OnMouseUp()
	{
		camera_.EndRotate();
	}

	void SceneView::OnPanDown(int x, int y)
	{
		camera_.BeginPan(x, y);
	}

	void SceneView::OnPanUp()
	{
		camera_.EndPan();
	}

	void SceneView::OnMouseMove(int x, int y)
	{
		camera_.Rotate(x, y);
		camera_.Pan(x, y, std::min(width_, plotHeight_));
	}

	void SceneView::OnWheel(int direction)
	{
		camera_.Zoom(direction);
	}

	void SceneView::ResetCamera()
	{
		camera_.Reset();
	}

	void SceneView::Destroy()
	{
		text_.Destroy();
        hints_.Destroy();
        measurements_.Destroy();
		meshRenderer_.Destroy();
		lineRenderer_.Destroy();
	}
}
