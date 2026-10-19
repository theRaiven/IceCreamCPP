#include "MainViewModel.h"

#include "../../IceCream.Core/Services/SurfaceMeshBuilder.h"
#include "../../IceCream.Core/Services/VolumeCalculator.h"
#include "../../IceCream.Core/Services/MeshSectionCalculator.h"
#include "../../IceCream.Core/Services/SectionAreaCalculator.h"

#include <algorithm>
#include <cmath>

using namespace IceCream::Core::Services;
using namespace IceCream::Core::Models;
using namespace IceCream::Core::Geometry;

namespace IceCream::App::ViewModels
{
	namespace
	{
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

	void MainViewModel::ApplyShape(const IceCreamParameters& parameters, const std::string& formula)
	{
		const double values[] = {parameters.BaseHeight, parameters.JoinRadius, parameters.BaseExponent, parameters.CapHeight};
		const double low[] = {0.5, 0.2, 0.2, 0.2}, high[] = {8, 4, 3, 4};

		for (int i = 0; i < 4; ++i)
		{
			if (!std::isfinite(values[i]) || values[i] < low[i] || values[i] > high[i])
			{
				throw std::invalid_argument("Shape parameter outside allowed range");
			}
		}

		auto body = CreateBody(parameters, formula);
		double volume = VolumeCalculator().Calculate(body.Profile());
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), sectionPlane_);
		double area = section.IsEmpty() ? 0 : SectionAreaCalculator().Calculate(section);
		std::vector<Point3D> joinBoundary;
		std::optional<TriangleMesh> joinDisk;
		std::vector<SectionContour> joinTrace;

		if (formula == Core::Profiles::FormulaProfile::DefaultFormula())
		{
			// Границу частей берём из сетки, чтобы она совпадала с сечением.
			auto boundary = MeshSectionCalculator().Calculate(body.Mesh(), Plane3D::AtZ(parameters.BaseHeight));

			if (!boundary.IsEmpty())
			{
				joinBoundary = boundary.Contours().front().Points3D();
			}

			if (joinBoundary.size() >= 3)
			{
				std::vector<Point3D> vertices{{0, 0, parameters.BaseHeight}};
				vertices.insert(vertices.end(), joinBoundary.begin(), joinBoundary.end());
				std::vector<int> indices;

				for (size_t i = 0; i < joinBoundary.size(); ++i)
				{
					indices.push_back(0);
					indices.push_back(static_cast<int>(i + 1));
					indices.push_back(static_cast<int>((i + 1) % joinBoundary.size() + 1));
				}

				joinDisk.emplace(std::move(vertices), std::move(indices));
				joinTrace = MeshSectionCalculator().Calculate(*joinDisk, sectionPlane_).Contours();
			}
		}
		// Обновляем модель после успешного завершения расчётов.
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		parameters_ = parameters;
		volume_ = volume;
		section_ = std::move(section);
		sectionArea_ = area;
		joinBoundary_ = std::move(joinBoundary);
		joinDisk_ = std::move(joinDisk);
		joinInSection_ = std::move(joinTrace);
		heightField_ = false;
		surfaceEdges_.clear();
		++geometryRevision_;
	}

	void MainViewModel::ApplyHeightField(const Core::Services::HeightFieldBounds& bounds, const std::string& formula)
	{
		auto solid = Core::Services::HeightFieldBuilder().Build(formula, bounds);
		IceCreamBody body(parameters_, std::move(solid.Mesh));
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), sectionPlane_);
		double area = section.IsEmpty() ? 0 : SectionAreaCalculator().Calculate(section);
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		section_ = std::move(section);
		sectionArea_ = area;
		volume_ = solid.Volume;
		surfaceBounds_ = bounds;
		surfaceBaseZ_ = solid.BaseZ;
		surfaceEdges_ = std::move(solid.Edges);
		joinBoundary_.clear();
		joinDisk_.reset();
		joinInSection_.clear();
		heightField_ = true;
		++geometryRevision_;
	}

	void MainViewModel::RecalculateSection()
	{
		// Поворачиваем плоскость XY вокруг её начала по глобальным осям X, Y, Z.
		const double k = std::acos(-1.0) / 180.0;
		const double x = sectionRotation_.X * k;
		const double y = sectionRotation_.Y * k;
		const double z = sectionRotation_.Z * k;
		const Vector3D normal{
		    std::cos(z) * std::sin(y) * std::cos(x) + std::sin(z) * std::sin(x),
		    std::sin(z) * std::sin(y) * std::cos(x) - std::cos(z) * std::sin(x),
		    std::cos(y) * std::cos(x)};
		sectionPlane_ = Plane3D(sectionPosition_, normal);
		section_ = MeshSectionCalculator().Calculate(body_.Mesh(), sectionPlane_);
		sectionArea_ = section_.IsEmpty() ? 0.0 : SectionAreaCalculator().Calculate(section_);
		joinInSection_.clear();

		if (joinDisk_)
		{
			joinInSection_ = MeshSectionCalculator().Calculate(*joinDisk_, sectionPlane_).Contours();
		}
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
		formula_ = Core::Profiles::FormulaProfile::DefaultFormula();
		parameters_ = DefaultParameters();
		transparentPositive_ = true;
		sectionPosition_ = {0.0, 0.0, 2.1};
		sectionRotation_ = {};
		RebuildBody();
		RecalculateSection();
	}

	void MainViewModel::SetSectionPosition(int axis, double value)
	{
		if (axis < 0 || axis > 2 || !std::isfinite(value) || value < -20 || value > 20)
		{
			throw std::out_of_range("Position must be between -20 and 20.");
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

		RecalculateSection();
	}

	void MainViewModel::SetSectionRotation(int axis, double degrees)
	{
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

		RecalculateSection();
	}

	void MainViewModel::SetSectionPreset(int normalAxis)
	{
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
			sectionRotation_.X = 90;
		}

		RecalculateSection();
	}
	// свет
	void MainViewModel::SetLighting(const Views::Graphics::LightingSettings& lighting)
	{
		lighting_ = lighting;
	}
}
