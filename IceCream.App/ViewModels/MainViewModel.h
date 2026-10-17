#pragma once

#include "../../IceCream.Core/Models/IceCreamBody.h"
#include "../../IceCream.Core/Models/SectionResult.h"
#include "../../IceCream.Core/Geometry/Plane3D.h"
#include "../../IceCream.Core/Profiles/FormulaProfile.h"
#include <string>
#include <cstdint>
#include <optional>
#include "../../IceCream.Core/Services/HeightFieldBuilder.h"

namespace IceCream::App::ViewModels
{
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
        Core::Geometry::Point3D sectionPosition_{0.0, 0.0, 2.1};
        Core::Geometry::Vector3D sectionRotation_{};

        static Core::Models::IceCreamBody CreateBody(const Core::Models::IceCreamParameters& parameters, const std::string& formula);
        void RebuildBody();
        void RecalculateSection();

    public:
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
        bool IsHeightField() const { return heightField_; }
        const auto& SurfaceBounds() const { return surfaceBounds_; }
        double SurfaceBaseZ() const { return surfaceBaseZ_; }
        const auto& SurfaceEdges() const { return surfaceEdges_; }
        void ApplyHeightField(const Core::Services::HeightFieldBounds& bounds,const std::string& formula);
        void ApplyShape(const Core::Models::IceCreamParameters& parameters, const std::string& formula);
        bool HasPartJoin() const { return !joinBoundary_.empty(); }
        const auto& JoinBoundary() const { return joinBoundary_; }
        const auto& JoinInSection() const { return joinInSection_; }
        const std::string& Formula() const { return formula_; }
        std::uint64_t GeometryRevision() const { return geometryRevision_; }
        bool TransparentPositive() const { return transparentPositive_; }
        void ToggleTransparentSide() { transparentPositive_ = !transparentPositive_; }
        const Core::Geometry::Point3D& SectionPosition() const { return sectionPosition_; }
        const Core::Geometry::Vector3D& SectionRotation() const { return sectionRotation_; }
        void SetSectionPosition(int axis, double value);
        void SetSectionRotation(int axis, double degrees);
        void SetSectionPreset(int normalAxis);
    };
}
