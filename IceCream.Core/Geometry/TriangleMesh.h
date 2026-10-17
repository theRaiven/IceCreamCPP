#pragma once

#include "Point3D.h"
#include <vector>
#include <tuple>
#include <utility>

namespace IceCream::Core::Geometry
{
	/// <summary>
	/// Полученная после триангуляции сетка
	/// </summary>
	class TriangleMesh
	{
	private:
		std::vector<Point3D> vertices_;
		std::vector<int> indices_;

	public:
		const std::vector<Point3D>& Vertices() const
		{
			return vertices_;
		}

		const std::vector<int>& Indices() const
		{
			return indices_;
		}

		size_t TriangleCount() const
		{
			return Indices().size() / 3;
		}

		/// <summary>
		/// Инициализирует новый экземпляр класса TriangleMesh с заданными вершинами и индексами.
		/// </summary>
		/// <param name="vertices">Список вершин</param>
		/// <param name="indices">Список индексов</param>
		TriangleMesh(std::vector<Point3D> vertices, std::vector<int> indices);

		/// <summary>
		/// Возвращает треугольник по его индексу в списке треугольников.
		/// </summary>
		/// <param name="triangleIndex">Индекс треугольника</param>
		/// <returns>Возвращает треугольник по его индексу в списке треугольников</returns>
		std::tuple<Point3D, Point3D, Point3D> GetTriangle(size_t triangleIndex) const;
	};
}
