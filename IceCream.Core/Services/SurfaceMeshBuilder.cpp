#include "SurfaceMeshBuilder.h"

namespace IceCream::Core::Services
{
    TriangleMesh SurfaceMeshBuilder::Build(const IRadialProfile& profile, int heightSegments, int radialSegments) const
    {
        if(heightSegments < 2 || radialSegments < 3) throw std::invalid_argument("Invalid mesh resolution");
        std::vector<Point3D> vertices;
        std::vector<int> indices;
        auto first=CreateRing(profile,profile.MinZ(),radialSegments,vertices);
        if(first.size()>1) {
            int center=static_cast<int>(vertices.size()); vertices.push_back({0,0,profile.MinZ()});
            ConnectRing({center},first,indices);
        }
        auto last=BuildPart(profile,heightSegments,radialSegments,first,vertices,indices);
        if(last.size()>1) {
            int center=static_cast<int>(vertices.size()); vertices.push_back({0,0,profile.MaxZ()});
            ConnectRing(last,{center},indices);
        }
        return TriangleMesh(std::move(vertices),std::move(indices));
    }

	TriangleMesh SurfaceMeshBuilder::Build(const IceCreamProfile& profile, int baseSegments, int capSegments, int radialSegments) const
	{
		if (baseSegments < 1)
		{
			throw std::out_of_range("baseSegments");
		}

		if (capSegments < 1)
		{
			throw std::out_of_range("capSegments");
		}

		if (radialSegments < 3)
		{
			throw std::out_of_range("radialSegments");
		}

		std::vector<Point3D> vertices;
		std::vector<int> indices;

		std::vector<int> joinRing = BuildBase(profile.BaseProfile(), baseSegments, radialSegments, vertices, indices);
		BuildCap(profile.CapProfile(), capSegments, radialSegments, vertices, indices, joinRing);

		return TriangleMesh(std::move(vertices), std::move(indices));
	}

	std::vector<int> SurfaceMeshBuilder::BuildBase(const IRadialProfile& profile, int heightSegments, int radialSegments, std::vector<Point3D>& vertices, std::vector<int>& indices)
	{
		std::vector<int> firstRing = CreateRing(profile, profile.MinZ(), radialSegments, vertices);

		return BuildPart(profile, heightSegments, radialSegments, firstRing, vertices, indices);
	}

	void SurfaceMeshBuilder::BuildCap(const IRadialProfile& profile, int heightSegments, int radialSegments, std::vector<Point3D>& vertices, std::vector<int>& indices, const std::vector<int>& joinRing)
	{
		// Начальное кольцо уже создано при построении рожка
		BuildPart(profile, heightSegments, radialSegments, joinRing, vertices, indices);
	}

	std::vector<int> SurfaceMeshBuilder::BuildPart(const IRadialProfile& profile, int heightSegments, int radialSegments, const std::vector<int>& firstRing, std::vector<Point3D>& vertices, std::vector<int>& indices)
	{
		std::vector<int> previousRing = firstRing;

		for (int i = 1; i <= heightSegments; i++)
		{
			double t = static_cast<double>(i) / heightSegments;
			double z;

			if (i == heightSegments)
			{
				z = profile.MaxZ();
			}
			else
			{
				z = profile.MinZ() + t * (profile.MaxZ() - profile.MinZ());
			}

			std::vector<int> currentRing = CreateRing(profile, z, radialSegments, vertices);
			ConnectRing(previousRing, currentRing, indices);

			previousRing = std::move(currentRing);
		}

		return previousRing;
	}

	std::vector<int> SurfaceMeshBuilder::CreateRing(const IRadialProfile& profile, double z, int radialSegments, std::vector<Point3D>& vertices)
	{
		double radius = profile.GetRadius(z);

		if (!std::isfinite(radius) || radius < 0)
		{
			throw std::runtime_error("Радиус должен быть конечным неотрицательным числом.");
		}

		// Размер vector — size_t, но индексы сетки, как в C#, имеют тип int.
		if (vertices.size() > static_cast<size_t>(std::numeric_limits<int>::max() - radialSegments))
		{
			throw std::length_error("Количество вершин не помещается в int.");
		}

		if (radius == 0)
		{
			// Если радиус равен нулю, создаем одну вершину в центре
			int centerIndex = static_cast<int>(vertices.size());
			vertices.push_back({0, 0, z});

			return {centerIndex};
		}

		std::vector<int> ringIndices(radialSegments);
		double angleStep = 2 * std::acos(-1.0) / radialSegments;

		for (int i = 0; i < radialSegments; i++)
		{
			double angle = i * angleStep;

			double x = radius * std::cos(angle);
			double y = radius * std::sin(angle);

			ringIndices[i] = static_cast<int>(vertices.size());
			vertices.push_back({x, y, z});
		}

		return ringIndices;
	}

	void SurfaceMeshBuilder::ConnectRing(const std::vector<int>& lower, const std::vector<int>& upper, std::vector<int>& indices)
	{
		if (lower.empty() || upper.empty())
		{
			throw std::invalid_argument("Кольца не должны быть пустыми.");
		}

		if (lower.size() == 1 && upper.size() == 1)
		{
			throw std::runtime_error("Нельзя построить поверхность между двумя полюсами.");
		}

		if (lower.size() == 1)
		{
			// Нижний полюс
			int polus = lower[0];

			for (size_t i = 0; i < upper.size(); i++)
			{
				size_t next = (i + 1) % upper.size();

				AddTriangle(indices, polus, upper[next], upper[i]);
			}

			return;
		}

		if (upper.size() == 1)
		{
			// Верхний полюс
			int polus = upper[0];

			for (size_t i = 0; i < lower.size(); i++)
			{
				size_t next = (i + 1) % lower.size();

				AddTriangle(indices, lower[i], lower[next], polus);
			}

			return;
		}

		if (lower.size() != upper.size())
		{
			throw std::invalid_argument("Обычные кольца должны содержать одинаковое количество вершин.");
		}

		// Два обычных кольца
		for (size_t i = 0; i < lower.size(); i++)
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

	void SurfaceMeshBuilder::AddTriangle(std::vector<int>& indices, int a, int b, int c)
	{
		indices.push_back(a);
		indices.push_back(b);
		indices.push_back(c);
	}
}
