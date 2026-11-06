#include "IntegralBodyBuilder.h"
#include "../../Profiles/Basic/FormulaProfile.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>
#include <tuple>

namespace IceCream::Core::Services
{
IntegralPart IntegralBodyBuilder::WithDependentAxis(const IntegralPart &source, int axis)
{
    if (source.Definition != PartDefinition::Function || source.Coordinates != CoordinateSystem::Cartesian ||
        source.DependentAxis < 0 || source.DependentAxis > 2 || axis < 0 || axis > 2)
        throw std::invalid_argument("Dependent coordinate selection requires a Cartesian function");
    auto part = source;
    part.DependentAxis = axis;
    auto oldOrder = source.FunctionOrder(), nextOrder = part.FunctionOrder();
    auto rename = [&](const std::string &expression) {
        const char *names[] = {"x", "y", "z"};
        std::string result;
        for (size_t i = 0; i < expression.size();)
        {
            if (!std::isalpha(static_cast<unsigned char>(expression[i])))
            {
                result += expression[i++];
                continue;
            }
            size_t begin = i;
            while (i < expression.size() && std::isalpha(static_cast<unsigned char>(expression[i])))
                ++i;
            auto word = expression.substr(begin, i - begin);
            for (int k = 0; k < 2; ++k)
                if (word == names[oldOrder[k]])
                {
                    word = names[nextOrder[k]];
                    break;
                }
            result += word;
        }
        return result;
    };
    part.Function = rename(source.Function);
    part.Base = rename(source.Base);
    for (int k = 0; k < 2; ++k)
    {
        part.Lower[nextOrder[k]] = rename(source.Lower[oldOrder[k]]);
        part.Upper[nextOrder[k]] = rename(source.Upper[oldOrder[k]]);
    }
    part.Order = nextOrder;
    return part;
}
IntegralPart IntegralBodyBuilder::AsIntegral(const IntegralPart &source)
{
    auto part = source;
    if (source.Definition == PartDefinition::Function)
    {
        if (source.DependentAxis < 0 || source.DependentAxis > 2 ||
            (source.Coordinates != CoordinateSystem::Cartesian && source.DependentAxis != 2))
            throw std::invalid_argument("Invalid dependent coordinate");
        part.Order = source.FunctionOrder();
        int axis = part.Order[2];
        part.Lower[axis] = source.AboveFunction ? source.Function : source.Base;
        part.Upper[axis] = source.AboveFunction ? source.Base : source.Function;
        part.Function = "1";
        part.Definition = PartDefinition::Integral;
        if (part.Coordinates == CoordinateSystem::Polar)
            part.Coordinates = CoordinateSystem::Cylindrical;
    }
    return part;
}
namespace
{
using Point = Geometry::Point3D;
using Expression = std::function<double(double, double, double)>;
using Values = std::array<double, 3>;
constexpr double Pi = 3.14159265358979323846;
struct Compiled
{
    IntegralPart Part;
    Expression Function;
    std::array<Expression, 3> Low, High;
    int Dimensions;
    bool Graph;
    double Sign = 1;
    static double Eval(const Expression &f, const Values &q)
    {
        double v = f(q[0], q[1], q[2]);
        if (!std::isfinite(v))
            throw std::invalid_argument("Expression is not finite");
        return v;
    }
    explicit Compiled(const IntegralPart &source)
        : Part(IntegralBodyBuilder::AsIntegral(source)),
          Dimensions(Part.Coordinates == CoordinateSystem::Polar ? 2 : 3),
          Graph(source.Definition == PartDefinition::Function)
    {
        const auto &p = Part;
        if (p.Coordinates != CoordinateSystem::Cartesian && p.Coordinates != CoordinateSystem::Cylindrical &&
            p.Coordinates != CoordinateSystem::Polar)
            throw std::invalid_argument("Invalid coordinate system");
        std::array<bool, 3> seen{};
        for (int d = 0; d < Dimensions; ++d)
        {
            int k = p.Order[d];
            if (k < 0 || k >= Dimensions || seen[k])
                throw std::invalid_argument("Invalid integration order");
            seen[k] = true;
        }
        auto compile = [&](const std::string &text, int available) {
            for (size_t i = 0; i < text.size();)
            {
                if (!std::isalpha(static_cast<unsigned char>(text[i])))
                {
                    ++i;
                    continue;
                }
                size_t a = i;
                while (i < text.size() && std::isalpha(static_cast<unsigned char>(text[i])))
                    ++i;
                auto name = text.substr(a, i - a);
                int k = -1;
                if (p.Coordinates == CoordinateSystem::Cartesian)
                {
                    if (name == "x")
                        k = 0;
                    if (name == "y")
                        k = 1;
                    if (name == "z")
                        k = 2;
                    if (name == "r" || name == "phi")
                        throw std::invalid_argument("Use x,y,z in Cartesian coordinates");
                }
                else
                {
                    if (name == "r")
                        k = 0;
                    if (name == "phi")
                        k = 1;
                    if (name == "z")
                        k = 2;
                    if (name == "x" || name == "y")
                        throw std::invalid_argument("Use r,phi,z in cylindrical coordinates");
                }
                if (k >= 0 && std::find(p.Order.begin(), p.Order.begin() + available, k) == p.Order.begin() + available)
                    throw std::invalid_argument("A bound may depend only on preceding outer coordinates");
            }
            return Profiles::CompileSpatialExpression(text, {3, 1, .5, 1.2});
        };
        Function = compile(p.Function, Dimensions);
        for (int d = 0; d < Dimensions; ++d)
        {
            int k = p.Order[d];
            Low[k] = compile(p.Lower[k], d);
            High[k] = compile(p.Upper[k], d);
        }
        int inversions = 0;
        for (int i = 0; i < Dimensions; ++i)
            for (int j = i + 1; j < Dimensions; ++j)
                if (p.Order[i] > p.Order[j])
                    ++inversions;
        Sign = inversions % 2 ? -1 : 1;
    }
    Values Map(const Values &t, double *jacobian = nullptr, double *thickness = nullptr) const
    {
        Values q{};
        double j = 1;
        for (int d = 0; d < Dimensions; ++d)
        {
            int k = Part.Order[d];
            double a = Eval(Low[k], q), b = Eval(High[k], q);
            if (std::abs(a) > 100 || std::abs(b) > 100 || b < a - 1e-10)
                throw std::invalid_argument("Bounds must be ordered and within -100..100");
            if (Part.Coordinates != CoordinateSystem::Cartesian && k == 0 && a < 0)
                throw std::invalid_argument("Radius cannot be negative");
            if (Part.Coordinates != CoordinateSystem::Cartesian && k == 1 && b - a > 2 * Pi + 1e-8)
                throw std::invalid_argument("Angle interval must not exceed 2*pi");
            if (thickness && Graph && d == 2)
                *thickness = b - a;
            q[k] = a + (b - a) * t[d];
            j *= std::max(0.0, b - a);
        }
        if (Part.Coordinates == CoordinateSystem::Polar)
        {
            double h = Eval(Function, q);
            if (h < 0 || h > 100)
                throw std::invalid_argument("Polar graph height must be in 0..100");
            q[2] = t[2] * h;
        }
        if (jacobian)
            *jacobian = j * (Part.Coordinates == CoordinateSystem::Cartesian ? 1 : q[0]);
        return q;
    }
    Point World(const Values &t) const
    {
        double thickness = 0;
        auto q = Map(t, nullptr, &thickness);
        // Square-root boundaries can leave roundoff slivers at a zero-thickness rim.
        // Snap only the display mesh; quadrature still evaluates the original functions.
        if (Graph && std::abs(thickness) < 1e-7)
        {
            int axis = Part.Order[2];
            q[axis] = Eval(Low[axis], q);
        }
        if (Part.Coordinates == CoordinateSystem::Cartesian)
            return {q[0], q[1], q[2]};
        return {q[0] * std::cos(q[1]), q[0] * std::sin(q[1]), q[2]};
    }
    bool Contains(Point point) const
    {
        Values q{{point.X, point.Y, point.Z}};
        if (Part.Coordinates != CoordinateSystem::Cartesian)
        {
            q[0] = std::hypot(point.X, point.Y);
            q[1] = std::atan2(point.Y, point.X);
        }
        for (int d = 0; d < Dimensions; ++d)
        {
            int k = Part.Order[d];
            double a = Eval(Low[k], q), b = Eval(High[k], q);
            if (Part.Coordinates != CoordinateSystem::Cartesian && k == 1)
                q[1] += 2 * Pi * std::round(((a + b) / 2 - q[1]) / (2 * Pi));
            if (q[k] <= a + 1e-7 || q[k] >= b - 1e-7)
                return false;
        }
        if (Part.Coordinates == CoordinateSystem::Polar)
            return point.Z > 1e-7 && point.Z < Eval(Function, q) - 1e-7;
        return true;
    }
    double Integrate(int n) const
    {
        Values t{};
        std::function<double(int)> step = [&](int d) -> double {
            if (d == (Graph ? 2 : Dimensions))
            {
                double j;
                auto q = Map(t, &j);
                return Eval(Function, q) * j;
            }
            double sum = 0;
            for (int i = 0; i <= n; ++i)
            {
                t[d] = double(i) / n;
                double value = step(d + 1);
                sum += (i == 0 || i == n ? 1 : i % 2 ? 4 : 2) * value;
            }
            return sum / (3 * n);
        };
        double value = step(0);
        if (!std::isfinite(value))
            throw std::invalid_argument("Integral overflow");
        return value;
    }
};
} // namespace
IntegralSolid IntegralBodyBuilder::Build(const std::vector<IntegralPart> &parts, int n, int intervals) const
{
    if (parts.empty() || parts.size() > 8 || n < 4 || n > 64 || intervals < 4 || intervals > 128 || intervals % 2)
        throw std::invalid_argument("Invalid part count or resolution");
    std::vector<Compiled> compiled;
    for (const auto &part : parts)
    {
        for (float channel : part.Color)
            if (!std::isfinite(channel) || channel < 0 || channel > 1)
                throw std::invalid_argument("Color channels must be in 0..1");
        compiled.emplace_back(part);
    }
    // This operation joins contacting parameterized domains; overlapping solids need a Boolean union.
    for (size_t i = 0; i < compiled.size(); ++i)
        for (size_t j = 0; j < i; ++j)
            for (double a : {.2, .5, .8})
                for (double b : {.2, .5, .8})
                    for (double c : {.2, .5, .8})
                        if (compiled[i].Contains(compiled[j].World({{a, b, c}})) ||
                            compiled[j].Contains(compiled[i].World({{a, b, c}})))
                            throw std::invalid_argument(
                                "Parts overlap. Define non-overlapping domains that meet at their boundary.");
    std::vector<Point> vertices;
    std::map<std::array<long long, 3>, int> welded;
    struct Face
    {
        std::array<int, 3> Indices;
        size_t Part;
    };
    std::map<std::array<int, 3>, Face> faces;
    std::map<std::pair<int, int>, int> seams;
    double integral = 0, error = 0;
    auto vertex = [&](Point p) {
        std::array<long long, 3> key{{std::llround(p.X * 1e8), std::llround(p.Y * 1e8), std::llround(p.Z * 1e8)}};
        auto it = welded.find(key);
        if (it != welded.end())
            return it->second;
        int id = static_cast<int>(vertices.size());
        vertices.push_back(p);
        welded[key] = id;
        return id;
    };
    for (size_t part = 0; part < parts.size(); ++part)
    {
        try
        {
            const auto &c = compiled[part];
            int resolution = c.Graph ? std::max(intervals, 128) : intervals;
            double coarse = c.Integrate(resolution), fine = c.Integrate(resolution * 2);
            integral += fine;
            error += std::abs(fine - coarse) / 15;
            // Each normalized cube face uses the same diagonal at shared boundaries.
            auto triangle = [&](int a, int b, int d) {
                if (a == b || b == d || a == d)
                    return;
                auto normal = (vertices[b] - vertices[a]).Cross(vertices[d] - vertices[a]);
                if (normal.Length() < 1e-12)
                    return;
                if (c.Sign < 0)
                    std::swap(b, d);
                std::array<int, 3> key{{a, b, d}};
                std::sort(key.begin(), key.end());
                auto it = faces.find(key);
                if (it == faces.end())
                    faces.emplace(key, Face{{{a, b, d}}, part});
                else
                {
                    if (it->second.Part != part)
                    {
                        for (int i = 0; i < 3; ++i)
                        {
                            int x = key[i], y = key[(i + 1) % 3];
                            if (x > y)
                                std::swap(x, y);
                            ++seams[{x, y}];
                        }
                    }
                    faces.erase(it);
                }
            };
            for (int axis = 0; axis < 3; ++axis)
                for (int side = 0; side < 2; ++side)
                    for (int i = 0; i < n; ++i)
                        for (int j = 0; j < n; ++j)
                        {
                            int a = (axis + 1) % 3, b = (axis + 2) % 3;
                            Values t{};
                            t[axis] = side;
                            auto p = [&](int x, int y) {
                                t[a] = double(x) / n;
                                t[b] = double(y) / n;
                                return vertex(c.World(t));
                            };
                            int p00 = p(i, j), p10 = p(i + 1, j), p11 = p(i + 1, j + 1), p01 = p(i, j + 1);
                            if (side)
                            {
                                triangle(p00, p10, p11);
                                triangle(p00, p11, p01);
                            }
                            else
                            {
                                triangle(p00, p11, p10);
                                triangle(p00, p01, p11);
                            }
                        }
        }
        catch (const std::exception &e)
        {
            throw std::invalid_argument("Part " + std::to_string(part + 1) + ": " + e.what());
        }
    }
    std::vector<int> indices;
    std::vector<std::array<float, 4>> colors;
    // Group by part for stable material assignment and reproducible meshes.
    for (const auto &item : faces)
    {
        const auto &f = item.second;
        indices.insert(indices.end(), f.Indices.begin(), f.Indices.end());
        colors.push_back(parts[f.Part].Color);
    }
    if (indices.empty())
        throw std::invalid_argument("The integral domain has no drawable volume");
    std::vector<std::vector<Point>> boundaries;
    for (const auto &edge : seams)
        if (edge.second == 1)
            boundaries.push_back({vertices[edge.first.first], vertices[edge.first.second]});
    return {Geometry::TriangleMesh(std::move(vertices), std::move(indices)), integral, error, std::move(colors),
            std::move(boundaries)};
}
std::vector<IntegralPart> IntegralBodyBuilder::DefaultParts()
{
    IntegralPart cone;
    cone.Name = "Рожок";
    cone.Definition = PartDefinition::Function;
    cone.Function = "3*sqrt(x^2+y^2)";
    cone.Base = "3";
    cone.AboveFunction = true;
    cone.Lower = {{"-1", "-sqrt(max(0,1-x^2))", "0"}};
    cone.Upper = {{"1", "sqrt(max(0,1-x^2))", "3"}};
    cone.Color = {{.76f, .5f, .3f, 1}};
    IntegralPart ice = cone;
    ice.Name = "Мороженое";
    ice.AboveFunction = false;
    ice.Function = "3+sqrt(max(0,1-x^2-y^2))";
    ice.Color = {{.6f, .85f, .35f, 1}};
    return {cone, ice};
}
} // namespace IceCream::Core::Services
