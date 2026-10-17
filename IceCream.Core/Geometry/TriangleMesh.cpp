#include "TriangleMesh.h"

namespace IceCream::Core::Geometry
{
	TriangleMesh::TriangleMesh(std::vector<Point3D> vertices, std::vector<int> indices)
	{
		if (indices.size() % 3 != 0)
		{
			throw std::invalid_argument("Количество индексов должно быть кратно трём.");
		}

		for (const auto& vertex : vertices)
		{
			if (!std::isfinite(vertex.X) || !std::isfinite(vertex.Y) || !std::isfinite(vertex.Z))
			{
				throw std::invalid_argument("Координаты вершин должны быть конечными.");
			}
		}

		for (int index : indices)
		{
			if (index < 0 || static_cast<size_t>(index) >= vertices.size())
			{
				throw std::out_of_range("Индекс выходит за границы массива вершин.");
			}
		}

		vertices_ = std::move(vertices);
		indices_ = std::move(indices);
	}

	std::tuple<Point3D, Point3D, Point3D> TriangleMesh::GetTriangle(size_t triangleIndex) const
	{
		if (triangleIndex >= TriangleCount())
		{
			throw std::out_of_range("triangleIndex");
		}

		size_t offset = triangleIndex * 3;

		return {Vertices()[Indices()[offset]], Vertices()[Indices()[offset + 1]], Vertices()[Indices()[offset + 2]]};
	}
}
