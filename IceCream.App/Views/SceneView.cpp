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

	void SceneView::SetViewport(int x, int y, int width, int height)
	{
		x_ = x;
		y_ = y;
		width_ = std::max(width, 1);
		height_ = std::max(height, 1);
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

	void SceneView::Render(const ViewModels::MainViewModel& viewModel) const
	{
		const auto& appearance = viewModel.Appearance();
        text_.SetSize(appearance.CoordinateSize);
        hints_.SetSize(appearance.HintSize);
        glViewport(x_, y_, width_, height_);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_SCISSOR_TEST);
		glScissor(x_, y_, width_, height_);
		const auto background = appearance.Color(ColorRole::Scene);
        glClearColor(background.r, background.g, background.b, 1);
		glStencilMask(0xff);
		glClearStencil(0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);

		const glm::mat4 rotation(viewModel.SurfaceRotation());
        const glm::vec3 center = glm::vec3(viewModel.SurfaceRotation() * glm::dvec3(centerX_,centerY_,viewModel.CenterZ()));
        const glm::mat4 model = glm::translate(
		    glm::mat4(1.0f),
		    -center) * rotation;

		// Масштабируем сцену, чтобы поверхность и сетка помещались в кадре.
		const float fit = std::max({1.0f, (maxZ_ - minZ_) / 5.0f, extent_ / 3.5f});
		const glm::mat4 view = camera_.ViewMatrix() * glm::scale(glm::mat4(1), glm::vec3(1.0f / fit));
		const glm::mat4 projection = glm::perspective(
		    glm::radians(45.0f),
		    static_cast<float>(width_) / static_cast<float>(height_),
		    0.1f,
		    100.0f);
		const glm::mat4 mvp = projection * view * model;
		DrawCoordinates(projection * view * glm::translate(glm::mat4(1), -glm::vec3(viewModel.SurfaceRotation() * glm::dvec3(centerX_,centerY_,viewModel.CenterZ()))), appearance, glm::mat3(rotation));
		const float joinHeight = static_cast<float>(viewModel.Parameters().BaseHeight);

		const auto& plane = viewModel.SectionPlane();
		const auto& normal = plane.Normal();
		const auto& origin = plane.Origin();
		const glm::vec4 cutPlane(static_cast<float>(normal.X), static_cast<float>(normal.Y),
		                         static_cast<float>(normal.Z), static_cast<float>(-normal.X * origin.X - normal.Y * origin.Y - normal.Z * origin.Z));

		meshRenderer_.RenderShadowMap(viewModel.Lighting());

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
        char status[128];
        if (!viewModel.HasVolume())
            std::snprintf(status, sizeof(status), "Объём недоступен: сетка не замкнута");
        else
            std::snprintf(status, sizeof(status), viewModel.UsesMeshVolume() ? "Объём по сетке: %.5g" : "Интеграл / объём: %.5g", viewModel.Volume());
        text_.Screen(16, height_-80, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), status);
        if (appearance.ShowConstraintSurfaces && !viewModel.ConstraintSurfaces().empty())
        {
            // Keep a long condition inside its own viewport, independently of font size.
            glEnable(GL_SCISSOR_TEST);
            glScissor(x_, y_, width_, height_);
            int row = height_ - 118;
            const int rowHeight = appearance.HintSize + 6;
            const size_t characters = static_cast<size_t>(std::max(12, (width_-32) / std::max(1, appearance.HintSize*2/3)));
            for (size_t i = 0; i < viewModel.ConstraintSurfaces().size(); ++i)
            {
                if (row < 175)
                {
                    hints_.Screen(16, static_cast<float>(row), width_, height_, glm::vec3(appearance.Color(ColorRole::Text)),
                        ("Ещё ограничений: " + std::to_string(viewModel.ConstraintSurfaces().size()-i)).c_str());
                    break;
                }
                const auto& guide = viewModel.ConstraintSurfaces()[i];
                std::string label = std::to_string(i+1) + ": " + guide.Condition;
                if (guide.Undefined) label += " (не определена)";
                else if (guide.Triangles.empty()) label += " (вне области показа)";
                if (label.size() > characters)
                {
                    size_t end = characters-3;
                    while (end > 0 && (static_cast<unsigned char>(label[end]) & 0xc0) == 0x80) --end;
                    label = label.substr(0, end) + "...";
                }
                hints_.Screen(16, static_cast<float>(row), width_, height_, guide.Color, label.c_str());
                row -= rowHeight;
            }
            glDisable(GL_SCISSOR_TEST);
        }
        if (appearance.ShowHints) {
            hints_.Screen(16,height_-62,width_,height_,glm::vec3(appearance.Color(ColorRole::Text)),"ЛКМ: вращать · ПКМ: перемещать · колесо: масштаб");
        }

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
		text_.World({extent, 0, 0}, mvp, colors[0], "X");
		text_.World({0, extent, 0}, mvp, colors[1], "Y");
		text_.World({0, 0, zEnd}, mvp, colors[2], "Z");
		char number[24];

		for (float t = -extent + step; t < extent; t += step)
		{
			if (std::abs(t) < .01f)
			{
				continue;
			}

			lineRenderer_.Draw({t, -.06f, 0, t, .06f, 0, -.06f, t, 0, .06f, t, 0}, mvp, glm::vec3(appearance.Color(ColorRole::Text)), GL_LINES);
			std::snprintf(number, sizeof(number), "%g", t);
			text_.World({t, -.17f, 0}, mvp, colors[0], number);
			text_.World({-.17f, t, 0}, mvp, colors[1], number);
		}

		const float zStep = std::max(1.0f, std::ceil((zEnd - floorZ) / 10));

		for (float z = std::ceil(floorZ / zStep) * zStep; z < zEnd; z += zStep)
		{
			lineRenderer_.Draw({-.07f, 0, z, .07f, 0, z}, mvp, colors[2], GL_LINES);
			std::snprintf(number, sizeof(number), "%g", z);
			text_.World({.10f, 0, z}, mvp, colors[2], number);
		}

		text_.World({.06f, .06f, .03f}, mvp, glm::vec3(appearance.Color(ColorRole::Text)), "0");
		glLineWidth(1);
	}

	void SceneView::DrawOrientation(const glm::mat4& view, const ViewModels::AppearanceSettings& appearance) const
	{
		const glm::mat4 ortho = glm::ortho(0.0f, static_cast<float>(width_), 0.0f, static_cast<float>(height_));
		lineRenderer_.Draw({12, 12, 0, 118, 12, 0, 118, 122, 0, 12, 122, 0}, ortho, glm::vec3(appearance.Color(ColorRole::Panel)), GL_TRIANGLE_FAN);
		text_.Screen(22, 105, width_, height_, glm::vec3(appearance.Color(ColorRole::Text)), "Ориентация");
		const glm::vec3 colors[] = {glm::vec3(appearance.Color(ColorRole::AxisX)), glm::vec3(appearance.Color(ColorRole::AxisY)), glm::vec3(appearance.Color(ColorRole::AxisZ))};
		const char* names[] = {"X", "Y", "Z"};
		const glm::mat3 rotation(view);
		glLineWidth(2.5f);

		for (int i = 0; i < 3; ++i)
		{
			glm::vec3 axis(0);
			axis[i] = 1;
			glm::vec3 direction = glm::normalize(rotation * axis);
			float x = 65 + direction.x * 37, y = 58 + direction.y * 37;
			lineRenderer_.Draw({65, 58, 0, x, y, 0}, ortho, colors[i], GL_LINES);
			text_.Screen(x + 3, y + 3, width_, height_, colors[i], names[i]);
		}

		glLineWidth(1);
	}

	bool SceneView::Contains(int windowX, int windowY, int windowHeight) const
	{
		const int openGlY = windowHeight - windowY;

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
		camera_.Pan(x, y, height_);
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
		meshRenderer_.Destroy();
		lineRenderer_.Destroy();
	}
}
