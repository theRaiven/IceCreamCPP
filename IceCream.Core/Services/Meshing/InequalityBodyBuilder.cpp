#include "InequalityBodyBuilder.h"
#include "ImplicitSurfaceBuilder.h"
#include "../Measurements/VolumeCalculator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Inequalities;

namespace IceCream::Core::Services
{
    namespace
    {
        struct Point2
        {
            double X, Y;
        };

        enum class BoundKind { Lateral, Lower, Upper };

        struct Bound
        {
            const Inequality* Constraint;
            BoundKind Kind;
        };

        struct Heights
        {
            double Lower, Upper;
        };

        // Не все области являются вертикальными телами с плоскими крышками.
        // В таких случаях не подменяем геометрию догадками, а используем Marching Cubes.
        struct NotApplicable {};

        // Не подменяем обрезанное границами поиска тело грубой сеткой Marching Cubes.
        struct DomainClipped {};

        double Cross(Point2 a, Point2 b, Point2 c)
        {
            return (b.X - a.X) * (c.Y - a.Y) - (b.Y - a.Y) * (c.X - a.X);
        }

        double SignedArea(const vector<Point2>& polygon)
        {
            double area = 0.0;

            for (size_t i = 0; i < polygon.size(); ++i)
            {
                const Point2& a = polygon[i];
                const Point2& b = polygon[(i + 1) % polygon.size()];
                area += a.X * b.Y - a.Y * b.X;
            }

            return area * 0.5;
        }

        // Проверяем зависимость каждого ограничения от z, не разбирая текст формулы.
        // Сложные и немонотонные зависимости оставляем универсальному построителю.
        vector<Bound> Classify(const InequalityRegion& region, Point3D min, Point3D max)
        {
            vector<Bound> bounds;
            int lowerCount = 0;
            int upperCount = 0;

            for (const Inequality& inequality : region.GetInequalities())
            {
                int direction = 0;

                for (double sx : {0.23, 0.57, 0.81})
                {
                    for (double sy : {0.19, 0.63})
                    {
                        double x = min.X + (max.X - min.X) * sx;
                        double y = min.Y + (max.Y - min.Y) * sy;
                        double previous = inequality.Evaluate(x, y, min.Z);
                        int localDirection = 0;

                        if (!isfinite(previous)) throw NotApplicable{};

                        for (int k = 1; k <= 6; ++k)
                        {
                            double z = min.Z + (max.Z - min.Z) * (static_cast<double>(k) / 6.0);
                            double current = inequality.Evaluate(x, y, z);
                            if (!isfinite(current)) throw NotApplicable{};

                            double difference = current - previous;
                            double tolerance = 1e-11 * std::max({ 1.0, abs(current), abs(previous) });
                            int sign = difference > tolerance ? 1 : difference < -tolerance ? -1 : 0;

                            if (sign != 0)
                            {
                                if (localDirection != 0 && sign != localDirection) throw NotApplicable{};
                                localDirection = sign;
                            }
                            previous = current;
                        }

                        if (localDirection != 0)
                        {
                            if (direction != 0 && direction != localDirection) throw NotApplicable{};
                            direction = localDirection;
                        }
                        else if (direction != 0)
                        {
                            throw NotApplicable{};
                        }
                    }
                }

                BoundKind kind = direction > 0 ? BoundKind::Upper :
                    direction < 0 ? BoundKind::Lower : BoundKind::Lateral;
                bounds.push_back({ &inequality, kind });
                lowerCount += kind == BoundKind::Lower;
                upperCount += kind == BoundKind::Upper;
            }

            if (lowerCount == 0 || upperCount == 0) throw NotApplicable{};
            return bounds;
        }

        class VerticalDomain
        {
        public:
            VerticalDomain(const vector<Bound>& bounds, Point3D min, Point3D max)
                : bounds_(bounds), min_(min), max_(max) {
            }

            // Ограничения по z ищем бисекцией; область должна лежать внутри 3D-бокса.
            bool GetHeights(double x, double y, Heights& heights) const
            {
                heights = { min_.Z, max_.Z };

                for (const Bound& bound : bounds_)
                {
                    if (bound.Kind == BoundKind::Lateral) continue;

                    double a = bound.Constraint->Evaluate(x, y, min_.Z);
                    double b = bound.Constraint->Evaluate(x, y, max_.Z);
                    if (!isfinite(a) || !isfinite(b)) throw NotApplicable{};

                    double epsilon = 1e-10 * std::max({ 1.0, abs(a), abs(b) });
                    if (bound.Kind == BoundKind::Upper)
                    {
                        if (a > epsilon) return false;
                        if (b < -epsilon) throw NotApplicable{};
                    }
                    else
                    {
                        if (b > epsilon) return false;
                        if (a < -epsilon) throw NotApplicable{};
                    }

                    double low = min_.Z;
                    double high = max_.Z;

                    for (int i = 0; i < 45; ++i)
                    {
                        double mid = low + (high - low) * 0.5;
                        double value = bound.Constraint->Evaluate(x, y, mid);
                        if (!isfinite(value)) throw NotApplicable{};

                        bool moveLow = bound.Kind == BoundKind::Upper ? value < 0.0 : value > 0.0;
                        if (moveLow) low = mid;
                        else high = mid;
                    }

                    double root = low + (high - low) * 0.5;
                    if (abs(a) <= epsilon) root = min_.Z;
                    if (abs(b) <= epsilon) root = max_.Z;
                    if (bound.Kind == BoundKind::Upper) heights.Upper = min(heights.Upper, root);
                    else heights.Lower = max(heights.Lower, root);
                }

                return true;
            }

            // Знак нужен для Marching Squares. Положительное значение означает снаружи.
            double Evaluate(double x, double y) const
            {
                double lateral = -numeric_limits<double>::infinity();

                for (const Bound& bound : bounds_)
                {
                    if (bound.Kind != BoundKind::Lateral) continue;
                    double value = bound.Constraint->Evaluate(x, y, (min_.Z + max_.Z) * 0.5);
                    if (!isfinite(value)) throw NotApplicable{};
                    lateral = max(lateral, value);
                }

                if (lateral > 0.0) return lateral;

                Heights heights;
                if (!GetHeights(x, y, heights)) return max(1e-8, -lateral + 1e-8);
                return max(lateral, heights.Lower - heights.Upper);
            }

            bool IsInside(double x, double y) const
            {
                Heights heights;
                return Evaluate(x, y) < 0.0 && GetHeights(x, y, heights) && heights.Lower < heights.Upper;
            }

        private:
            const vector<Bound>& bounds_;
            Point3D min_, max_;
        };

        struct Contours
        {
            vector<Point2> Points;
            vector<vector<int>> Loops;
        };

        Contours ExtractContours(const VerticalDomain& domain, Point3D min, Point3D max, int nx, int ny)
        {
            const size_t width = static_cast<size_t>(nx) + 1;
            auto Index = [width](int i, int j) { return static_cast<size_t>(j) * width + i; };
            double dx = (max.X - min.X) / nx;
            double dy = (max.Y - min.Y) / ny;
            vector<double> field(width * (static_cast<size_t>(ny) + 1));

            for (int j = 0; j <= ny; ++j)
            {
                for (int i = 0; i <= nx; ++i)
                {
                    field[Index(i, j)] = domain.Evaluate(min.X + dx * i, min.Y + dy * j);
                    if (!isfinite(field[Index(i, j)])) throw NotApplicable{};
                    if ((i == 0 || i == nx || j == 0 || j == ny) && field[Index(i, j)] <= 0.0)
                    {
                        throw DomainClipped{}; // Тело касается боковой границы расчётной области.
                    }
                }
            }

            Contours result;
            map<pair<size_t, size_t>, int> edgeCache;
            map<pair<long long, long long>, int> pointCache;
            vector<pair<int, int>> segments;

            auto EdgePoint = [&](int ia, int ja, int ib, int jb) -> int
                {
                    size_t a = Index(ia, ja);
                    size_t b = Index(ib, jb);
                    if ((field[a] <= 0.0) == (field[b] <= 0.0)) return -1;
                    auto key = minmax(a, b);
                    auto found = edgeCache.find(key);
                    if (found != edgeCache.end()) return found->second;

                    Point2 p{ min.X + dx * ia, min.Y + dy * ja };
                    Point2 q{ min.X + dx * ib, min.Y + dy * jb };
                    bool insideP = field[a] <= 0.0;
                    double low = 0.0, high = 1.0;

                    for (int iteration = 0; iteration < 45; ++iteration)
                    {
                        double t = (low + high) * 0.5;
                        double value = domain.Evaluate(p.X + (q.X - p.X) * t, p.Y + (q.Y - p.Y) * t);
                        if ((value <= 0.0) == insideP) low = t;
                        else high = t;
                    }

                    double t = (low + high) * 0.5;
                    Point2 point{ p.X + (q.X - p.X) * t, p.Y + (q.Y - p.Y) * t };
                    auto pointKey = make_pair(llround((point.X - min.X) / dx * 1e9),
                        llround((point.Y - min.Y) / dy * 1e9));
                    auto existing = pointCache.find(pointKey);
                    if (existing != pointCache.end())
                    {
                        edgeCache.emplace(key, existing->second);
                        return existing->second;
                    }

                    int index = static_cast<int>(result.Points.size());
                    result.Points.push_back(point);
                    pointCache.emplace(pointKey, index);
                    edgeCache.emplace(key, index);
                    return index;
                };

            for (int j = 0; j < ny; ++j)
            {
                for (int i = 0; i < nx; ++i)
                {
                    array<int, 4> edge{
                        EdgePoint(i, j, i + 1, j),
                        EdgePoint(i + 1, j, i + 1, j + 1),
                        EdgePoint(i, j + 1, i + 1, j + 1),
                        EdgePoint(i, j, i, j + 1)
                    };

                    vector<int> hits;
                    for (int e = 0; e < 4; ++e) if (edge[e] >= 0) hits.push_back(e);

                    if (hits.size() == 2)
                    {
                        segments.emplace_back(edge[hits[0]], edge[hits[1]]);
                    }
                    else if (hits.size() == 4)
                    {
                        bool bottomLeftInside = field[Index(i, j)] <= 0.0;
                        bool centerInside = domain.Evaluate(min.X + (i + 0.5) * dx, min.Y + (j + 0.5) * dy) <= 0.0;
                        bool connectAroundBottomLeft = bottomLeftInside != centerInside;

                        if (connectAroundBottomLeft)
                        {
                            segments.emplace_back(edge[0], edge[3]);
                            segments.emplace_back(edge[1], edge[2]);
                        }
                        else
                        {
                            segments.emplace_back(edge[0], edge[1]);
                            segments.emplace_back(edge[2], edge[3]);
                        }
                    }
                    else if (!hits.empty())
                    {
                        throw NotApplicable{};
                    }
                }
            }

            if (segments.empty()) throw NotApplicable{};
            vector<vector<int>> neighbors(result.Points.size());

            for (const auto& [a, b] : segments)
            {
                if (a == b) continue;
                neighbors[a].push_back(b);
                neighbors[b].push_back(a);
            }

            for (const auto& links : neighbors) if (links.size() != 2) throw NotApplicable{};
            vector<bool> visited(result.Points.size(), false);

            for (int start = 0; start < static_cast<int>(result.Points.size()); ++start)
            {
                if (visited[start]) continue;
                vector<int> loop;
                int previous = -1;
                int current = start;

                do
                {
                    if (visited[current]) throw NotApplicable{};
                    visited[current] = true;
                    loop.push_back(current);
                    int next = neighbors[current][0] == previous ? neighbors[current][1] : neighbors[current][0];
                    previous = current;
                    current = next;
                } while (current != start && loop.size() <= result.Points.size());

                if (current != start || loop.size() < 3) throw NotApplicable{};
                result.Loops.push_back(move(loop));
            }

            return result;
        }

        struct Plane
        {
            double A, B, C;
            double At(double x, double y) const { return A * x + B * y + C; }
        };

        Plane FitPlane(const vector<Point2>& polygon, const vector<double>& values, double tolerance)
        {
            const Point2& first = polygon.front();
            double z0 = values.front();
            double best = 0.0;
            size_t iBest = 0, jBest = 0;

            // Берём три максимально разнесённые точки, а не почти коллинеарные соседние.
            for (size_t i = 1; i < polygon.size(); ++i)
            {
                for (size_t j = i + 1; j < polygon.size(); ++j)
                {
                    double determinant = abs(Cross(first, polygon[i], polygon[j]));
                    if (determinant <= best) continue;
                    best = determinant;
                    iBest = i;
                    jBest = j;
                }
            }

            if (best <= tolerance * tolerance) throw NotApplicable{};
            const Point2& b = polygon[iBest];
            const Point2& c = polygon[jBest];
            double determinant = Cross(first, b, c);
            double dx1 = b.X - first.X, dy1 = b.Y - first.Y;
            double dx2 = c.X - first.X, dy2 = c.Y - first.Y;
            double dz1 = values[iBest] - z0, dz2 = values[jBest] - z0;
            Plane plane{ (dz1 * dy2 - dz2 * dy1) / determinant,
                (dx1 * dz2 - dx2 * dz1) / determinant, 0.0 };
            plane.C = z0 - plane.A * first.X - plane.B * first.Y;

            for (size_t k = 0; k < polygon.size(); ++k)
            {
                if (abs(plane.At(polygon[k].X, polygon[k].Y) - values[k]) > tolerance)
                {
                    throw NotApplicable{};
                }
            }

            return plane;
        }

        TriangleMesh BuildPlanarBody(const VerticalDomain& domain, const Contours& contours,
            Point3D min, Point3D max, int nx, int ny)
        {
            vector<Point3D> vertices;
            vector<int> indices;
            const double dx = (max.X - min.X) / nx;
            const double dy = (max.Y - min.Y) / ny;
            const double scale = std::max({ max.X - min.X, max.Y - min.Y, max.Z - min.Z });
            const double tolerance = std::max(1e-10 * scale, 1e-12);

            auto AddTriangle = [&](int a, int b, int c)
                {
                    if (a == b || b == c || a == c) return;
                    Point3D p = vertices[a], q = vertices[b], r = vertices[c];
                    double ux = q.X - p.X, uy = q.Y - p.Y, uz = q.Z - p.Z;
                    double vx = r.X - p.X, vy = r.Y - p.Y, vz = r.Z - p.Z;
                    double area2 = (uy * vz - uz * vy) * (uy * vz - uz * vy) +
                        (uz * vx - ux * vz) * (uz * vx - ux * vz) +
                        (ux * vy - uy * vx) * (ux * vy - uy * vx);
                    if (area2 <= 1e-26 * scale * scale * scale * scale) return;
                    indices.insert(indices.end(), { a, b, c });
                };

            for (const vector<int>& loop : contours.Loops)
            {
                vector<Point2> polygon;
                polygon.reserve(loop.size());
                for (int index : loop) polygon.push_back(contours.Points[index]);

                if (SignedArea(polygon) < 0.0) reverse(polygon.begin(), polygon.end());
                if (SignedArea(polygon) <= 1e-10 * dx * dy) throw NotApplicable{};

                // Этот путь предназначен для выпуклых контуров. Для вогнутых и с отверстиями — MC.
                for (size_t i = 0; i < polygon.size(); ++i)
                {
                    if (Cross(polygon[i], polygon[(i + 1) % polygon.size()],
                        polygon[(i + 2) % polygon.size()]) < -1e-6 * dx * dy)
                    {
                        throw NotApplicable{};
                    }
                }

                Point2 center{ 0, 0 };
                vector<double> bottom, top;
                for (const Point2& p : polygon)
                {
                    Heights heights;
                    if (!domain.GetHeights(p.X, p.Y, heights)) throw NotApplicable{};
                    bottom.push_back(heights.Lower);
                    top.push_back(heights.Upper);
                    center.X += p.X;
                    center.Y += p.Y;
                }
                center.X /= polygon.size();
                center.Y /= polygon.size();
                if (!domain.IsInside(center.X, center.Y)) throw NotApplicable{};

                // Проверяем планарность обеих крышек во всех точках контура и в центре.
                Plane lowPlane = FitPlane(polygon, bottom, tolerance);
                Plane highPlane = FitPlane(polygon, top, tolerance);
                Heights centerHeights;
                if (!domain.GetHeights(center.X, center.Y, centerHeights)) throw NotApplicable{};
                if (abs(lowPlane.At(center.X, center.Y) - centerHeights.Lower) > tolerance ||
                    abs(highPlane.At(center.X, center.Y) - centerHeights.Upper) > tolerance)
                {
                    throw NotApplicable{};
                }

                // Планарность проверяем и внутри области: одной проверки контура недостаточно.
                for (int iy = 1; iy < 9; ++iy)
                {
                    for (int ix = 1; ix < 9; ++ix)
                    {
                        double x = min.X + (max.X - min.X) * (static_cast<double>(ix) / 9.0);
                        double y = min.Y + (max.Y - min.Y) * (static_cast<double>(iy) / 9.0);
                        if (!domain.IsInside(x, y)) continue;
                        Heights sample;
                        if (!domain.GetHeights(x, y, sample)) throw NotApplicable{};
                        if (abs(lowPlane.At(x, y) - sample.Lower) > tolerance ||
                            abs(highPlane.At(x, y) - sample.Upper) > tolerance)
                        {
                            throw NotApplicable{};
                        }
                    }
                }

                vector<int> bottomIndices, topIndices;
                for (size_t i = 0; i < polygon.size(); ++i)
                {
                    int low = static_cast<int>(vertices.size());
                    vertices.push_back({ polygon[i].X, polygon[i].Y, bottom[i] });
                    int high = low;
                    if (abs(top[i] - bottom[i]) > tolerance)
                    {
                        high = static_cast<int>(vertices.size());
                        vertices.push_back({ polygon[i].X, polygon[i].Y, top[i] });
                    }
                    bottomIndices.push_back(low);
                    topIndices.push_back(high);
                }

                int centerBottom = static_cast<int>(vertices.size());
                vertices.push_back({ center.X, center.Y, centerHeights.Lower });
                int centerTop = static_cast<int>(vertices.size());
                vertices.push_back({ center.X, center.Y, centerHeights.Upper });

                for (size_t i = 0; i < polygon.size(); ++i)
                {
                    size_t next = (i + 1) % polygon.size();
                    AddTriangle(centerTop, topIndices[i], topIndices[next]);
                    AddTriangle(centerBottom, bottomIndices[next], bottomIndices[i]);
                    AddTriangle(bottomIndices[i], bottomIndices[next], topIndices[next]);
                    AddTriangle(bottomIndices[i], topIndices[next], topIndices[i]);
                }
            }

            TriangleMesh mesh(move(vertices), move(indices));
            if (!VolumeCalculator().TryCalculate(mesh)) throw NotApplicable{};
            return mesh;
        }

        TriangleMesh TryBuildOptimized(const InequalityRegion& region, Point3D min, Point3D max, int nx, int ny)
        {
            vector<Bound> bounds = Classify(region, min, max);
            VerticalDomain domain(bounds, min, max);
            Contours contours = ExtractContours(domain, min, max, nx, ny);
            if (contours.Loops.size() != 1) throw NotApplicable{}; // Отверстия/несколько контуров пока не обслуживаем.
            return BuildPlanarBody(domain, contours, min, max, nx, ny);
        }
    }

    TriangleMesh InequalityBodyBuilder::Build(const InequalityRegion& region,
        Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ) const
    {
        // Общие проверки области и предельного расхода памяти оставляем основному построителю.
        // Специализированный путь применяется только к подходящим телам.
        if (cellsX >= 2 && cellsY >= 2 && cellsZ >= 2 && cellsX <= 512 && cellsY <= 512)
        {
            try
            {
                return TryBuildOptimized(region, min, max, cellsX, cellsY);
            }
            catch (const DomainClipped&)
            {
                throw invalid_argument("Тело касается границ области построения. Включите автоподбор или расширьте диапазоны X и Y.");
            }
            catch (const NotApplicable&)
            {
                // Неподходящая форма тела: сохраняем старый универсальный алгоритм.
            }
        }

        return ImplicitSurfaceBuilder().Build([&region](double x, double y, double z)
            {
                return region.Evaluate(x, y, z);
            }, min, max, cellsX, cellsY, cellsZ);
    }
}
