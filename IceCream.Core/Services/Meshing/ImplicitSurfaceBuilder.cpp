
#include "ImplicitSurfaceBuilder.h"
#include "MarchingCubesTables.h"

#include <utility>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <array>
#include <map>
#include <algorithm>

using namespace std;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Expressions;

namespace IceCream::Core::Services
{
    /// <summary>
	/// Строит треугольную сетку, представляющую неявную поверхность, заданную скалярным полем.
    /// </summary>
    TriangleMesh ImplicitSurfaceBuilder::Build(const CompiledExpression& expression, Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ) const
    {
        if (!expression)
        {
            throw invalid_argument("Expression must not be empty");
        }

        if (cellsX < 1 || cellsY < 1 || cellsZ < 1)
        {
            throw invalid_argument("Grid resolution must be positive");
        }

        if (!isfinite(min.X) || !isfinite(min.Y) || !isfinite(min.Z) ||
            !isfinite(max.X) || !isfinite(max.Y) || !isfinite(max.Z))
        {
            throw invalid_argument("Grid bounds must be finite");
        }

        if (min.X >= max.X || min.Y >= max.Y || min.Z >= max.Z)
        {
            throw invalid_argument("Invalid grid bounds");
        }

        double stepX = (max.X - min.X) / cellsX;
        double stepY = (max.Y - min.Y) / cellsY;
        double stepZ = (max.Z - min.Z) / cellsZ;

        size_t countX = static_cast<size_t>(cellsX) + 1;
        size_t countY = static_cast<size_t>(cellsY) + 1;
        size_t countZ = static_cast<size_t>(cellsZ) + 1;

        vector<double> values(countX * countY * countZ);

        auto Index = [countX, countY](size_t i, size_t j, size_t k)
            {
                return (k * countY + j) * countX + i;
            };


        auto GetPoint = [min, stepX, stepY, stepZ, countX, countY](size_t index) -> Point3D
            {
                size_t i = index % countX;
                size_t j = (index / countX) % countY;
                size_t k = index / (countX * countY);

                return {
                    min.X + i * stepX,
                    min.Y + j * stepY,
                    min.Z + k * stepZ
                };
            };


        for (int k = 0; k <= cellsZ; ++k)
        {
            for (int j = 0; j <= cellsY; ++j)
            {
                for (int i = 0; i <= cellsX; ++i)
                {
                    double x = min.X + i * stepX;
                    double y = min.Y + j * stepY;
                    double z = min.Z + k * stepZ;

                    double value = expression(x, y, z);

                    if (!isfinite(value))
                    {
                        throw invalid_argument("Non-finite scalar field value");
                    }

                    values[Index(i, j, k)] = value;
                }
            }
        }

        vector<double> correctedValues = values;

        for (int k = 0; k <= cellsZ; ++k)
        {
            for (int j = 0; j <= cellsY; ++j)
            {
                for (int i = 0; i <= cellsX; ++i)
                {
                    size_t index = Index(i, j, k);
                    double localScale = 0.0;

                    auto ConsiderNeighbor = [&](size_t ni, size_t nj, size_t nk)
                        {
                            double difference = abs(values[index] - values[Index(ni, nj, nk)]);
                            localScale = std::max(localScale, difference);
                        };

                    if (i > 0) ConsiderNeighbor(i - 1, j, k);
                    if (i < cellsX) ConsiderNeighbor(i + 1, j, k);

                    if (j > 0) ConsiderNeighbor(i, j - 1, k);
                    if (j < cellsY) ConsiderNeighbor(i, j + 1, k);

                    if (k > 0) ConsiderNeighbor(i, j, k - 1);
                    if (k < cellsZ) ConsiderNeighbor(i, j, k + 1);

                    double epsilon = localScale * 1e-4;

                    if (epsilon > 0.0 && abs(values[index]) < epsilon)
                    {
                        correctedValues[index] = epsilon;
                    }
                }
            }
        }

        values.swap(correctedValues);


        struct ActiveCube
        {
            array<size_t, 8> corners;
            unsigned int configuration;
        };

        vector<ActiveCube> activeCubes;

        for (int k = 0; k < cellsZ; ++k)
        {
            for (int j = 0; j < cellsY; ++j)
            {
                for (int i = 0; i < cellsX; ++i)
                {
                    array<size_t, 8> cube = {
                        Index(i, j, k),
                        Index(i + 1, j, k),
                        Index(i + 1, j + 1, k),
                        Index(i, j + 1, k),
                        Index(i, j, k + 1),
                        Index(i + 1, j, k + 1),
                        Index(i + 1, j + 1, k + 1),
                        Index(i, j + 1, k + 1)
                    };

                    unsigned int configuration = 0;

                    for (int vertex = 0; vertex < 8; ++vertex)
                    {
                        if (values[cube[vertex]] < 0.0)
                        {
                            configuration |= 1u << vertex;
                        }
                    }

                    if (configuration == 0 || configuration == 255)
                    {
                        continue;
                    }

                    activeCubes.push_back({ cube, configuration });
                }
            }
        }

        const int cubeEdges[12][2] = {
            {0, 1}, {1, 2}, {2, 3}, {3, 0},
            {4, 5}, {5, 6}, {6, 7}, {7, 4},
            {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };

        vector<Point3D> vertices;
        vector<int> indices;
        map<pair<size_t, size_t>, int> vertexCache;

        for (const ActiveCube& cube : activeCubes)
        {
            array<Point3D, 12> edgePoints{};
            array<bool, 12> intersected{};

            for (int edge = 0; edge < 12; ++edge)
            {
                size_t indexA = cube.corners[cubeEdges[edge][0]];
                size_t indexB = cube.corners[cubeEdges[edge][1]];

                double valueA = values[indexA];
                double valueB = values[indexB];

                if ((valueA < 0.0) == (valueB < 0.0))
                {
                    continue;
                }

                Point3D a = GetPoint(indexA);
                Point3D b = GetPoint(indexB);

                double t = valueA / (valueA - valueB);

                edgePoints[edge] = {
                    a.X + t * (b.X - a.X),
                    a.Y + t * (b.Y - a.Y),
                    a.Z + t * (b.Z - a.Z)
                };

                intersected[edge] = true;
            }

            const unsigned char* triangles = MarchingCubesTriangleTable[cube.configuration];

            auto GetVertexIndex = [&](int edge) -> int
                {
                    size_t indexA = cube.corners[cubeEdges[edge][0]];
                    size_t indexB = cube.corners[cubeEdges[edge][1]];

                    pair<size_t, size_t> key;

                    if (values[indexA] == 0.0)
                    {
                        key = { indexA, indexA };
                    }
                    else if (values[indexB] == 0.0)
                    {
                        key = { indexB, indexB };
                    }
                    else
                    {
                        if (indexA > indexB)
                        {
                            swap(indexA, indexB);
                        }

                        key = { indexA, indexB };
                    }

                    auto it = vertexCache.find(key);

                    if (it != vertexCache.end())
                    {
                        return it->second;
                    }

                    int index = static_cast<int>(vertices.size());

                    if (key.first == key.second)
                    {
                        vertices.push_back(GetPoint(key.first));
                    }
                    else
                    {
                        vertices.push_back(edgePoints[edge]);
                    }

                    vertexCache.emplace(key, index);

                    return index;
                };

            for (int i = 0; i < 16 && triangles[i] != 255; i += 3)
            {
                int edgeA = triangles[i];
                int edgeB = triangles[i + 1];
                int edgeC = triangles[i + 2];

                if (!intersected[edgeA] || !intersected[edgeB] || !intersected[edgeC])
                {
                    throw logic_error("Invalid Marching Cubes triangulation");
                }

                int a = GetVertexIndex(edgeA);
                int b = GetVertexIndex(edgeB);
                int c = GetVertexIndex(edgeC);

                if (a == b || b == c || a == c)
                {
                    continue;
                }


                const Point3D& p0 = vertices[a];
                const Point3D& p1 = vertices[b];
                const Point3D& p2 = vertices[c];

                double ux = p1.X - p0.X;
                double uy = p1.Y - p0.Y;
                double uz = p1.Z - p0.Z;

                double vx = p2.X - p0.X;
                double vy = p2.Y - p0.Y;
                double vz = p2.Z - p0.Z;

                double nx = uy * vz - uz * vy;
                double ny = uz * vx - ux * vz;
                double nz = ux * vy - uy * vx;

                double areaSquared4 = nx * nx + ny * ny + nz * nz;

                double maxStep = std::max(stepX, std::max(stepY, stepZ));
                double tolerance = 1e-24 * maxStep * maxStep * maxStep * maxStep;

                if (areaSquared4 <= tolerance)
                {
                    continue;
                }

                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(b);
            }
        }

        return TriangleMesh(move(vertices), move(indices));
    }
}
