#include "MainViewModel.h"

#include "../../IceCream.Core/Services/Meshing/SurfaceMeshBuilder.h"
#include "../../IceCream.Core/Services/Measurements/VolumeCalculator.h"
#include "../../IceCream.Core/Services/Measurements/SectionAreaCalculator.h"
#include "../../IceCream.Core/Services/Sections/MeshSectionCalculator.h"

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
		auto nextPlane = CreateSectionPlane(false);
        auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
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
				joinTrace = MeshSectionCalculator().Calculate(*joinDisk, nextPlane).Contours();
			}
		}
		// Обновляем модель после успешного завершения расчётов.
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		parameters_ = parameters;
		volume_ = volume;
		sectionPlane_ = nextPlane;
        section_ = std::move(section);
		sectionArea_ = area;
		joinBoundary_ = std::move(joinBoundary);
		joinDisk_ = std::move(joinDisk);
		joinInSection_ = std::move(joinTrace);
		heightField_ = false;
		surfaceEdges_.clear();
		integralParts_.clear(); triangleColors_.clear();
        ++geometryRevision_;
	}

	void MainViewModel::ApplyHeightField(const Core::Services::HeightFieldBounds& bounds, const std::string& formula)
	{
		auto solid = Core::Services::HeightFieldBuilder().Build(formula, bounds);
		IceCreamBody body(parameters_, std::move(solid.Mesh));
		auto nextPlane = CreateSectionPlane(true);
        auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		double area = section.IsEmpty() ? 0 : SectionAreaCalculator().Calculate(section);
		std::string acceptedFormula = formula;
		body_ = std::move(body);
		formula_ = std::move(acceptedFormula);
		sectionPlane_ = nextPlane;
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
		integralParts_.clear(); triangleColors_.clear();
        ++geometryRevision_;
	}


	void MainViewModel::ApplyImplicitSurface(const std::string& formula, Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ)
	{
		auto expression = Core::Expressions::ExpressionParser::Compile(formula);

		TriangleMesh mesh = ImplicitSurfaceBuilder().Build(expression, min, max, cellsX, cellsY, cellsZ);

		if (mesh.Vertices().empty() || mesh.Indices().empty())
		{
			throw std::invalid_argument("Implicit surface is empty");
		}

		IceCreamBody body(parameters_, std::move(mesh));

		auto nextPlane = CreateSectionPlane(false);
		auto section = MeshSectionCalculator().Calculate(body.Mesh(), nextPlane);
		double area = section.IsEmpty() ? 0.0 : SectionAreaCalculator().Calculate(section);

		body_ = std::move(body);
		sectionPlane_ = nextPlane;
		section_ = std::move(section);
		sectionArea_ = area;

		volume_ = 0.0;
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
		    std::cos(y) * std::cos(x)};
		const auto inverse = glm::transpose(!surface || dependentAxis_==2 ? glm::dmat3(1) : dependentAxis_==0 ? glm::dmat3(0,1,0, 0,0,1, 1,0,0) : glm::dmat3(0,0,1, 1,0,0, 0,1,0));
		const auto o = inverse * glm::dvec3(sectionPosition_.X, sectionPosition_.Y, sectionPosition_.Z);
		const auto n = inverse * glm::dvec3(normal.X, normal.Y, normal.Z);
        return Plane3D({o.x,o.y,o.z}, {n.x,n.y,n.z});
    }

    void MainViewModel::RecalculateSection()
    {
        auto plane=CreateSectionPlane(heightField_);
        auto section=MeshSectionCalculator().Calculate(body_.Mesh(),plane);
        double area=section.IsEmpty()?0:SectionAreaCalculator().Calculate(section);
        std::vector<SectionContour> join;
        if(joinDisk_) join=MeshSectionCalculator().Calculate(*joinDisk_,plane).Contours();
        sectionPlane_=plane;section_=std::move(section);sectionArea_=area;joinInSection_=std::move(join);
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
		const bool integral=IsIntegralBody();
        dependentAxis_ = 2;
		formula_ = Core::Profiles::FormulaProfile::DefaultFormula();
		parameters_ = DefaultParameters();
		transparentPositive_ = true;
		sectionPosition_ = {0.0, 0.0, 2.1};
		sectionRotation_ = {};
		RebuildBody();
		RecalculateSection();
        if(integral) ApplyIntegralParts(IntegralBodyBuilder::DefaultParts());
	}

	void MainViewModel::SetSectionPosition(int axis, double value)
	{
        const auto previous=sectionPosition_;
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

		try { RecalculateSection(); } catch (...) { sectionPosition_=previous; throw; }
	}

    void MainViewModel::SetSectionPose(Point3D position, Vector3D rotation)
    {
        for(double value : {position.X,position.Y,position.Z})
            if(!std::isfinite(value) || std::abs(value)>20) throw std::out_of_range("Invalid section position");
        for(double value : {rotation.X,rotation.Y,rotation.Z})
            if(!std::isfinite(value) || std::abs(value)>180) throw std::out_of_range("Invalid section rotation");
        const auto oldPosition=sectionPosition_;
        const auto oldRotation=sectionRotation_;
        sectionPosition_=position; sectionRotation_=rotation;
        try { RecalculateSection(); }
        catch (...) { sectionPosition_=oldPosition;sectionRotation_=oldRotation;throw; }
    }

	void MainViewModel::SetSectionRotation(int axis, double degrees)
	{
        const auto previous=sectionRotation_;
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

		try { RecalculateSection(); } catch (...) { sectionRotation_=previous; throw; }
	}

	void MainViewModel::SetSectionPreset(int normalAxis)
	{
        const auto previous=sectionRotation_;
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

		try { RecalculateSection(); } catch (...) { sectionRotation_=previous; throw; }
	}
	// свет
	void MainViewModel::SetLighting(const Views::Graphics::LightingSettings& lighting)
	{
		lighting_ = lighting;
	}

	glm::dmat3 MainViewModel::SurfaceRotation() const
	{
		if (!heightField_ || dependentAxis_ == 2) return glm::dmat3(1);
		if (dependentAxis_ == 0) return glm::dmat3(0,1,0, 0,0,1, 1,0,0);
		return glm::dmat3(0,0,1, 1,0,0, 0,1,0);
	}

	void MainViewModel::SetDependentAxis(int axis)
	{
		if (axis<0 || axis>2) throw std::out_of_range("Invalid dependent axis");
		const int previous=dependentAxis_;
        dependentAxis_=axis;
        try { RecalculateSection(); } catch (...) { dependentAxis_=previous; throw; }
	}

	void MainViewModel::SetAppearance(const AppearanceSettings& settings)
	{
		if (settings.CoordinateSize<12 || settings.CoordinateSize>36 || settings.HintSize<10 || settings.HintSize>24)
			throw std::out_of_range("Invalid text size");
		for (auto color : settings.Colors) for(int i=0;i<4;++i)
			if (!std::isfinite(color[i]) || color[i]<0 || color[i]>1) throw std::invalid_argument("Invalid color");
		appearance_=settings;
	}
    void MainViewModel::ApplyIntegralParts(const std::vector<IntegralPart>& parts)
    {
        auto accepted=parts;
        auto solid=IntegralBodyBuilder().Build(parts);
        IceCreamBody body(parameters_,std::move(solid.Mesh));
        auto plane=CreateSectionPlane(false);
        auto section=MeshSectionCalculator().Calculate(body.Mesh(),plane);
        double area=section.IsEmpty()?0:SectionAreaCalculator().Calculate(section);
        body_=std::move(body);sectionPlane_=plane;section_=std::move(section);sectionArea_=area;
        volume_=solid.Integral;integralError_=solid.EstimatedError;integralParts_=std::move(accepted);
        triangleColors_=std::move(solid.TriangleColors);surfaceEdges_=std::move(solid.Seams);
        heightField_=false;dependentAxis_=2;joinDisk_.reset();joinBoundary_.clear();joinInSection_.clear();
        ++geometryRevision_;
    }
}
