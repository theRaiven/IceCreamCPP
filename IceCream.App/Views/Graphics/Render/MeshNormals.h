#pragma once

#include "../../../../IceCream.Core/Geometry/Meshes/TriangleMesh.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <stdexcept>
#include <utility>
#include <vector>

namespace IceCream::App::Views::Graphics
{
    // Нормали для каждого угла треугольника. На гладких рёбрах нормали
    // усредняются, на острых границах и границах материалов остаются раздельными.
    inline std::vector<glm::vec3> CalculateCornerNormals(
        const Core::Geometry::TriangleMesh& mesh,
        const std::vector<std::array<float, 4>>& colors,
        double creaseAngleDegrees = 40.0)
    {
        const auto& points = mesh.Vertices();
        const auto& indices = mesh.Indices();
        const size_t triangleCount = mesh.TriangleCount();

        if (indices.size() != triangleCount * 3)
        {
            throw std::invalid_argument("Некорректный набор индексов треугольной сетки.");
        }

        if (!colors.empty() && colors.size() != triangleCount)
        {
            throw std::invalid_argument("Количество цветов не соответствует количеству треугольников.");
        }

        // На входе могут быть раздельные вершины с одинаковыми координатами.
        // Связываем их только для поиска действительно общих геометрических рёбер.
        std::map<std::array<double, 3>, size_t> pointIds;
        std::vector<size_t> canonical(points.size());

        for (size_t i = 0; i < points.size(); ++i)
        {
            const auto& p = points[i];
            const auto key = std::array<double, 3>{ p.X, p.Y, p.Z };
            auto [it, inserted] = pointIds.emplace(key, pointIds.size());
            canonical[i] = it->second;
        }

        struct EdgeUse
        {
            size_t Triangle;
            int FirstCorner;
            int SecondCorner;
            size_t FirstPoint;
            size_t SecondPoint;
        };

        std::map<std::pair<size_t, size_t>, std::vector<EdgeUse>> edges;
        std::vector<glm::dvec3> faceNormals(triangleCount, glm::dvec3(0.0));
        std::vector<glm::dvec3> cornerWeights(indices.size(), glm::dvec3(0.0));

        for (size_t triangle = 0; triangle < triangleCount; ++triangle)
        {
            const size_t start = triangle * 3;
            std::array<glm::dvec3, 3> p;

            for (int corner = 0; corner < 3; ++corner)
            {
                const int index = indices[start + corner];

                if (index < 0 || static_cast<size_t>(index) >= points.size())
                {
                    throw std::invalid_argument("Индекс треугольника выходит за пределы массива вершин.");
                }

                const auto& vertex = points[index];
                p[corner] = { vertex.X, vertex.Y, vertex.Z };
            }

            const glm::dvec3 cross = glm::cross(p[1] - p[0], p[2] - p[0]);
            const double length = glm::length(cross);

            if (length > 1e-20 && std::isfinite(length))
            {
                faceNormals[triangle] = cross / length;

                // Взвешиваем по углу в вершине: не искажаем освещение
                // из-за того, что соседние треугольники разного размера.
                for (int corner = 0; corner < 3; ++corner)
                {
                    glm::dvec3 u = p[(corner + 1) % 3] - p[corner];
                    glm::dvec3 v = p[(corner + 2) % 3] - p[corner];
                    double lu = glm::length(u);
                    double lv = glm::length(v);

                    if (lu > 0.0 && lv > 0.0)
                    {
                        double cosine = glm::clamp(glm::dot(u / lu, v / lv), -1.0, 1.0);
                        cornerWeights[start + corner] = faceNormals[triangle] * std::acos(cosine);
                    }
                }
            }

            for (int corner = 0; corner < 3; ++corner)
            {
                const int next = (corner + 1) % 3;
                const size_t a = canonical[indices[start + corner]];
                const size_t b = canonical[indices[start + next]];

                if (a != b)
                {
                    edges[std::minmax(a, b)].push_back({ triangle, corner, next, a, b });
                }
            }
        }

        std::vector<size_t> parent(indices.size());
        std::iota(parent.begin(), parent.end(), 0);

        auto root = [&](size_t i)
            {
                while (parent[i] != i)
                {
                    parent[i] = parent[parent[i]];
                    i = parent[i];
                }

                return i;
            };

        auto unite = [&](size_t a, size_t b)
            {
                a = root(a);
                b = root(b);
                if (a != b) parent[b] = a;
            };

        const double angle = glm::clamp(creaseAngleDegrees, 0.0, 180.0);
        const double threshold = std::cos(angle * 3.14159265358979323846 / 180.0);

        for (const auto& [edge, uses] : edges)
        {
            // Не соединяем нормали на границах и неманифолдных рёбрах.
            if (uses.size() != 2) continue;

            const EdgeUse& a = uses[0];
            const EdgeUse& b = uses[1];
            const glm::dvec3& na = faceNormals[a.Triangle];
            const glm::dvec3& nb = faceNormals[b.Triangle];

            if (glm::length(na) < 0.5 || glm::length(nb) < 0.5) continue;
            if (glm::dot(na, nb) < threshold) continue;
            if (!colors.empty() && colors[a.Triangle] != colors[b.Triangle]) continue;

            auto cornerAt = [](const EdgeUse& use, size_t point)
                {
                    return 3 * use.Triangle + static_cast<size_t>(use.FirstPoint == point ? use.FirstCorner : use.SecondCorner);
                };

            unite(cornerAt(a, edge.first), cornerAt(b, edge.first));
            unite(cornerAt(a, edge.second), cornerAt(b, edge.second));
        }

        std::vector<glm::dvec3> sums(indices.size(), glm::dvec3(0.0));

        for (size_t i = 0; i < indices.size(); ++i)
        {
            sums[root(i)] += cornerWeights[i];
        }

        std::vector<glm::vec3> normals(indices.size());

        for (size_t i = 0; i < indices.size(); ++i)
        {
            glm::dvec3 normal = sums[root(i)];
            double length = glm::length(normal);

            if (!(length > 1e-20) || !std::isfinite(length))
            {
                normal = faceNormals[i / 3];
                length = glm::length(normal);
            }

            if (!(length > 1e-20) || !std::isfinite(length))
            {
                normal = { 0.0, 0.0, 1.0 };
                length = 1.0;
            }

            normal /= length;
            normals[i] = glm::vec3(normal);
        }

        return normals;
    }
}
