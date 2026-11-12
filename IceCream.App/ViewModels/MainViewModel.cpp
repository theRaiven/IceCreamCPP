#include "MainViewModel.h"

#include "../../IceCream.Core/Services/Meshing/SurfaceMeshBuilder.h"
#include "../../IceCream.Core/Services/Measurements/VolumeCalculator.h"
#include "../../IceCream.Core/Services/Measurements/SectionAreaCalculator.h"
#include "../../IceCream.Core/Services/Sections/MeshSectionCalculator.h"
#include "../../IceCream.Core/Services/Meshing/MeshClipper.h"
#include "../../IceCream.Core/Services/Meshing/InequalityBodyBuilder.h"
#include "../../IceCream.Core/Expressions/Inequalities/InequalityParser.h"
#include "../../IceCream.Core/Expressions/Inequalities/InequalityRegion.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <memory>

using namespace IceCream::Core::Services;
using namespace IceCream::Core::Models;
using namespace IceCream::Core::Geometry;

namespace IceCream::App::ViewModels
{
	namespace
	{
		constexpr double unavailable = std::numeric_limits<double>::quiet_NaN();
		std::optional<double> TrySectionArea(const SectionResult& section)
		{
			for (const auto& contour : section.Contours())
				if (!contour.IsClosed() && contour.Points2D().size() > 1) return std::nullopt;
			return section.IsEmpty() ? 0.0 : SectionAreaCalculator().Calculate(section);
		}

		std::string NormalizeConstraints(const std::string& input)
		{
			std::string result;
			int depth = 0;
			bool separator = false;
			for (char c : input)
			{
				if ((c == '\n' || c == '\r' || c == ';') && depth == 0)
				{
					while (!result.empty() && std::isspace(static_cast<unsigned char>(result.back()))) result.pop_back();
					if (!result.empty()) separator = true;
					continue;
				}
				if (std::isspace(static_cast<unsigned char>(c)))
				{
					if (!result.empty() && !separator) result += ' ';
					continue;
				}
				if (separator && c != ',' && result.back() != ',') result += ',';
				separator = false;
				result += c;
				if (c == '(') ++depth;
				if (c == ')') --depth;
			}
			if (result.size() > 4096) throw std::invalid_argument("Ограничения: не более 4096 символов.");
			return result;
		}

		struct PreparedSurface
		{
			TriangleMesh Mesh;
			std::vector<std::array<float, 4>> Colors;
			std::optional<double> Volume;
			bool Clipped;
			std::vector<ConstraintSurface> Constraints;
		};
		PreparedSurface PrepareSurface(TriangleMesh mesh, const std::string& constraints,
			std::optional<double> volume = std::nullopt, std::vector<std::array<float, 4>> colors = {},
			glm::dmat3 rotation = glm::dmat3(1), bool alreadyClipped = false,
			std::optional<std::pair<Point3D, Point3D>> guideBounds = std::nullopt)
		{
			if (mesh.TriangleCount() == 0) throw std::invalid_argument("Поверхность пуста. Проверьте формулу, границы и ограничения.");
			auto text = NormalizeConstraints(constraints);
			std::vector<ConstraintSurface> guides;
			if (!text.empty())
			{
				auto inequalities = Core::Inequalities::InequalityParser::Parse(text);
				for (auto& inequality : inequalities)
					inequality = Core::Inequalities::Inequality([inequality, rotation](double x, double y, double z) {
					auto p = rotation * glm::dvec3(x, y, z);
					return inequality.Evaluate(p.x, p.y, p.z);
						}, Core::Inequalities::InequalityRelation::LessOrEqual, inequality.Label());

				// Cache the guides once per build. Use the original, uncut body's bounds,
				// so the display region does not collapse after clipping.
				auto low = guideBounds ? guideBounds->first : mesh.Vertices().front();
				auto high = guideBounds ? guideBounds->second : low;
				if (!guideBounds)
					for (const auto& p : mesh.Vertices())
					{
						low.X = std::min(low.X, p.X); high.X = std::max(high.X, p.X);
						low.Y = std::min(low.Y, p.Y); high.Y = std::max(high.Y, p.Y);
						low.Z = std::min(low.Z, p.Z); high.Z = std::max(high.Z, p.Z);
					}
				double span = std::max({ high.X - low.X, high.Y - low.Y, high.Z - low.Z, .1 });
				auto padding = Vector3D{ span * .2, span * .2, span * .2 };
				low = low - padding; high = high + padding;
				const std::array<glm::vec3, 6> palette{ {{.2f,.85f,1.f}, {1.f,.55f,.15f}, {.8f,.45f,1.f},
					{.35f,.9f,.45f}, {1.f,.3f,.5f}, {1.f,.85f,.25f}} };
				for (size_t i = 0; i < inequalities.size(); ++i)
				{
					ConstraintSurface guide{ inequalities[i].Label(), palette[i % palette.size()], {} };
					try
					{
						auto surface = ImplicitSurfaceBuilder().Build([&](double x, double y, double z) {
							return inequalities[i].Evaluate(x, y, z);
							}, low, high, 20, 20, 20);
						guide.Triangles.reserve(surface.Indices().size() * 3);
						for (int index : surface.Indices())
						{
							const auto& p = surface.Vertices()[index];
							guide.Triangles.insert(guide.Triangles.end(), { static_cast<float>(p.X), static_cast<float>(p.Y), static_cast<float>(p.Z) });
							guide.Center += glm::vec3(p.X, p.Y, p.Z);
						}
						if (!surface.Indices().empty()) guide.Center /= static_cast<float>(surface.Indices().size());
					}
					catch (const std::invalid_argument&) { guide.Undefined = true; }
					catch (const std::domain_error&) { guide.Undefined = true; }
					guides.push_back(std::move(guide));
				}
				if (!alreadyClipped)
				{
					std::vector<size_t> sources;
					mesh = MeshClipper().Clip(mesh, Core::Inequalities::InequalityRegion(std::move(inequalities)), {},
						colors.empty() ? nullptr : &sources);
					if (!colors.empty())
					{
						std::vector<std::array<float, 4>> clippedColors;
						clippedColors.reserve(sources.size());
						for (size_t source : sources) clippedColors.push_back(colors.at(source));
						colors = std::move(clippedColors);
					}
				}
				volume.reset(); // The original integral no longer describes the clipped mesh.
			}
			if (mesh.TriangleCount() == 0) throw std::invalid_argument("Поверхность пуста. Проверьте формулу, границы и ограничения.");
			if (!volume) volume = VolumeCalculator().TryCalculate(mesh);
			return { std::move(mesh), std::move(colors), volume, !text.empty(), std::move(guides) };
		}

		// Единые проверки для неявной поверхности и границы области по неравенствам.
		void ValidateSurfaceGrid(Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ)
		{
			for (int cells : {cellsX, cellsY, cellsZ})
			{
				if (cells < 2 || cells > 128)
				{
					throw std::invalid_argument("Ячейки по каждой оси: целое число от 2 до 128.");
				}
			}

			if (static_cast<size_t>(cellsX) * cellsY * cellsZ > 1000000)
			{
				throw std::invalid_argument("Сетка: не более 1 000 000 ячеек суммарно.");
			}

			for (double bound : {min.X, min.Y, min.Z, max.X, max.Y, max.Z})
			{
				if (!std::isfinite(bound))
				{
					throw std::invalid_argument("Границы X, Y, Z должны быть конечными числами.");
				}
			}

			if (min.X >= max.X || min.Y >= max.Y || min.Z >= max.Z)
			{
				throw std::invalid_argument("Граница «От» должна быть меньше «До» по каждой оси.");
			}

			if (!std::isfinite(max.X - min.X) || !std::isfinite(max.Y - min.Y) || !std::isfinite(max.Z - min.Z))
			{
				throw std::invalid_argument("Слишком большие размеры области построения.");
			}
		}

		IceCreamParameters DefaultParameters()
		{
			return IceCreamParameters{
				3.0, // BaseHeight
				1.0, // JoinRadius
				0.5, // BaseExponent
				1.2  // CapHeight
			};
		}

		double ClampPositive(double value, double minValue, double maxValue)
		{
			if (!std::isfinite(value))
			{
				throw std::invalid_argument("Expected a finite number.");
			}

			return std::clamp(value, minValue, maxValue);
		}
	}

	IceCreamBody MainViewModel::CreateBody(const IceCreamParameters& parameters, const std::string& formula)
	{
		if (formula == Core::Profiles::FormulaProfile::DefaultFormula())
		{
			return IceCreamBody(parameters, SurfaceMeshBuilder(), 80, 80, 96);
		}

		return IceCreamBody(parameters, std::make_shared<Core::Profiles::FormulaProfile>(formula, parameters), SurfaceMeshBuilder());
	}

	MainViewModel::MainViewModel()
		: parameters_(DefaultParameters()),
		body_(CreateBody(parameters_, formula_)),
		sectionPlane_(Plane3D::AtZ(2.1)),
		section_(sectionPlane_, {})
	{
		RebuildBody();
	}

	void MainViewModel::RebuildBody()
	{
		ApplyShape(parameters_, formula_);
	}

	void MainViewModel::ApplyShape(const IceCreamParameters& parameters, const std::string& formula, const std::string& constraints)
	{
		const double values[] = { parameters.BaseHeight, parameters.JoinRadius, parameters.BaseExponent, parameters.CapHeight };
		const double low[] = { 0.5, 0.2, 0.2, 0.2 }, high[] = { 8, 4, 3, 4 };

		for (int i = 0; i < 4; ++i)
		{
			if (!std::isfinite(values[i]) || values[i] < low[i] || values[i] > high[i])
			{
				throw std::invalid_argument("Shape parameter outside allowed range");
			}
		}

		auto body = CreateBody(parameters, formula);
		double volume = VolumeCalculator().Calculate(body.Profile());
		auto prepared = PrepareSurface(body.Mesh(), constraints, volume);
		if (prepared.Clipped) body = IceCreamBody(parameters, std::move(prepared.Mesh));
		auto nextPlane = CreateSectionPlane(false);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		auto area = TrySectionArea(section);
		std::vector<Point3D> joinBoundary;
		std::optional<TriangleMesh> joinDisk;
		std::vector<SectionContour> joinTrace;

		if (!prepared.Clipped && formula == Core::Profiles::FormulaProfile::DefaultFormula())
		{
			// Границу частей берём из сетки, чтобы она совпадала с сечением.
			auto boundary = MeshSectionCalculator().Calculate(body.Mesh(), Plane3D::AtZ(parameters.BaseHeight));

			if (!boundary.IsEmpty())
			{
				joinBoundary = boundary.Contours().front().Points3D();
			}

			if (joinBoundary.size() >= 3)
			{
				std::vector<Point3D> vertices{ {0, 0, parameters.BaseHeight} };
				vertices.insert(vertices.end(), joinBoundary.begin(), joinBoundary.end());
				std::vector<int> indices;

				for (size_t i = 0; i < joinBoundary.size(); ++i)
				{
					indices.push_back(0);
					indices.push_back(static_cast<int>(i + 1));
					indices.push_back(static_cast<int>((i + 1) % joinBoundary.size() + 1));
				}

				joinDisk.emplace(std::move(vertices), std::move(indices));
				joinTrace = MeshSectionCalculator().Calculate(*joinDisk, nextPlane).Contours();
			}
		}
		// Обновляем модель после успешного завершения расчётов.
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		parameters_ = parameters;
		volume_ = prepared.Volume.value_or(unavailable);
		volumeAvailable_ = prepared.Volume.has_value();
		meshVolume_ = prepared.Clipped;
		implicitSurface_ = false;
		inequalityBody_ = false;
		constraints_ = constraints;
		constraintSurfaces_ = std::move(prepared.Constraints);
		sectionPlane_ = nextPlane;
		section_ = std::move(section);
		sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value();
		joinBoundary_ = std::move(joinBoundary);
		joinDisk_ = std::move(joinDisk);
		joinInSection_ = std::move(joinTrace);
		heightField_ = false;
		surfaceEdges_.clear();
		integralParts_.clear(); triangleColors_.clear(); integralError_ = 0;
		++geometryRevision_;
	}

	void MainViewModel::ApplyHeightField(const Core::Services::HeightFieldBounds& bounds, const std::string& formula, const std::string& constraints)
	{
		auto solid = Core::Services::HeightFieldBuilder().Build(formula, bounds);
		auto rotation = dependentAxis_ == 2 ? glm::dmat3(1) : dependentAxis_ == 0
			? glm::dmat3(0, 1, 0, 0, 0, 1, 1, 0, 0) : glm::dmat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
		auto prepared = PrepareSurface(std::move(solid.Mesh), constraints, solid.Volume, {}, rotation);
		IceCreamBody body(parameters_, std::move(prepared.Mesh));
		auto nextPlane = CreateSectionPlane(true);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		auto area = TrySectionArea(section);
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		sectionPlane_ = nextPlane;
		section_ = std::move(section);
		sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value();
		volume_ = prepared.Volume.value_or(unavailable);
		volumeAvailable_ = prepared.Volume.has_value();
		meshVolume_ = prepared.Clipped;
		implicitSurface_ = false;
		inequalityBody_ = false;
		constraints_ = constraints;
		constraintSurfaces_ = std::move(prepared.Constraints);
		surfaceBounds_ = bounds;
		surfaceBaseZ_ = solid.BaseZ;
		surfaceEdges_ = std::move(solid.Edges);
		if (prepared.Clipped) surfaceEdges_.clear();
		joinBoundary_.clear();
		joinDisk_.reset();
		joinInSection_.clear();
		heightField_ = true;
		integralParts_.clear(); triangleColors_.clear(); integralError_ = 0;
		++geometryRevision_;
	}


	void MainViewModel::ApplyImplicitSurface(const std::string& formula, Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ, const std::string& constraints)
	{
		ValidateSurfaceGrid(min, max, cellsX, cellsY, cellsZ);
		auto expression = Core::Expressions::ExpressionParser::CompileEquation(formula);

		TriangleMesh mesh = ImplicitSurfaceBuilder().Build(expression, min, max, cellsX, cellsY, cellsZ);

		if (mesh.Vertices().empty() || mesh.Indices().empty())
		{
			throw std::invalid_argument("Поверхность пуста. Проверьте уравнение и границы построения.");
		}

		// A clipped closed implicit surface must include the new cut face.
		// Intersect the solid F<=0 with all constraints before triangulating,
		// instead of deleting old surface triangles and leaving an open hole.
		bool alreadyClipped = false;
		const std::string normalized = NormalizeConstraints(constraints);
		std::optional<std::pair<Point3D, Point3D>> guideBounds;
		if (!normalized.empty())
		{
			auto low = mesh.Vertices().front(), high = low;
			for (const auto& p : mesh.Vertices())
			{
				low.X = std::min(low.X, p.X); high.X = std::max(high.X, p.X);
				low.Y = std::min(low.Y, p.Y); high.Y = std::max(high.Y, p.Y);
				low.Z = std::min(low.Z, p.Z); high.Z = std::max(high.Z, p.Z);
			}
			guideBounds = std::make_pair(low, high);
		}
		if (!normalized.empty() && VolumeCalculator().TryCalculate(mesh))
		{
			// The sign of an implicit equation is arbitrary (F=0 and -F=0).
			// For a closed surface, the box corners must lie on the exterior side.
			const double corner = expression(min.X, min.Y, min.Z);
			bool exteriorKnown = std::isfinite(corner) && corner != 0.0;
			for (double x : {min.X, max.X})
				for (double y : {min.Y, max.Y})
					for (double z : {min.Z, max.Z})
					{
						double value = expression(x, y, z);
						if (!std::isfinite(value) || (value > 0.0) != (corner > 0.0)) exteriorKnown = false;
					}

			if (exteriorKnown)
			{
				auto region = Core::Inequalities::InequalityRegion(Core::Inequalities::InequalityParser::Parse(normalized));
				const double sign = corner < 0.0 ? -1.0 : 1.0;
				mesh = ImplicitSurfaceBuilder().Build([&](double x, double y, double z)
					{
						return std::max(sign * expression(x, y, z), region.Evaluate(x, y, z));
					}, min, max, cellsX, cellsY, cellsZ);
				if (mesh.TriangleCount() == 0)
					throw std::invalid_argument("Поверхность пуста. Проверьте уравнение, границы и ограничения.");
				alreadyClipped = true;
			}
		}

		auto prepared = PrepareSurface(std::move(mesh), constraints, std::nullopt, {}, glm::dmat3(1), alreadyClipped, guideBounds);
		IceCreamBody body(parameters_, std::move(prepared.Mesh));

		auto nextPlane = CreateSectionPlane(false);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		auto area = TrySectionArea(section);
		std::string acceptedFormula = formula;

		body_ = std::move(body);
		sectionPlane_ = nextPlane;
		section_ = std::move(section);
		sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value();
		formula_ = std::move(acceptedFormula);
		constraints_ = constraints;
		constraintSurfaces_ = std::move(prepared.Constraints);

		volume_ = prepared.Volume.value_or(unavailable);
		volumeAvailable_ = prepared.Volume.has_value();
		implicitSurface_ = meshVolume_ = true;
		inequalityBody_ = false;
		heightField_ = false;
		dependentAxis_ = 2;

		surfaceEdges_.clear();
		joinBoundary_.clear();
		joinDisk_.reset();
		joinInSection_.clear();

		integralParts_.clear();
		triangleColors_.clear();
		integralError_ = 0.0;

		++geometryRevision_;
	}


	void MainViewModel::EstimateImplicitBounds(const std::string& formula, Point3D& min, Point3D& max) const
	{
		ValidateSurfaceGrid(min, max, 2, 2, 2);
		auto expression = Core::Expressions::ExpressionParser::CompileEquation(formula);

		// Для уравнения ищем нулевой уровень F(x,y,z)=0, а не всю область F<=0.
		// Если обнаруженная поверхность упирается в границы поиска, расширяем коробку.
		constexpr int samples = 48;
		constexpr int row = samples + 1;
		constexpr int maxAttempts = 6;
		std::vector<double> values(static_cast<size_t>(row) * row * row);

		Point3D searchMin = min;
		Point3D searchMax = max;

		auto index = [](int i, int j, int k) -> size_t
			{
				return (static_cast<size_t>(k) * row + j) * row + i;
			};

		for (int attempt = 0; attempt < maxAttempts; ++attempt)
		{
			double sizeX = searchMax.X - searchMin.X;
			double sizeY = searchMax.Y - searchMin.Y;
			double sizeZ = searchMax.Z - searchMin.Z;
			ValidateSurfaceGrid(searchMin, searchMax, 2, 2, 2);

			auto point = [&](int i, int j, int k) -> Point3D
				{
					return {
						searchMin.X + sizeX * (static_cast<double>(i) / samples),
						searchMin.Y + sizeY * (static_cast<double>(j) / samples),
						searchMin.Z + sizeZ * (static_cast<double>(k) / samples)
					};
				};

			for (int k = 0; k <= samples; ++k)
			{
				for (int j = 0; j <= samples; ++j)
				{
					for (int i = 0; i <= samples; ++i)
					{
						Point3D p = point(i, j, k);
						double value = expression(p.X, p.Y, p.Z);
						if (!std::isfinite(value))
						{
							throw std::invalid_argument("Функция принимает недопустимое значение в области поиска.");
						}
						values[index(i, j, k)] = value;
					}
				}
			}

			Point3D foundMin{ std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
			Point3D foundMax{ -foundMin.X, -foundMin.Y, -foundMin.Z };
			bool found = false;
			bool touchesBoundary = false;

			auto include = [&](const Point3D& p)
				{
					found = true;
					foundMin.X = std::min(foundMin.X, p.X);
					foundMin.Y = std::min(foundMin.Y, p.Y);
					foundMin.Z = std::min(foundMin.Z, p.Z);
					foundMax.X = std::max(foundMax.X, p.X);
					foundMax.Y = std::max(foundMax.Y, p.Y);
					foundMax.Z = std::max(foundMax.Z, p.Z);
				};

			for (int k = 0; k <= samples; ++k)
			{
				for (int j = 0; j <= samples; ++j)
				{
					for (int i = 0; i <= samples; ++i)
					{
						double a = values[index(i, j, k)];
						Point3D p = point(i, j, k);
						bool boundary = i == 0 || i == samples || j == 0 || j == samples || k == 0 || k == samples;

						if (a == 0.0)
						{
							include(p);
							if (boundary) touchesBoundary = true;
						}

						auto checkEdge = [&](int ni, int nj, int nk)
							{
								double b = values[index(ni, nj, nk)];
								if ((a < 0.0) == (b < 0.0) || a == 0.0 || b == 0.0) return;

								// Линейная интерполяция достаточна для приближённой оценки границ.
								double t = a / (a - b);
								Point3D q = point(ni, nj, nk);
								Point3D intersection{
									p.X + t * (q.X - p.X),
									p.Y + t * (q.Y - p.Y),
									p.Z + t * (q.Z - p.Z)
								};
								include(intersection);

								// Ребро, целиком лежащее на одной из шести граней коробки.
								if ((i == ni && (i == 0 || i == samples)) ||
									(j == nj && (j == 0 || j == samples)) ||
									(k == nk && (k == 0 || k == samples)))
								{
									touchesBoundary = true;
								}
							};

						if (i < samples) checkEdge(i + 1, j, k);
						if (j < samples) checkEdge(i, j + 1, k);
						if (k < samples) checkEdge(i, j, k + 1);
					}
				}
			}

			if (found && !touchesBoundary)
			{
				auto padding = [](double searchSize, double surfaceSize)
					{
						return std::max(2.0 * searchSize / samples, 0.04 * surfaceSize);
					};

				double padX = padding(sizeX, foundMax.X - foundMin.X);
				double padY = padding(sizeY, foundMax.Y - foundMin.Y);
				double padZ = padding(sizeZ, foundMax.Z - foundMin.Z);
				Point3D fittedMin{ foundMin.X - padX, foundMin.Y - padY, foundMin.Z - padZ };
				Point3D fittedMax{ foundMax.X + padX, foundMax.Y + padY, foundMax.Z + padZ };
				ValidateSurfaceGrid(fittedMin, fittedMax, 2, 2, 2);
				min = fittedMin;
				max = fittedMax;
				return;
			}

			if (attempt + 1 == maxAttempts)
			{
				if (touchesBoundary)
				{
					throw std::invalid_argument("Поверхность пересекает границу области поиска или не ограничена. Автоподбор невозможен.");
				}
				throw std::invalid_argument("Поверхность в исследованной области не найдена. Задайте другую область поиска.");
			}

			// Увеличиваем каждую сторону коробки вдвое относительно её центра.
			auto expand = [](double low, double high) -> std::pair<double, double>
				{
					double extent = high - low;
					return { low - extent / 2.0, high + extent / 2.0 };
				};
			auto [minX, maxX] = expand(searchMin.X, searchMax.X);
			auto [minY, maxY] = expand(searchMin.Y, searchMax.Y);
			auto [minZ, maxZ] = expand(searchMin.Z, searchMax.Z);
			searchMin = { minX, minY, minZ };
			searchMax = { maxX, maxY, maxZ };
		}
	}

	void MainViewModel::EstimateInequalityBounds(const std::string& inequalities, Point3D& min, Point3D& max) const
	{
		// Входные min/max задают начальную область поиска, а не готовые границы тела.
		ValidateSurfaceGrid(min, max, 2, 2, 2);

		std::string text = NormalizeConstraints(inequalities);
		if (text.empty())
		{
			throw std::invalid_argument("Введите систему неравенств.");
		}

		Core::Inequalities::InequalityRegion region(Core::Inequalities::InequalityParser::Parse(text));
		constexpr int samples = 48;
		constexpr int row = samples + 1;
		constexpr int maxAttempts = 7;
		std::vector<double> values(static_cast<size_t>(row) * row * row);

		Point3D searchMin = min;
		Point3D searchMax = max;

		auto index = [](int i, int j, int k) -> size_t
			{
				return (static_cast<size_t>(k) * row + j) * row + i;
			};

		for (int attempt = 0; attempt < maxAttempts; ++attempt)
		{
			ValidateSurfaceGrid(searchMin, searchMax, 2, 2, 2);
			double sizeX = searchMax.X - searchMin.X;
			double sizeY = searchMax.Y - searchMin.Y;
			double sizeZ = searchMax.Z - searchMin.Z;

			auto point = [&](int i, int j, int k) -> Point3D
				{
					return {
						i == samples ? searchMax.X : searchMin.X + sizeX * (static_cast<double>(i) / samples),
						j == samples ? searchMax.Y : searchMin.Y + sizeY * (static_cast<double>(j) / samples),
						k == samples ? searchMax.Z : searchMin.Z + sizeZ * (static_cast<double>(k) / samples)
					};
				};

			Point3D foundMin{ std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
			Point3D foundMax{ -foundMin.X, -foundMin.Y, -foundMin.Z };
			bool found = false;
			bool clippedMinX = false, clippedMaxX = false;
			bool clippedMinY = false, clippedMaxY = false;
			bool clippedMinZ = false, clippedMaxZ = false;

			auto include = [&](Point3D p)
				{
					found = true;
					foundMin.X = std::min(foundMin.X, p.X);
					foundMin.Y = std::min(foundMin.Y, p.Y);
					foundMin.Z = std::min(foundMin.Z, p.Z);
					foundMax.X = std::max(foundMax.X, p.X);
					foundMax.Y = std::max(foundMax.Y, p.Y);
					foundMax.Z = std::max(foundMax.Z, p.Z);
				};

			for (int k = 0; k <= samples; ++k)
			{
				for (int j = 0; j <= samples; ++j)
				{
					for (int i = 0; i <= samples; ++i)
					{
						Point3D p = point(i, j, k);
						double value = region.Evaluate(p.X, p.Y, p.Z);
						values[index(i, j, k)] = value;

						if (value <= 0.0) include(p);
						// Отрицательное значение на грани коробки означает, что тело обрезано.
						// Нулевое значение допускаем: реальная граница может совпадать с плоскостью коробки.
						if (value < -1e-10)
						{
							if (i == 0) clippedMinX = true;
							if (i == samples) clippedMaxX = true;
							if (j == 0) clippedMinY = true;
							if (j == samples) clippedMaxY = true;
							if (k == 0) clippedMinZ = true;
							if (k == samples) clippedMaxZ = true;
						}
					}
				}
			}

			bool clipped = clippedMinX || clippedMaxX || clippedMinY || clippedMaxY || clippedMinZ || clippedMaxZ;
			if (clipped || !found)
			{
				if (attempt + 1 == maxAttempts)
				{
					throw std::invalid_argument("Не удалось охватить всю область: тело не ограничено, выходит за пределы поиска или слишком мало для предварительной сетки.");
				}

				// Расширяем только те стороны, через которые тело действительно выходит.
				// Если ничего не найдено, увеличиваем всю коробку для повторного поиска.
				if (!found) clippedMinX = clippedMaxX = clippedMinY = clippedMaxY = clippedMinZ = clippedMaxZ = true;
				if (clippedMinX) searchMin.X -= sizeX * 0.5;
				if (clippedMaxX) searchMax.X += sizeX * 0.5;
				if (clippedMinY) searchMin.Y -= sizeY * 0.5;
				if (clippedMaxY) searchMax.Y += sizeY * 0.5;
				if (clippedMinZ) searchMin.Z -= sizeZ * 0.5;
				if (clippedMaxZ) searchMax.Z += sizeZ * 0.5;
				continue;
			}

			// Уточняем положение обнаруженных границ интерполяцией на рёбрах грубой сетки.
			for (int k = 0; k <= samples; ++k)
			{
				for (int j = 0; j <= samples; ++j)
				{
					for (int i = 0; i <= samples; ++i)
					{
						double a = values[index(i, j, k)];
						auto includeEdge = [&](int ni, int nj, int nk)
							{
								double b = values[index(ni, nj, nk)];
								if ((a <= 0.0) == (b <= 0.0)) return;
								double t = a / (a - b);
								Point3D p = point(i, j, k);
								Point3D q = point(ni, nj, nk);
								include({ p.X + t * (q.X - p.X), p.Y + t * (q.Y - p.Y), p.Z + t * (q.Z - p.Z) });
							};

						if (i < samples) includeEdge(i + 1, j, k);
						if (j < samples) includeEdge(i, j + 1, k);
						if (k < samples) includeEdge(i, j, k + 1);
					}
				}
			}

			auto padding = [](double searchSize, double bodySize)
				{
					return std::max(2.0 * searchSize / samples, 0.04 * bodySize);
				};

			double padX = padding(sizeX, foundMax.X - foundMin.X);
			double padY = padding(sizeY, foundMax.Y - foundMin.Y);
			double padZ = padding(sizeZ, foundMax.Z - foundMin.Z);
			Point3D fittedMin{ foundMin.X - padX, foundMin.Y - padY, foundMin.Z - padZ };
			Point3D fittedMax{ foundMax.X + padX, foundMax.Y + padY, foundMax.Z + padZ };
			ValidateSurfaceGrid(fittedMin, fittedMax, 2, 2, 2);
			min = fittedMin;
			max = fittedMax;
			return;
		}
	}

	void MainViewModel::ApplyInequalityBody(const std::string& inequalities, Point3D min, Point3D max,
		int cellsX, int cellsY, int cellsZ)
	{
		ValidateSurfaceGrid(min, max, cellsX, cellsY, cellsZ);

		std::string text = NormalizeConstraints(inequalities);
		if (text.empty())
		{
			throw std::invalid_argument("Введите хотя бы одно неравенство для построения тела.");
		}

		auto region = Core::Inequalities::InequalityRegion(Core::Inequalities::InequalityParser::Parse(text));
		TriangleMesh mesh = InequalityBodyBuilder().Build(region, min, max, cellsX, cellsY, cellsZ);

		if (mesh.TriangleCount() == 0)
		{
			throw std::invalid_argument("Граница области не найдена. Проверьте неравенства и границы сетки.");
		}

		// Не обрезаем полученную сетку: она уже представляет полную границу области.
		auto prepared = PrepareSurface(std::move(mesh), "");
		IceCreamBody body(parameters_, std::move(prepared.Mesh));
		auto nextPlane = CreateSectionPlane(false);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		auto area = TrySectionArea(section);

		// Изменяем состояние только после успешного завершения всех вычислений.
		body_ = std::move(body);
		sectionPlane_ = nextPlane;
		section_ = std::move(section);
		sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value();
		volume_ = prepared.Volume.value_or(unavailable);
		volumeAvailable_ = prepared.Volume.has_value();
		meshVolume_ = true;
		implicitSurface_ = false;
		inequalityBody_ = true;
		constraints_ = inequalities;
		constraintSurfaces_.clear();
		heightField_ = false;
		dependentAxis_ = 2;
		surfaceEdges_.clear();
		joinBoundary_.clear();
		joinDisk_.reset();
		joinInSection_.clear();
		integralParts_.clear();
		triangleColors_.clear();
		integralError_ = 0.0;
		++geometryRevision_;
	}

	Plane3D MainViewModel::CreateSectionPlane(bool surface) const
	{
		// Поворачиваем плоскость XY вокруг её начала по глобальным осям X, Y, Z.
		const double k = std::acos(-1.0) / 180.0;
		const double x = sectionRotation_.X * k;
		const double y = sectionRotation_.Y * k;
		const double z = sectionRotation_.Z * k;
		const Vector3D normal{
			std::cos(z) * std::sin(y) * std::cos(x) + std::sin(z) * std::sin(x),
			std::sin(z) * std::sin(y) * std::cos(x) - std::cos(z) * std::sin(x),
			std::cos(y) * std::cos(x) };
		const auto inverse = glm::transpose(!surface || dependentAxis_ == 2 ? glm::dmat3(1) : dependentAxis_ == 0 ? glm::dmat3(0, 1, 0, 0, 0, 1, 1, 0, 0) : glm::dmat3(0, 0, 1, 1, 0, 0, 0, 1, 0));
		const auto o = inverse * glm::dvec3(sectionPosition_.X, sectionPosition_.Y, sectionPosition_.Z);
		const auto n = inverse * glm::dvec3(normal.X, normal.Y, normal.Z);
		return Plane3D({ o.x,o.y,o.z }, { n.x,n.y,n.z });
	}

	void MainViewModel::RecalculateSection()
	{
		auto plane = CreateSectionPlane(heightField_);
		auto section = MeshSectionCalculator().Calculate(body_.Mesh(), plane);
		auto area = TrySectionArea(section);
		std::vector<SectionContour> join;
		if (joinDisk_) join = MeshSectionCalculator().Calculate(*joinDisk_, plane).Contours();
		sectionPlane_ = plane; section_ = std::move(section); sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value(); joinInSection_ = std::move(join);
	}

	void MainViewModel::SetBaseHeight(double value)
	{
		auto parameters = parameters_;
		parameters.BaseHeight = ClampPositive(value, 0.5, 8.0);
		ApplyShape(parameters, formula_);
	}

	void MainViewModel::SetJoinRadius(double value)
	{
		auto parameters = parameters_;
		parameters.JoinRadius = ClampPositive(value, 0.2, 4.0);
		ApplyShape(parameters, formula_);
	}

	void MainViewModel::SetBaseExponent(double value)
	{
		auto parameters = parameters_;
		parameters.BaseExponent = ClampPositive(value, 0.2, 3.0);
		ApplyShape(parameters, formula_);
	}

	void MainViewModel::SetCapHeight(double value)
	{
		auto parameters = parameters_;
		parameters.CapHeight = ClampPositive(value, 0.2, 4.0);
		ApplyShape(parameters, formula_);
	}

	void MainViewModel::ResetDefaults()
	{
		const bool integral = IsIntegralBody() || IsImplicitSurface() || IsInequalityBody();
		dependentAxis_ = 2;
		formula_ = Core::Profiles::FormulaProfile::DefaultFormula();
		parameters_ = DefaultParameters();
		transparentPositive_ = true;
		sectionPosition_ = { 0.0, 0.0, 2.1 };
		sectionRotation_ = {};
		RebuildBody();
		RecalculateSection();
		if (integral) ApplyIntegralParts(IntegralBodyBuilder::DefaultParts());
	}

	void MainViewModel::SetSectionPosition(int axis, double value)
	{
		const auto previous = sectionPosition_;
		if (axis < 0 || axis > 2 || !std::isfinite(value) || std::abs(value) > SectionPositionLimit)
		{
			throw std::out_of_range("Положение плоскости должно быть от −1000 до 1000.");
		}

		if (axis == 0)
		{
			sectionPosition_.X = value;
		}

		if (axis == 1)
		{
			sectionPosition_.Y = value;
		}

		if (axis == 2)
		{
			sectionPosition_.Z = value;
		}

		try { RecalculateSection(); }
		catch (...) { sectionPosition_ = previous; throw; }
	}

	void MainViewModel::SetSectionPose(Point3D position, Vector3D rotation)
	{
		for (double value : {position.X, position.Y, position.Z})
			if (!std::isfinite(value) || std::abs(value) > SectionPositionLimit) throw std::out_of_range("Invalid section position");
		for (double value : {rotation.X, rotation.Y, rotation.Z})
			if (!std::isfinite(value) || std::abs(value) > 180) throw std::out_of_range("Invalid section rotation");
		const auto oldPosition = sectionPosition_;
		const auto oldRotation = sectionRotation_;
		sectionPosition_ = position; sectionRotation_ = rotation;
		try { RecalculateSection(); }
		catch (...) { sectionPosition_ = oldPosition; sectionRotation_ = oldRotation; throw; }
	}

	void MainViewModel::SetSectionRotation(int axis, double degrees)
	{
		const auto previous = sectionRotation_;
		if (axis < 0 || axis > 2 || !std::isfinite(degrees))
		{
			throw std::invalid_argument("Invalid rotation.");
		}

		degrees = std::remainder(degrees, 360.0);

		if (axis == 0)
		{
			sectionRotation_.X = degrees;
		}

		if (axis == 1)
		{
			sectionRotation_.Y = degrees;
		}

		if (axis == 2)
		{
			sectionRotation_.Z = degrees;
		}

		try { RecalculateSection(); }
		catch (...) { sectionRotation_ = previous; throw; }
	}

	void MainViewModel::SetSectionPreset(int normalAxis)
	{
		const auto previous = sectionRotation_;
		if (normalAxis < 0 || normalAxis > 2)
		{
			throw std::out_of_range("Invalid axis.");
		}

		sectionRotation_ = {};

		if (normalAxis == 0)
		{
			sectionRotation_.Y = 90;
		}

		if (normalAxis == 1)
		{
			sectionRotation_.X = -90;
		}

		try { RecalculateSection(); }
		catch (...) { sectionRotation_ = previous; throw; }
	}
	// свет
	void MainViewModel::SetLighting(const Views::Graphics::LightingSettings& lighting)
	{
		lighting_ = lighting;
	}

	glm::dmat3 MainViewModel::SurfaceRotation() const
	{
		if (!heightField_ || dependentAxis_ == 2) return glm::dmat3(1);
		if (dependentAxis_ == 0) return glm::dmat3(0, 1, 0, 0, 0, 1, 1, 0, 0);
		return glm::dmat3(0, 0, 1, 1, 0, 0, 0, 1, 0);
	}

	void MainViewModel::SetDependentAxis(int axis)
	{
		if (axis < 0 || axis>2) throw std::out_of_range("Invalid dependent axis");
		const int previous = dependentAxis_;
		dependentAxis_ = axis;
		try { RecalculateSection(); }
		catch (...) { dependentAxis_ = previous; throw; }
	}

	void MainViewModel::SetAppearance(const AppearanceSettings& settings)
	{
		if (settings.CoordinateSize < 12 || settings.CoordinateSize>36 || settings.HintSize < 10 || settings.HintSize>24)
			throw std::out_of_range("Invalid text size");
		for (auto color : settings.Colors) for (int i = 0; i < 4; ++i)
			if (!std::isfinite(color[i]) || color[i] < 0 || color[i]>1) throw std::invalid_argument("Invalid color");
		appearance_ = settings;
	}
	void MainViewModel::ApplyIntegralParts(const std::vector<IntegralPart>& parts, const std::string& constraints)
	{
		auto accepted = parts;
		auto solid = IntegralBodyBuilder().Build(parts);
		auto prepared = PrepareSurface(std::move(solid.Mesh), constraints, solid.Integral, std::move(solid.TriangleColors));
		IceCreamBody body(parameters_, std::move(prepared.Mesh));
		auto plane = CreateSectionPlane(false);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), plane);
		auto area = TrySectionArea(section);
		body_ = std::move(body); sectionPlane_ = plane; section_ = std::move(section); sectionArea_ = area.value_or(unavailable);
		sectionAreaAvailable_ = area.has_value();
		volume_ = prepared.Volume.value_or(unavailable); volumeAvailable_ = prepared.Volume.has_value();
		meshVolume_ = prepared.Clipped; implicitSurface_ = false; inequalityBody_ = false; constraints_ = constraints;
		constraintSurfaces_ = std::move(prepared.Constraints);
		integralError_ = prepared.Clipped ? 0 : solid.EstimatedError; integralParts_ = std::move(accepted);
		triangleColors_ = std::move(prepared.Colors); surfaceEdges_ = prepared.Clipped ? std::vector<std::vector<Point3D>>{} : std::move(solid.Seams);
		heightField_ = false; dependentAxis_ = 2; joinDisk_.reset(); joinBoundary_.clear(); joinInSection_.clear();
		++geometryRevision_;
	}
}
