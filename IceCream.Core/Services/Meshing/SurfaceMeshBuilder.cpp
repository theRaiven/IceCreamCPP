#include "SurfaceMeshBuilder.h"

#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace IceCream::Core::Services
{
    TriangleMesh SurfaceMeshBuilder::Build(const IRadialProfile& profile, int heightSegments, int radialSegments) const
    {
        if (heightSegments < 2)
        {
            throw invalid_argument("Количество сегментов по высоте должно быть не меньше 2.");
        }

        if (radialSegments < 3)
        {
            throw invalid_argument("Количество сегментов по окружности должно быть не меньше 3.");
        }

        const double minZ = profile.MinZ();
        const double maxZ = profile.MaxZ();
        const double height = maxZ - minZ;

        if (!isfinite(minZ) || !isfinite(maxZ) || !isfinite(height) || height <= 0.0)
        {
            throw invalid_argument("Границы профиля по Z должны быть конечными числами, причём нижняя граница должна быть меньше верхней.");
        }

        // Индексы TriangleMesh имеют тип int.
        const size_t rings = static_cast<size_t>(heightSegments) + 1;
        const size_t maxVertices = static_cast<size_t>(numeric_limits<int>::max());

        if (static_cast<size_t>(radialSegments) > (maxVertices - 2) / rings)
        {
            throw length_error("Слишком большое количество вершин для треугольной сетки.");
        }

        vector<Point3D> vertices;
        vector<int> indices;

        vector<int> firstRing = CreateRing(profile, minZ, radialSegments, vertices);

        if (firstRing.size() > 1)
        {
            int center = static_cast<int>(vertices.size());
            vertices.push_back({0, 0, minZ});
            ConnectRing({center}, firstRing, indices);
        }

        vector<int> lastRing = BuildPart(profile, heightSegments, radialSegments, firstRing, vertices, indices);

        if (lastRing.size() > 1)
        {
            int center = static_cast<int>(vertices.size());
            vertices.push_back({0, 0, maxZ});
            ConnectRing(lastRing, {center}, indices);
        }

        return TriangleMesh(move(vertices), move(indices));
    }

    vector<int> SurfaceMeshBuilder::BuildPart(const IRadialProfile& profile, int heightSegments, int radialSegments, const vector<int>& firstRing, vector<Point3D>& vertices, vector<int>& indices)
    {
        vector<int> previousRing = firstRing;
        const double minZ = profile.MinZ();
        const double maxZ = profile.MaxZ();

        for (int i = 1; i <= heightSegments; ++i)
        {
            double t = static_cast<double>(i) / heightSegments;
            double z = i == heightSegments ? maxZ : minZ * (1.0 - t) + maxZ * t;

            vector<int> currentRing = CreateRing(profile, z, radialSegments, vertices);
            ConnectRing(previousRing, currentRing, indices);

            previousRing = move(currentRing);
        }

        return previousRing;
    }

    vector<int> SurfaceMeshBuilder::CreateRing(const IRadialProfile& profile, double z, int radialSegments, vector<Point3D>& vertices)
    {
        double radius = profile.GetRadius(z);

        if (!isfinite(radius) || radius < 0.0)
        {
            throw invalid_argument("Радиус профиля должен быть конечным неотрицательным числом.");
        }

        if (vertices.size() > static_cast<size_t>(numeric_limits<int>::max()) - static_cast<size_t>(radialSegments))
        {
            throw length_error("Слишком большое количество вершин для треугольной сетки.");
        }

        // Нулевой радиус соответствует одной вершине на оси вращения.
        if (radius == 0.0)
        {
            int center = static_cast<int>(vertices.size());
            vertices.push_back({0, 0, z});
            return {center};
        }

        vector<int> ringIndices(radialSegments);
        const double angleStep = 2.0 * acos(-1.0) / radialSegments;

        for (int i = 0; i < radialSegments; ++i)
        {
            double angle = i * angleStep;
            double x = radius * cos(angle);
            double y = radius * sin(angle);

            ringIndices[i] = static_cast<int>(vertices.size());
            vertices.push_back({x, y, z});
        }

        return ringIndices;
    }

    void SurfaceMeshBuilder::ConnectRing(const vector<int>& lower, const vector<int>& upper, vector<int>& indices)
    {
        if (lower.empty() || upper.empty())
        {
            throw invalid_argument("Кольца не должны быть пустыми.");
        }

        if (lower.size() == 1 && upper.size() == 1)
        {
            throw invalid_argument("Невозможно построить поверхность между двумя вершинами на оси вращения.");
        }

        if (lower.size() == 1)
        {
            // Нижний полюс.
            int pole = lower[0];

            for (size_t i = 0; i < upper.size(); ++i)
            {
                size_t next = (i + 1) % upper.size();
                AddTriangle(indices, pole, upper[next], upper[i]);
            }

            return;
        }

        if (upper.size() == 1)
        {
            // Верхний полюс.
            int pole = upper[0];

            for (size_t i = 0; i < lower.size(); ++i)
            {
                size_t next = (i + 1) % lower.size();
                AddTriangle(indices, lower[i], lower[next], pole);
            }

            return;
        }

        if (lower.size() != upper.size())
        {
            throw invalid_argument("Соседние кольца должны содержать одинаковое количество вершин.");
        }

        // Соединение двух обычных колец.
        for (size_t i = 0; i < lower.size(); ++i)
        {
            size_t next = (i + 1) % lower.size();

            int a = lower[i];
            int b = lower[next];
            int c = upper[i];
            int d = upper[next];

            AddTriangle(indices, a, b, c);
            AddTriangle(indices, b, d, c);
        }
    }

    void SurfaceMeshBuilder::AddTriangle(vector<int>& indices, int a, int b, int c)
    {
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    }
}
