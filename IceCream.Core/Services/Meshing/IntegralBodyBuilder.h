#pragma once
#include "../../Geometry/Meshes/TriangleMesh.h"
#include "../../Geometry/Primitives/Point3D.h"
#include <array>
#include <string>
#include <vector>
#include <utility>
#include <cctype>

namespace IceCream::Core::Services
{
enum class CoordinateSystem
{
    Cartesian,
    Cylindrical,
    Polar
};
enum class PartDefinition
{
    Integral,
    Function
};
struct IntegralPart
{
    std::string Name = "Часть";
    CoordinateSystem Coordinates = CoordinateSystem::Cartesian;
    std::string Function = "1";
    PartDefinition Definition = PartDefinition::Integral;
    int DependentAxis = 2;
    std::string Base = "0";
    bool AboveFunction = false;
    bool AutoUpper = false;
    std::string DependentLower;
    std::string DependentUpper;
    // Editing a graph starts a new definition; do not inherit a preset's hidden cap.
    void SetFunction(std::string formula)
    {
        auto compact = [](const std::string& input) {
            std::string result;
            for (unsigned char c : input) if (!std::isspace(c)) result += static_cast<char>(c);
            return result;
        };
        if (Definition == PartDefinition::Function && !AutoUpper && compact(formula) != compact(Function))
        {
            Base = "0";
            AboveFunction = false;
        }
        Function = std::move(formula);
    }
    std::array<int, 3> FunctionOrder() const
    {
        int axis = Coordinates == CoordinateSystem::Cartesian ? DependentAxis : 2;
        return {{(axis + 1) % 3, (axis + 2) % 3, axis}};
    }
    std::array<int, 3> Order{{0, 1, 2}};
    std::array<std::string, 3> Lower{{"0", "0", "0"}};
    std::array<std::string, 3> Upper{{"1", "1", "1"}};
    std::array<float, 4> Color{{.32f, .70f, .88f, 1}};
};
struct IntegralSolid
{
    Geometry::TriangleMesh Mesh;
    double Integral = 0, EstimatedError = 0;
    std::vector<std::array<float, 4>> TriangleColors;
    std::vector<std::vector<Geometry::Point3D>> Seams;
};
class IntegralBodyBuilder
{
  public:
    IntegralSolid Build(const std::vector<IntegralPart> &parts, int meshSegments = 24,
                        int integrationIntervals = 32) const;
    static std::vector<IntegralPart> DefaultParts();
    static IntegralPart AsIntegral(const IntegralPart &part);
    static IntegralPart WithDependentAxis(const IntegralPart &part, int axis);
};
} // namespace IceCream::Core::Services
