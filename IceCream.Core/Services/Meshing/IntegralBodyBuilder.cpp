#include "IntegralBodyBuilder.h"
#include "../../Profiles/Basic/FormulaProfile.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <functional>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace IceCream::Core::Services
{
    IntegralPart IntegralBodyBuilder::WithDependentAxis(const IntegralPart& source, int axis)
    {
        if (source.Definition != PartDefinition::Function || source.Coordinates != CoordinateSystem::Cartesian ||
            source.DependentAxis < 0 || source.DependentAxis > 2 || axis < 0 || axis > 2)
        {
            throw invalid_argument("Выбор зависимой координаты доступен только для функции в декартовой системе координат.");
        }

        IntegralPart part = source;
        part.DependentAxis = axis;

        auto oldOrder = source.FunctionOrder();
        auto nextOrder = part.FunctionOrder();

        auto rename = [&](const string& expression) -> string
            {
                const char* names[] = { "x", "y", "z" };
                string result;

                for (size_t i = 0; i < expression.size();)
                {
                    if (!isalpha(static_cast<unsigned char>(expression[i])))
                    {
                        result += expression[i++];
                        continue;
                    }

                    size_t begin = i;

                    while (i < expression.size() && isalpha(static_cast<unsigned char>(expression[i])))
                    {
                        ++i;
                    }

                    string word = expression.substr(begin, i - begin);

                    for (int k = 0; k < 2; ++k)
                    {
                        if (word == names[oldOrder[k]])
                        {
                            word = names[nextOrder[k]];
                            break;
                        }
                    }

                    result += word;
                }

                return result;
            };

        part.Function = rename(source.Function);
        part.Base = rename(source.Base);
        part.DependentLower = rename(source.DependentLower);
        part.DependentUpper = rename(source.DependentUpper);

        for (int k = 0; k < 2; ++k)
        {
            part.Lower[nextOrder[k]] = rename(source.Lower[oldOrder[k]]);
            part.Upper[nextOrder[k]] = rename(source.Upper[oldOrder[k]]);
        }

        part.Order = nextOrder;
        return part;
    }

    namespace
    {
        pair<double, double> EstimateGraphRange(const IntegralPart& part)
        {
            auto formula = Profiles::CompileSpatialExpression(part.Function, { 3, 1, 0.5, 1.2 });
            auto order = part.FunctionOrder();
            using Expression = function<double(double, double, double)>;
            array<Expression, 2> lower, upper;

            for (int d = 0; d < 2; ++d)
            {
                int axis = order[d];
                lower[d] = Profiles::CompileSpatialExpression(part.Lower[axis], { 3, 1, 0.5, 1.2 });
                upper[d] = Profiles::CompileSpatialExpression(part.Upper[axis], { 3, 1, 0.5, 1.2 });
            }

            auto eval = [](const Expression& f, const array<double, 3>& q)
                {
                    double value = f(q[0], q[1], q[2]);
                    if (!isfinite(value)) throw invalid_argument("Функция или пределы принимают недопустимое значение.");
                    return value;
                };

            constexpr int samples = 256;
            double maximum = -numeric_limits<double>::infinity();
            double minimum = numeric_limits<double>::infinity();

            for (int i = 0; i <= samples; ++i)
            {
                array<double, 3> q{};
                int a = order[0], b = order[1];
                double firstLow = eval(lower[0], q);
                double firstHigh = eval(upper[0], q);
                if (firstHigh < firstLow) throw invalid_argument("Неверные границы области графика.");
                q[a] = firstLow + (firstHigh - firstLow) * i / samples;

                double secondLow = eval(lower[1], q);
                double secondHigh = eval(upper[1], q);
                if (secondHigh < secondLow) throw invalid_argument("Неверные границы области графика.");

                for (int j = 0; j <= samples; ++j)
                {
                    q[b] = secondLow + (secondHigh - secondLow) * j / samples;
                    double height = eval(formula, q);
                    minimum = min(minimum, height);
                    maximum = max(maximum, height);
                }
            }

            return {minimum, maximum};
        }
    }

    IntegralPart IntegralBodyBuilder::AsIntegral(const IntegralPart& source)
    {
        IntegralPart part = source;

        if (source.Definition == PartDefinition::Function)
        {
            if (source.DependentAxis < 0 || source.DependentAxis > 2 ||
                (source.Coordinates != CoordinateSystem::Cartesian && source.DependentAxis != 2))
            {
                throw invalid_argument("Некорректно выбрана зависимая координата.");
            }

            if (source.AutoUpper)
            {
                auto [minimum, maximum] = EstimateGraphRange(source);
                double tolerance = 1e-12 * max({1.0, abs(minimum), abs(maximum)});

                if (maximum - minimum <= tolerance)
                {
                    if (abs(maximum) <= tolerance)
                    {
                        throw invalid_argument("График совпадает с нулевой плоскостью: объём отсутствует.");
                    }
                    part.Base = "0";
                    part.AboveFunction = false;
                }
                else
                {
                    ostringstream ceiling;
                    ceiling << setprecision(17) << maximum;
                    part.Base = ceiling.str();
                    part.AboveFunction = true;
                }
            }

            part.Order = source.FunctionOrder();
            int axis = part.Order[2];

            string lower = part.AboveFunction ? source.Function : part.Base;
            string upper = part.AboveFunction ? part.Base : source.Function;

            if (!source.DependentLower.empty())
            {
                lower = "max((" + lower + "),(" + source.DependentLower + "))";
            }
            if (!source.DependentUpper.empty())
            {
                upper = "min((" + upper + "),(" + source.DependentUpper + "))";
            }
            if (!source.DependentLower.empty() || !source.DependentUpper.empty())
            {
                upper = "max((" + lower + "),(" + upper + "))";
            }

            part.Lower[axis] = lower;
            part.Upper[axis] = upper;
            part.Function = "1";
            part.Definition = PartDefinition::Integral;
            part.AutoUpper = false;

            if (part.Coordinates == CoordinateSystem::Polar)
            {
                part.Coordinates = CoordinateSystem::Cylindrical;
            }
        }

        return part;
    }

    namespace
    {
        using Point = Geometry::Point3D;
        using Expression = function<double(double, double, double)>;
        using Values = array<double, 3>;

        constexpr double Pi = 3.14159265358979323846;

        struct Compiled
        {
            IntegralPart Part;
            Expression Function;
            array<Expression, 3> Low, High;
            int Dimensions;
            bool Graph;
            double Sign = 1.0;

            static double Eval(const Expression& f, const Values& q)
            {
                double value = f(q[0], q[1], q[2]);

                if (!isfinite(value))
                {
                    throw invalid_argument("Выражение принимает неконечное значение в области интегрирования.");
                }

                return value;
            }

            explicit Compiled(const IntegralPart& source)
                : Part(IntegralBodyBuilder::AsIntegral(source)),
                Dimensions(Part.Coordinates == CoordinateSystem::Polar ? 2 : 3),
                Graph(source.Definition == PartDefinition::Function)
            {
                const IntegralPart& part = Part;

                if (part.Coordinates != CoordinateSystem::Cartesian &&
                    part.Coordinates != CoordinateSystem::Cylindrical &&
                    part.Coordinates != CoordinateSystem::Polar)
                {
                    throw invalid_argument("Неизвестная система координат.");
                }

                array<bool, 3> seen{};

                for (int d = 0; d < Dimensions; ++d)
                {
                    int k = part.Order[d];

                    if (k < 0 || k >= Dimensions || seen[k])
                    {
                        throw invalid_argument("Некорректный порядок интегрирования.");
                    }

                    seen[k] = true;
                }

                auto compile = [&](const string& text, int available) -> Expression
                    {
                        for (size_t i = 0; i < text.size();)
                        {
                            if (!isalpha(static_cast<unsigned char>(text[i])))
                            {
                                ++i;
                                continue;
                            }

                            size_t begin = i;

                            while (i < text.size() && isalpha(static_cast<unsigned char>(text[i])))
                            {
                                ++i;
                            }

                            string name = text.substr(begin, i - begin);
                            int k = -1;

                            if (part.Coordinates == CoordinateSystem::Cartesian)
                            {
                                if (name == "x") k = 0;
                                if (name == "y") k = 1;
                                if (name == "z") k = 2;

                                if (name == "r" || name == "phi")
                                {
                                    throw invalid_argument("В декартовой системе координат используйте x, y, z.");
                                }
                            }
                            else
                            {
                                if (name == "r") k = 0;
                                if (name == "phi") k = 1;
                                if (name == "z") k = 2;

                                if (name == "x" || name == "y")
                                {
                                    throw invalid_argument("В цилиндрической и полярной системах используйте r, phi, z.");
                                }
                            }

                            if (k >= 0 && find(part.Order.begin(), part.Order.begin() + available, k) == part.Order.begin() + available)
                            {
                                throw invalid_argument("Граница интегрирования может зависеть только от предшествующих внешних координат.");
                            }
                        }

                        return Profiles::CompileSpatialExpression(text, { 3, 1, 0.5, 1.2 });
                    };

                Function = compile(part.Function, Dimensions);

                for (int d = 0; d < Dimensions; ++d)
                {
                    int k = part.Order[d];
                    Low[k] = compile(part.Lower[k], d);
                    High[k] = compile(part.Upper[k], d);
                }

                int inversions = 0;

                for (int i = 0; i < Dimensions; ++i)
                {
                    for (int j = i + 1; j < Dimensions; ++j)
                    {
                        if (part.Order[i] > part.Order[j])
                        {
                            ++inversions;
                        }
                    }
                }

                Sign = inversions % 2 ? -1.0 : 1.0;
            }

            Values Map(const Values& t, double* jacobian = nullptr, double* thickness = nullptr) const
            {
                Values q{};
                double j = 1.0;

                for (int d = 0; d < Dimensions; ++d)
                {
                    int k = Part.Order[d];
                    double a = Eval(Low[k], q);
                    double b = Eval(High[k], q);
                    double width = b - a;

                    if (!isfinite(width))
                    {
                        throw invalid_argument("Размер области интегрирования слишком велик для вычислений.");
                    }

                    if (b < a - 1e-10)
                    {
                        throw invalid_argument("Нижняя граница интегрирования не должна превышать верхнюю.");
                    }

                    if (Part.Coordinates != CoordinateSystem::Cartesian && k == 0 && a < 0.0)
                    {
                        throw invalid_argument("Радиус в полярных и цилиндрических координатах не может быть отрицательным.");
                    }

                    if (Part.Coordinates != CoordinateSystem::Cartesian && k == 1 && width > 2.0 * Pi + 1e-8)
                    {
                        throw invalid_argument("Угловой интервал не должен превышать 2π.");
                    }

                    if (thickness && Graph && d == 2)
                    {
                        *thickness = width;
                    }

                    q[k] = a + width * t[d];
                    j *= max(0.0, width);

                    if (!isfinite(q[k]) || !isfinite(j))
                    {
                        throw invalid_argument("Переполнение при вычислении координат или якобиана.");
                    }
                }

                if (Part.Coordinates == CoordinateSystem::Polar)
                {
                    double height = Eval(Function, q);

                    if (height < 0.0)
                    {
                        throw invalid_argument("Высота поверхности в полярных координатах не может быть отрицательной.");
                    }

                    q[2] = t[2] * height;
                }

                if (jacobian)
                {
                    *jacobian = j * (Part.Coordinates == CoordinateSystem::Cartesian ? 1.0 : q[0]);

                    if (!isfinite(*jacobian))
                    {
                        throw invalid_argument("Переполнение при вычислении якобиана.");
                    }
                }

                return q;
            }

            Point World(const Values& t) const
            {
                double thickness = 0.0;
                Values q = Map(t, nullptr, &thickness);

                // Убираем численную щель на границе нулевой толщины только в геометрии.
                // Интегрирование продолжает использовать исходные функции.
                if (Graph && abs(thickness) < 1e-7)
                {
                    int axis = Part.Order[2];
                    q[axis] = Eval(Low[axis], q);
                }

                if (Part.Coordinates == CoordinateSystem::Cartesian)
                {
                    return { q[0], q[1], q[2] };
                }

                return { q[0] * cos(q[1]), q[0] * sin(q[1]), q[2] };
            }

            bool Contains(Point point) const
            {
                Values q{ {point.X, point.Y, point.Z} };

                if (Part.Coordinates != CoordinateSystem::Cartesian)
                {
                    q[0] = hypot(point.X, point.Y);
                    q[1] = atan2(point.Y, point.X);
                }

                for (int d = 0; d < Dimensions; ++d)
                {
                    int k = Part.Order[d];
                    double a = Eval(Low[k], q);
                    double b = Eval(High[k], q);

                    if (Part.Coordinates != CoordinateSystem::Cartesian && k == 1)
                    {
                        q[1] += 2.0 * Pi * round(((a + b) / 2.0 - q[1]) / (2.0 * Pi));
                    }

                    if (q[k] <= a + 1e-7 || q[k] >= b - 1e-7)
                    {
                        return false;
                    }
                }

                if (Part.Coordinates == CoordinateSystem::Polar)
                {
                    return point.Z > 1e-7 && point.Z < Eval(Function, q) - 1e-7;
                }

                return true;
            }

            double Integrate(int n) const
            {
                Values t{};

                function<double(int)> step = [&](int d) -> double
                    {
                        if (d == (Graph ? 2 : Dimensions))
                        {
                            double jacobian = 0.0;
                            Values q = Map(t, &jacobian);
                            return Eval(Function, q) * jacobian;
                        }

                        double sum = 0.0;

                        for (int i = 0; i <= n; ++i)
                        {
                            t[d] = static_cast<double>(i) / n;
                            double value = step(d + 1);
                            sum += (i == 0 || i == n ? 1 : i % 2 ? 4 : 2) * value;
                        }

                        return sum / (3.0 * n);
                    };

                double result = step(0);

                if (!isfinite(result))
                {
                    throw invalid_argument("Переполнение при вычислении интеграла.");
                }

                return result;
            }
        };
    }

    IntegralSolid IntegralBodyBuilder::Build(const vector<IntegralPart>& parts, int n, int intervals) const
    {
        if (parts.empty() || parts.size() > 8)
        {
            throw invalid_argument("Количество частей тела должно быть от 1 до 8.");
        }

        if (n < 4 || n > 64)
        {
            throw invalid_argument("Количество разбиений геометрической сетки должно быть от 4 до 64.");
        }

        if (intervals < 4 || intervals > 128 || intervals % 2 != 0)
        {
            throw invalid_argument("Количество интервалов интегрирования должно быть чётным числом от 4 до 128.");
        }

        vector<Compiled> compiled;
        compiled.reserve(parts.size());

        for (const IntegralPart& part : parts)
        {
            for (float channel : part.Color)
            {
                if (!isfinite(channel) || channel < 0.0f || channel > 1.0f)
                {
                    throw invalid_argument("Компоненты цвета должны находиться в диапазоне от 0 до 1.");
                }
            }

            compiled.emplace_back(part);
        }

        // Соприкасающиеся части объединяются по общим граням.
        // Для пересекающихся объёмов потребуется отдельное булево объединение.
        for (size_t i = 0; i < compiled.size(); ++i)
        {
            for (size_t j = 0; j < i; ++j)
            {
                for (double a : {0.2, 0.5, 0.8})
                {
                    for (double b : {0.2, 0.5, 0.8})
                    {
                        for (double c : {0.2, 0.5, 0.8})
                        {
                            if (compiled[i].Contains(compiled[j].World({ {a, b, c} })) ||
                                compiled[j].Contains(compiled[i].World({ {a, b, c} })))
                            {
                                throw invalid_argument("Части тела пересекаются. Задайте области, которые соприкасаются только по границе.");
                            }
                        }
                    }
                }
            }
        }

        vector<Point> vertices;
        map<array<long long, 3>, int> welded;
        map<array<double, 3>, int> exactWelded;

        struct Face
        {
            array<int, 3> Indices;
            size_t Part;
        };

        map<array<int, 3>, Face> faces;
        map<pair<int, int>, int> seams;
        double integral = 0.0;
        double error = 0.0;

        // Для обычных координат сохраняем прежнее объединение вершин с точностью 1e-8.
        // Для очень больших координат используем точное сравнение, чтобы избежать переполнения llround.
        constexpr double maxQuantizedCoordinate = static_cast<double>(numeric_limits<long long>::max()) / 2e8;

        auto vertex = [&](Point p) -> int
            {
                if (!isfinite(p.X) || !isfinite(p.Y) || !isfinite(p.Z))
                {
                    throw invalid_argument("Не удалось построить вершину с конечными координатами.");
                }

                auto addVertex = [&]() -> int
                    {
                        if (vertices.size() >= static_cast<size_t>(numeric_limits<int>::max()))
                        {
                            throw length_error("Слишком много вершин в треугольной сетке.");
                        }

                        int id = static_cast<int>(vertices.size());
                        vertices.push_back(p);
                        return id;
                    };

                if (abs(p.X) <= maxQuantizedCoordinate && abs(p.Y) <= maxQuantizedCoordinate && abs(p.Z) <= maxQuantizedCoordinate)
                {
                    array<long long, 3> key{ {llround(p.X * 1e8), llround(p.Y * 1e8), llround(p.Z * 1e8)} };
                    auto it = welded.find(key);

                    if (it != welded.end())
                    {
                        return it->second;
                    }

                    int id = addVertex();
                    welded.emplace(key, id);
                    return id;
                }

                array<double, 3> key{ {p.X, p.Y, p.Z} };
                auto it = exactWelded.find(key);

                if (it != exactWelded.end())
                {
                    return it->second;
                }

                int id = addVertex();
                exactWelded.emplace(key, id);
                return id;
            };

        // Построение сетки каждой части и вычисление интеграла.
        for (size_t part = 0; part < parts.size(); ++part)
        {
            try
            {
                const Compiled& current = compiled[part];
                int resolution = current.Graph ? max(intervals, 128) : intervals;

                double coarse = current.Integrate(resolution);
                double fine = current.Integrate(resolution * 2);

                integral += fine;
                error += abs(fine - coarse) / 15.0;

                if (!isfinite(integral) || !isfinite(error))
                {
                    throw invalid_argument("Переполнение при суммировании интегралов частей тела.");
                }

                // На общих гранях используем одинаковое направление диагоналей.
                auto addTriangle = [&](int a, int b, int c)
                    {
                        if (a == b || b == c || a == c)
                        {
                            return;
                        }

                        auto normal = (vertices[b] - vertices[a]).Cross(vertices[c] - vertices[a]);

                        if (normal.Length() < 1e-12)
                        {
                            return;
                        }

                        if (current.Sign < 0.0)
                        {
                            swap(b, c);
                        }

                        array<int, 3> key{ {a, b, c} };
                        sort(key.begin(), key.end());
                        auto it = faces.find(key);

                        if (it == faces.end())
                        {
                            faces.emplace(key, Face{ {{a, b, c}}, part });
                        }
                        else
                        {
                            if (it->second.Part != part)
                            {
                                for (int i = 0; i < 3; ++i)
                                {
                                    int x = key[i];
                                    int y = key[(i + 1) % 3];

                                    if (x > y)
                                    {
                                        swap(x, y);
                                    }

                                    ++seams[{x, y}];
                                }
                            }

                            faces.erase(it);
                        }
                    };

                for (int axis = 0; axis < 3; ++axis)
                {
                    for (int side = 0; side < 2; ++side)
                    {
                        for (int i = 0; i < n; ++i)
                        {
                            for (int j = 0; j < n; ++j)
                            {
                                int a = (axis + 1) % 3;
                                int b = (axis + 2) % 3;

                                Values t{};
                                t[axis] = side;

                                auto getVertex = [&](int x, int y) -> int
                                    {
                                        t[a] = static_cast<double>(x) / n;
                                        t[b] = static_cast<double>(y) / n;
                                        return vertex(current.World(t));
                                    };

                                int p00 = getVertex(i, j);
                                int p10 = getVertex(i + 1, j);
                                int p11 = getVertex(i + 1, j + 1);
                                int p01 = getVertex(i, j + 1);

                                if (side)
                                {
                                    addTriangle(p00, p10, p11);
                                    addTriangle(p00, p11, p01);
                                }
                                else
                                {
                                    addTriangle(p00, p11, p10);
                                    addTriangle(p00, p01, p11);
                                }
                            }
                        }
                    }
                }
            }
            catch (const exception& e)
            {
                throw invalid_argument("Часть " + to_string(part + 1) + ": " + e.what());
            }
        }

        vector<int> indices;
        vector<array<float, 4>> colors;

        // Сохраняем порядок граней, чтобы материалы назначались воспроизводимо.
        for (const auto& item : faces)
        {
            const Face& face = item.second;
            indices.insert(indices.end(), face.Indices.begin(), face.Indices.end());
            colors.push_back(parts[face.Part].Color);
        }

        if (indices.empty())
        {
            throw invalid_argument("Область интегрирования не содержит поверхности для отображения.");
        }

        vector<vector<Point>> boundaries;

        for (const auto& edge : seams)
        {
            if (edge.second == 1)
            {
                boundaries.push_back({ vertices[edge.first.first], vertices[edge.first.second] });
            }
        }

        return { Geometry::TriangleMesh(move(vertices), move(indices)), integral, error, move(colors), move(boundaries) };
    }

    vector<IntegralPart> IntegralBodyBuilder::DefaultParts()
    {
        IntegralPart cone;
        cone.Name = "Рожок";
        cone.Definition = PartDefinition::Function;
        cone.Function = "3*sqrt(x^2+y^2)";
        cone.Base = "3";
        cone.AboveFunction = true;
        cone.AutoUpper = true;
        cone.Lower = { {"-1", "-sqrt(max(0,1-x^2))", "0"} };
        cone.Upper = { {"1", "sqrt(max(0,1-x^2))", "3"} };
        cone.Color = { {0.76f, 0.5f, 0.3f, 1.0f} };

        IntegralPart ice = cone;
        ice.Name = "Мороженое";
        ice.AboveFunction = false;
        ice.AutoUpper = false;
        ice.Function = "3+sqrt(max(0,1-x^2-y^2))";
        ice.Color = { {0.6f, 0.85f, 0.35f, 1.0f} };

        return { cone, ice };
    }
}
