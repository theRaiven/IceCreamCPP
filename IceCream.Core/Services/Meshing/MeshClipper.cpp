#include "MeshClipper.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

using namespace std;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Inequalities;

namespace IceCream::Core::Services
{
    namespace
    {
        struct Face
        {
            array<int, 3> Indices;
            size_t Source;
            int operator[](size_t i) const { return Indices[i]; }
        };
        using Edge = pair<int, int>;

        Edge MakeEdge(int a, int b)
        {
            return minmax(a, b);
        }

        Point3D Lerp(const Point3D& a, const Point3D& b, double t)
        {
            return { a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, a.Z + (b.Z - a.Z) * t };
        }

        double LengthSquared(const Point3D& a, const Point3D& b)
        {
            double x = a.X - b.X, y = a.Y - b.Y, z = a.Z - b.Z;
            return x * x + y * y + z * z;
        }

        bool Degenerate(const Point3D& a, const Point3D& b, const Point3D& c)
        {
            double ux = b.X - a.X, uy = b.Y - a.Y, uz = b.Z - a.Z;
            double vx = c.X - a.X, vy = c.Y - a.Y, vz = c.Z - a.Z;
            double nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
            double scale = max({ LengthSquared(a, b), LengthSquared(b, c), LengthSquared(c, a) });
            return scale == 0.0 || nx * nx + ny * ny + nz * nz <= 1e-26 * scale * scale;
        }

        double Value(const Inequality& inequality, const Point3D& p)
        {
            double value = inequality.Evaluate(p.X, p.Y, p.Z);
            if (!isfinite(value)) throw invalid_argument("Бесконечное значение неравенства");
            return value;
        }

        void AddFace(vector<Face>& faces, const vector<Point3D>& vertices, int a, int b, int c, size_t limit, size_t source)
        {
            if (a == b || b == c || c == a) return;
            if (Degenerate(vertices[a], vertices[b], vertices[c])) return;
            if (faces.size() >= limit) throw length_error("MeshClipper превысил максимальное количество треугольников");
            faces.push_back({ a, b, c });
        }

        bool ShouldRefine(const Face& face, const vector<Point3D>& vertices, const InequalityRegion& region, double maxEdgeLength)
        {
            const Point3D& a = vertices[face[0]];
            const Point3D& b = vertices[face[1]];
            const Point3D& c = vertices[face[2]];

            if (maxEdgeLength > 0.0 && max({ LengthSquared(a, b), LengthSquared(b, c), LengthSquared(c, a) }) > maxEdgeLength * maxEdgeLength) return true;

            Point3D ab = Lerp(a, b, 0.5), bc = Lerp(b, c, 0.5), ca = Lerp(c, a, 0.5);
            Point3D center = { (a.X + b.X + c.X) / 3, (a.Y + b.Y + c.Y) / 3, (a.Z + b.Z + c.Z) / 3 };
            for (const Inequality& inequality : region.GetInequalities())
            {
                array<double, 7> values = { Value(inequality, a), Value(inequality, b), Value(inequality, c), Value(inequality, ab), Value(inequality, bc), Value(inequality, ca), Value(inequality, center) };
                bool inside = false, outside = false;
                for (double v : values) { inside |= v <= 0.0; outside |= v > 0.0; }
                if (inside && outside) return true;
            }
            return false;
        }

        void Refine(vector<Point3D>& vertices, vector<Face>& faces, const InequalityRegion& region, const MeshClipperOptions& options)
        {
            for (int level = 0; level < options.maxSubdivisionDepth; ++level)
            {
                map<Edge, int> midpoints;
                for (const Face& face : faces)
                {
                    if (!ShouldRefine(face, vertices, region, options.maxEdgeLength)) continue;
                    for (int j = 0; j < 3; ++j) midpoints.emplace(MakeEdge(face[j], face[(j + 1) % 3]), -1);
                }
                if (midpoints.empty()) break;

                for (auto& entry : midpoints)
                {
                    if (vertices.size() >= static_cast<size_t>(numeric_limits<int>::max())) throw length_error("Слишком много вершин");
                    const Edge& edge = entry.first;
                    entry.second = static_cast<int>(vertices.size());
                    vertices.push_back(Lerp(vertices[edge.first], vertices[edge.second], 0.5));
                }

                vector<Face> next;
                for (const Face& face : faces)
                {
                    vector<int> polygon;
                    for (int j = 0; j < 3; ++j)
                    {
                        int a = face[j], b = face[(j + 1) % 3];
                        polygon.push_back(a);
                        auto it = midpoints.find(MakeEdge(a, b));
                        if (it != midpoints.end()) polygon.push_back(it->second);
                    }
                    if (polygon.size() == 3)
                    {
                        AddFace(next, vertices, face[0], face[1], face[2], options.maxTriangles, face.Source);
                        continue;
                    }
                    if (vertices.size() >= static_cast<size_t>(numeric_limits<int>::max())) throw length_error("Слишком много вершин");
                    const Point3D& a = vertices[face[0]];
                    const Point3D& b = vertices[face[1]];
                    const Point3D& c = vertices[face[2]];
                    int center = static_cast<int>(vertices.size());
                    vertices.push_back({ (a.X + b.X + c.X) / 3, (a.Y + b.Y + c.Y) / 3, (a.Z + b.Z + c.Z) / 3 });
                    for (size_t j = 0; j < polygon.size(); ++j) AddFace(next, vertices, polygon[j], polygon[(j + 1) % polygon.size()], center, options.maxTriangles, face.Source);
                }
                faces.swap(next);
            }
        }

        int Intersect(int a, int b, const Inequality& inequality, vector<Point3D>& vertices, vector<double>& values, map<Edge, int>& cache)
        {
            if (values[a] == 0.0) return a;
            if (values[b] == 0.0) return b;
            Edge edge = MakeEdge(a, b);
            auto found = cache.find(edge);
            if (found != cache.end()) return found->second;

            int loIndex = edge.first, hiIndex = edge.second;
            const Point3D& p = vertices[loIndex];
            const Point3D& q = vertices[hiIndex];
            double lo = 0.0, hi = 1.0;
            bool startInside = values[loIndex] <= 0.0;
            for (int i = 0; i < 35; ++i)
            {
                double middle = (lo + hi) * 0.5;
                double midValue = Value(inequality, Lerp(p, q, middle));
                if ((midValue <= 0.0) == startInside) lo = middle;
                else hi = middle;
            }
            if (vertices.size() >= static_cast<size_t>(numeric_limits<int>::max())) throw length_error("Слишком много вершин");
            int index = static_cast<int>(vertices.size());
            Point3D intersection = Lerp(p, q, (lo + hi) * 0.5);
            vertices.push_back(intersection);
            values.push_back(0.0);
            cache.emplace(edge, index);
            return index;
        }

        vector<int> ClipPolygon(const Face& face, const Inequality& inequality, vector<Point3D>& vertices, vector<double>& values, map<Edge, int>& cache)
        {
            vector<int> polygon = { face[0], face[1], face[2] };
            vector<int> result;
            int previous = polygon.back();
            for (int current : polygon)
            {
                bool wasInside = values[previous] <= 0.0, isInside = values[current] <= 0.0;
                if (wasInside != isInside)
                {
                    int intersection = Intersect(previous, current, inequality, vertices, values, cache);
                    if (result.empty() || result.back() != intersection) result.push_back(intersection);
                }
                if (isInside && (result.empty() || result.back() != current)) result.push_back(current);
                previous = current;
            }
            if (result.size() > 1 && result.front() == result.back()) result.pop_back();
            return result;
        }

        TriangleMesh Pack(const vector<Point3D>& vertices, const vector<Face>& faces)
        {
            vector<Point3D> outputVertices;
            vector<int> outputIndices;
            vector<int> oldToNew(vertices.size(), -1);
            outputIndices.reserve(faces.size() * 3);
            for (const Face& face : faces)
            {
                for (int id : face.Indices)
                {
                    if (oldToNew[id] < 0)
                    {
                        if (outputVertices.size() >= static_cast<size_t>(numeric_limits<int>::max())) throw length_error("Слишком много вершин");
                        oldToNew[id] = static_cast<int>(outputVertices.size());
                        outputVertices.push_back(vertices[id]);
                    }
                    outputIndices.push_back(oldToNew[id]);
                }
            }
            return TriangleMesh(move(outputVertices), move(outputIndices));
        }
    }

    TriangleMesh MeshClipper::Clip(const TriangleMesh& mesh, const InequalityRegion& region, const MeshClipperOptions& options, vector<size_t>* sourceTriangles) const
    {
        if (options.maxSubdivisionDepth < 0 || options.maxSubdivisionDepth > 12 || !isfinite(options.maxEdgeLength) || options.maxEdgeLength < 0 || options.maxTriangles == 0)
        {
            throw invalid_argument("Неверные параметры MeshClipper");
        }
        if (mesh.TriangleCount() > options.maxTriangles) throw length_error("MeshClipper превысил максимальное количество треугольников");

        vector<Point3D> vertices;
        vector<Face> faces;
        map<tuple<double, double, double>, int> canonical;
        vector<int> indicesMap(mesh.Vertices().size());
        for (size_t i = 0; i < mesh.Vertices().size(); ++i)
        {
            const Point3D& p = mesh.Vertices()[i];
            auto key = make_tuple(p.X, p.Y, p.Z);
            auto found = canonical.find(key);
            if (found == canonical.end())
            {
                if (vertices.size() >= static_cast<size_t>(numeric_limits<int>::max())) throw length_error("Слишком много вершин");
                int id = static_cast<int>(vertices.size());
                vertices.push_back(p);
                canonical.emplace(key, id);
                indicesMap[i] = id;
            }
            else indicesMap[i] = found->second;
        }
        const auto& indices = mesh.Indices();
        for (size_t i = 0; i < indices.size(); i += 3) AddFace(faces, vertices, indicesMap[indices[i]], indicesMap[indices[i + 1]], indicesMap[indices[i + 2]], options.maxTriangles, i / 3);

        Refine(vertices, faces, region, options);
        for (const Inequality& inequality : region.GetInequalities())
        {
            vector<double> values;
            values.reserve(vertices.size());
            for (const Point3D& point : vertices) values.push_back(Value(inequality, point));
            map<Edge, int> intersections;
            vector<Face> next;
            for (const Face& face : faces)
            {
                vector<int> polygon = ClipPolygon(face, inequality, vertices, values, intersections);
                for (size_t j = 1; j + 1 < polygon.size(); ++j) AddFace(next, vertices, polygon[0], polygon[j], polygon[j + 1], options.maxTriangles, face.Source);
            }
            faces.swap(next);
            if (faces.empty()) break;
        }
        auto result = Pack(vertices, faces);
        if (sourceTriangles)
        {
            sourceTriangles->clear();
            sourceTriangles->reserve(faces.size());
            for (const auto& face : faces) sourceTriangles->push_back(face.Source);
        }
        return result;
    }
}
