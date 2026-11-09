#include "ImplicitSurfaceBuilder.h"
#include "MarchingCubesTables.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Expressions;

namespace IceCream::Core::Services
{
    /// <summary>
    /// Строит треугольную сетку неявной поверхности g(x, y, z) = 0.
    /// </summary>
    TriangleMesh ImplicitSurfaceBuilder::Build(const CompiledExpression& expression, Point3D min, Point3D max, int cellsX, int cellsY, int cellsZ) const
    {
        if (!expression)
        {
            throw invalid_argument("Не задана функция для построения поверхности.");
        }

        if (cellsX < 1 || cellsY < 1 || cellsZ < 1)
        {
            throw invalid_argument("Количество ячеек сетки по каждой оси должно быть положительным.");
        }

        for (double value : {min.X, min.Y, min.Z, max.X, max.Y, max.Z})
        {
            if (!isfinite(value))
            {
                throw invalid_argument("Границы области построения должны быть конечными числами.");
            }
        }

        if (min.X >= max.X || min.Y >= max.Y || min.Z >= max.Z)
        {
            throw invalid_argument("Минимальные границы области должны быть меньше максимальных.");
        }

        const double sizeX = max.X - min.X;
        const double sizeY = max.Y - min.Y;
        const double sizeZ = max.Z - min.Z;

        if (!isfinite(sizeX) || !isfinite(sizeY) || !isfinite(sizeZ))
        {
            throw invalid_argument("Слишком большие размеры области построения.");
        }

        const double stepX = sizeX / cellsX;
        const double stepY = sizeY / cellsY;
        const double stepZ = sizeZ / cellsZ;

        if (stepX <= 0.0 || stepY <= 0.0 || stepZ <= 0.0 ||
            min.X + stepX == min.X || min.Y + stepY == min.Y || min.Z + stepZ == min.Z)
        {
            throw invalid_argument("Шаг сетки слишком мал для заданной области и разрешения.");
        }

        const size_t countX = static_cast<size_t>(cellsX) + 1;
        const size_t countY = static_cast<size_t>(cellsY) + 1;
        const size_t countZ = static_cast<size_t>(cellsZ) + 1;

        // Ограничиваем расход памяти, а не диапазоны координат.
        constexpr size_t maxGridNodes = 20'000'000;

        if (countX > maxGridNodes / countY || countX * countY > maxGridNodes / countZ)
        {
            throw invalid_argument("Слишком большое количество узлов трёхмерной сетки. Уменьшите число ячеек.");
        }

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

                return { min.X + i * stepX, min.Y + j * stepY, min.Z + k * stepZ };
            };

        // Вычисление скалярного поля в узлах сетки.
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
                        throw invalid_argument("Функция принимает недопустимое значение в заданной области.");
                    }

                    values[Index(i, j, k)] = value;
                }
            }
        }

        // Смещение значений, слишком близких к нулю, для уменьшения вырожденных случаев.
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

                            if (!isfinite(difference))
                            {
                                throw invalid_argument("Слишком большой перепад значений функции между соседними узлами сетки.");
                            }

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

        // Поиск кубов, пересекаемых поверхностью.
        for (int k = 0; k < cellsZ; ++k)
        {
            for (int j = 0; j < cellsY; ++j)
            {
                for (int i = 0; i < cellsX; ++i)
                {
                    array<size_t, 8> cube = {
                        Index(i, j, k), Index(i + 1, j, k),
                        Index(i + 1, j + 1, k), Index(i, j + 1, k),
                        Index(i, j, k + 1), Index(i + 1, j, k + 1),
                        Index(i + 1, j + 1, k + 1), Index(i, j + 1, k + 1)
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

        const double maxStep = std::max(stepX, std::max(stepY, stepZ));

        vector<Point3D> vertices;
        vector<int> indices;
        map<pair<size_t, size_t>, int> vertexCache;

        // Построение треугольников по таблице Marching Cubes.
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

                // Нормализация предотвращает переполнение при вычислении разности значений.
                double scale = std::max(abs(valueA), abs(valueB));
                double scaledA = valueA / scale;
                double scaledB = valueB / scale;
                double t = scaledA / (scaledA - scaledB);

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
                    throw logic_error("Ошибка триангуляции Marching Cubes: отсутствует точка пересечения ребра.");
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

                // Сравнение относительной площади не переполняется при больших координатах.
                double ux = (p1.X - p0.X) / maxStep;
                double uy = (p1.Y - p0.Y) / maxStep;
                double uz = (p1.Z - p0.Z) / maxStep;

                double vx = (p2.X - p0.X) / maxStep;
                double vy = (p2.Y - p0.Y) / maxStep;
                double vz = (p2.Z - p0.Z) / maxStep;

                double nx = uy * vz - uz * vy;
                double ny = uz * vx - ux * vz;
                double nz = ux * vy - uy * vx;

                double areaSquared4 = nx * nx + ny * ny + nz * nz;

                if (areaSquared4 <= 1e-24)
                {
                    continue;
                }

                // Сохраняем ориентацию треугольников из исходной реализации.
                indices.push_back(a);
                indices.push_back(c);
                indices.push_back(b);
            }
        }

        return TriangleMesh(move(vertices), move(indices));
    }
}
