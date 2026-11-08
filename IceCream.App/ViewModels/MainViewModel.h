#pragma once
#include "AppearanceSettings.h"
#include "../../IceCream.Core/Services/Meshing/IntegralBodyBuilder.h"

#include "../../IceCream.Core/Models/IceCreamBody.h"
#include "../../IceCream.Core/Services/Sections/SectionResult.h"
#include "../../IceCream.Core/Geometry/Primitives/Plane3D.h"
#include "../../IceCream.Core/Profiles/Basic/FormulaProfile.h"
#include "../../IceCream.Core/Services/Meshing/ImplicitSurfaceBuilder.h"
#include "../../IceCream.Core/Expressions/Parsing/ExpressionParser.h"
#include <stdexcept>
#include "../Views/Graphics/OrbitCamera/LightingSettings.h"
#include <string>
#include <cstdint>
#include <optional>
#include "../../IceCream.Core/Services/Meshing/HeightFieldBuilder.h"

namespace IceCream::App::ViewModels
{
	// Separate from the measured mesh: finite, translucent guides for g(x,y,z)=0.
	struct ConstraintSurface
	{
		std::string Condition;
		glm::vec3 Color;
		std::vector<float> Triangles;
		glm::vec3 Center{0};
		bool Undefined = false;
	};
	/// <summary>
	/// Хранит параметры, тело мороженого, сечение и предоставляет их представлению.
	/// </summary>
	class MainViewModel
	{
	private:
		Core::Models::IceCreamParameters parameters_;
		std::string formula_ = Core::Profiles::FormulaProfile::DefaultFormula();
		std::uint64_t geometryRevision_ = 0;
		bool transparentPositive_ = true;
		Core::Models::IceCreamBody body_;
		Core::Geometry::Plane3D sectionPlane_;
		Core::Models::SectionResult section_;
		std::vector<Core::Geometry::Point3D> joinBoundary_;
		std::optional<Core::Geometry::TriangleMesh> joinDisk_;
		std::vector<Core::Models::SectionContour> joinInSection_;
		bool heightField_ = false;
		Core::Services::HeightFieldBounds surfaceBounds_;
		double surfaceBaseZ_ = 0;
		std::vector<std::vector<Core::Geometry::Point3D>> surfaceEdges_;
		double volume_ = 0.0;
		double sectionArea_ = 0.0;
		bool volumeAvailable_ = true, sectionAreaAvailable_ = true;
		bool implicitSurface_ = false, meshVolume_ = false;
		std::string constraints_;
		std::vector<ConstraintSurface> constraintSurfaces_;
		Core::Geometry::Point3D sectionPosition_{0.0, 0.0, 2.1};
		Core::Geometry::Vector3D sectionRotation_{};
		
		Views::Graphics::LightingSettings lighting_;
		AppearanceSettings appearance_;
		int dependentAxis_ = 2;
        std::vector<Core::Services::IntegralPart> integralParts_;
        std::vector<std::array<float,4>> triangleColors_;
        double integralError_=0;

		static Core::Models::IceCreamBody CreateBody(const Core::Models::IceCreamParameters& parameters, const std::string& formula);
		void RebuildBody();
		void RecalculateSection();
        Core::Geometry::Plane3D CreateSectionPlane(bool surface) const;

	public:
        static constexpr double SectionPositionLimit = 1000;
		void ApplyIntegralParts(const std::vector<Core::Services::IntegralPart>& parts, const std::string& constraints = "");
        const auto& IntegralParts() const { return integralParts_; }
        const auto& TriangleColors() const { return triangleColors_; }
        bool IsIntegralBody() const { return !integralParts_.empty(); }
        double IntegralError() const { return integralError_; }
		bool HasVolume() const { return volumeAvailable_; }
		bool HasSectionArea() const { return sectionAreaAvailable_; }
		bool UsesMeshVolume() const { return meshVolume_; }
		bool IsImplicitSurface() const { return implicitSurface_; }
		const std::string& Constraints() const { return constraints_; }
		const auto& ConstraintSurfaces() const { return constraintSurfaces_; }
        const AppearanceSettings& Appearance() const { return appearance_; }
		void SetAppearance(const AppearanceSettings& settings);
		int DependentAxis() const { return dependentAxis_; }
		void SetDependentAxis(int axis);
		glm::dmat3 SurfaceRotation() const;
		// Formula and mesh remain in local (u,v,w) coordinates; only this matrix changes.
		/// <summary>
		/// Создаёт мороженое с начальными параметрами.
		/// </summary>
		MainViewModel();

		const Core::Models::IceCreamBody& Body() const
		{
			return body_;
		}

		const Core::Models::IceCreamParameters& Parameters() const
		{
			return parameters_;
		}

		const Core::Models::SectionResult& Section() const
		{
			return section_;
		}

		const Core::Geometry::Plane3D& SectionPlane() const
		{
			return sectionPlane_;
		}

		double Volume() const
		{
			return volume_;
		}

		double SectionArea() const
		{
			return sectionArea_;
		}

		double CenterZ() const
		{
			return 0.5 * (body_.MinZ() + body_.MaxZ());
		}

		void SetBaseHeight(double value);
		void SetJoinRadius(double value);
		void SetBaseExponent(double value);
		void SetCapHeight(double value);
		void ResetDefaults();

		bool IsHeightField() const
		{
			return heightField_;
		}

		const auto& SurfaceBounds() const
		{
			return surfaceBounds_;
		}

		double SurfaceBaseZ() const
		{
			return surfaceBaseZ_;
		}

		const auto& SurfaceEdges() const
		{
			return surfaceEdges_;
		}

		void ApplyHeightField(const Core::Services::HeightFieldBounds& bounds, const std::string& formula, const std::string& constraints = "");
		void ApplyImplicitSurface(const std::string& formula, Core::Geometry::Point3D min, Core::Geometry::Point3D max,
		    int cellsX = 40, int cellsY = 40, int cellsZ = 40, const std::string& constraints = "");
		void ApplyShape(const Core::Models::IceCreamParameters& parameters, const std::string& formula, const std::string& constraints = "");

		bool HasPartJoin() const
		{
			return !joinBoundary_.empty();
		}

		const auto& JoinBoundary() const
		{
			return joinBoundary_;
		}

		const auto& JoinInSection() const
		{
			return joinInSection_;
		}

		const std::string& Formula() const
		{
			return formula_;
		}

		std::uint64_t GeometryRevision() const
		{
			return geometryRevision_;
		}

		bool TransparentPositive() const
		{
			return transparentPositive_;
		}

		void ToggleTransparentSide()
		{
			transparentPositive_ = !transparentPositive_;
		}

		const Core::Geometry::Point3D& SectionPosition() const
		{
			return sectionPosition_;
		}

		const Core::Geometry::Vector3D& SectionRotation() const
		{
			return sectionRotation_;
		}

		void SetSectionPosition(int axis, double value);
		void SetSectionPose(Core::Geometry::Point3D position, Core::Geometry::Vector3D rotation);
		void SetSectionRotation(int axis, double degrees);
		void SetSectionPreset(int normalAxis);

		// свет
		const Views::Graphics::LightingSettings& Lighting() const
		{
			return lighting_;
		}

		void SetLighting(const Views::Graphics::LightingSettings& lighting);
	};
}
