
#include "HeightFieldBuilder.h"
#include "../../Profiles/Basic/FormulaProfile.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace std;

namespace IceCream::Core::Services
{
    HeightFieldSolid HeightFieldBuilder::Build(const string& formula, const HeightFieldBounds& b, int n) const
    {
        for (double value : {b.MinX, b.MaxX, b.MinY, b.MaxY})
        {
            if (!isfinite(value))
            {
                throw invalid_argument("Границы области должны быть конечными числами.");
            }
        }

        double sizeX = b.MaxX - b.MinX;
        double sizeY = b.MaxY - b.MinY;

        if (sizeX <= 0.0 || sizeY <= 0.0)
        {
            throw invalid_argument("Минимальные границы области должны быть меньше максимальных.");
        }

        if (!isfinite(sizeX) || !isfinite(sizeY))
        {
            throw invalid_argument("Слишком большой размер области построения.");
        }

        if (n < 2 || n > 256)
        {
            throw invalid_argument("Количество разбиений сетки должно быть от 2 до 256.");
        }

        string expression = formula;
        expression.erase(0, expression.find_first_not_of(" \t\r\n"));

        if (!expression.empty() && expression[0] == 'z')
        {
            size_t equals = expression.find_first_not_of(" \t", 1);

            if (equals != string::npos && expression[equals] == '=')
            {
                expression = expression.substr(equals + 1);
            }
        }

        auto f = Profiles::CompileExpression(expression, { 3, 1, 0.5, 1.2 });

        const int row = n + 1;
        const int count = row * row;

        vector<Geometry::Point3D> vertices;
        vertices.reserve(count * 2);

        double minZ = numeric_limits<double>::max();
        double maxZ = -minZ;

        // Построение верхней поверхности
        for (int j = 0; j <= n; ++j)
        {
            for (int i = 0; i <= n; ++i)
            {
                double x = b.MinX + (b.MaxX - b.MinX) * i / n;
                double y = b.MinY + (b.MaxY - b.MinY) * j / n;
                double z = f(x, y);

                if (!isfinite(z))
                {
                    throw invalid_argument("Функция принимает недопустимое значение в заданной области.");
                }

                vertices.push_back({ x, y, z });

                minZ = min(minZ, z);
                maxZ = max(maxZ, z);
            }
        }

        // Построение нижней поверхности
        const double baseZ = minZ - max(0.25, (maxZ - minZ) * 0.1);

        for (int k = 0; k < count; ++k)
        {
            vertices.push_back({ vertices[k].X, vertices[k].Y, baseZ });
        }

        vector<int> indices;
        indices.reserve(12 * n * n + 24 * n);

        auto addTriangle = [&](int a, int b, int c)
            {
                indices.insert(indices.end(), { a, b, c });
            };

        double volume = 0.0;
        const double triangleArea = (b.MaxX - b.MinX) * (b.MaxY - b.MinY) / (2 * n * n);

        auto addTopTriangle = [&](int a, int b, int c)
            {
                addTriangle(a, b, c);

                double averageZ = (vertices[a].Z + vertices[b].Z + vertices[c].Z) / 3.0;
                volume += triangleArea * (averageZ - baseZ);

                addTriangle(a + count, c + count, b + count);
            };

        // Триангуляция верхней и нижней поверхностей
        for (int j = 0; j < n; ++j)
        {
            for (int i = 0; i < n; ++i)
            {
                int a = j * row + i;
                int b = a + 1;
                int c = a + row;

                addTopTriangle(a, b, c + 1);
                addTopTriangle(a, c + 1, c);
            }
        }

        // Формирование периметра
        vector<int> perimeter;

        for (int i = 0; i <= n; ++i)
        {
            perimeter.push_back(i);
        }

        for (int j = 1; j <= n; ++j)
        {
            perimeter.push_back(j * row + n);
        }

        for (int i = n - 1; i >= 0; --i)
        {
            perimeter.push_back(n * row + i);
        }

        for (int j = n - 1; j >= 1; --j)
        {
            perimeter.push_back(j * row);
        }

        // Построение боковых стенок
        vector<Geometry::Point3D> upper;
        vector<Geometry::Point3D> lower;

        for (size_t k = 0; k < perimeter.size(); ++k)
        {
            int a = perimeter[k];
            int b = perimeter[(k + 1) % perimeter.size()];

            addTriangle(a, a + count, b + count);
            addTriangle(a, b + count, b);

            upper.push_back(vertices[a]);
            lower.push_back(vertices[a + count]);
        }

        upper.push_back(upper.front());
        lower.push_back(lower.front());

        // Формирование граничных линий
        vector<vector<Geometry::Point3D>> edges{ upper, lower };

        for (int corner : {0, n, n* row, n* row + n})
        {
            edges.push_back({ vertices[corner], vertices[corner + count] });
        }

        return { Geometry::TriangleMesh(move(vertices), move(indices)), volume, baseZ, move(edges) };
    }
}
