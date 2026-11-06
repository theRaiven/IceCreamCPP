#pragma once

#include "../../Geometry/Meshes/TriangleMesh.h"
#include "../../Geometry/Primitives/Plane3D.h"
#include "../../Profiles/Basic/IRadialProfile.h"

#include <limits>

namespace IceCream::Core::Services
{
	using Geometry::TriangleMesh;
	using Geometry::Point3D;
	using Profiles::IRadialProfile;

	class SurfaceMeshBuilder
	{
	public:
		/// <summary>
		/// Строит треугольную сетку для тела, состоящего из n профилей.
		/// </summary>
		/// <param name="profile"> Составной профиль тела </param>
		/// <param name="firstSegments"> Количество сегментов первой части </param>
		/// <param name="secondSegments"> Количество сегментов второй части </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <returns> Треугольная сетка </returns>
		TriangleMesh Build(const IRadialProfile& profile, int heightSegments = 160, int radialSegments = 96) const;

	private:
		/// <summary>
		/// Строит часть мороженого на основе профиля.
		/// </summary>
		/// <param name="profile"> Профиль </param>
		/// <param name="heightSegments"> Количество сегментов по высоте </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <param name="firstRing"> Массив индексов для первого кольца </param>
		/// <param name="vertices"> Список вершин </param>
		/// <param name="indices"> Список индексов </param>
		/// <returns></returns>
		static std::vector<int> BuildPart(const IRadialProfile& profile, int heightSegments, int radialSegments, const std::vector<int>& firstRing, std::vector<Point3D>& vertices, std::vector<int>& indices);

		/// <summary>
		/// Создаёт вершины кольца на заданной высоте.
		/// При нулевом радиусе создаёт одну вершину на оси.
		/// </summary>
		/// <param name="profile"> Профиль </param>
		/// <param name="z"> Высота </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <param name="vertices"> Список вершин </param>
		/// <returns> Массив индексов для кольца </returns>
		static std::vector<int> CreateRing(const IRadialProfile& profile, double z, int radialSegments, std::vector<Point3D>& vertices);

		/// <summary>
		/// соединяет два соседних кольца.
		/// Есть три случая: нижний полюс, верхний полюс и два обычных кольца.
		/// </summary>
		/// <param name="lower"> Нижнее кольцо </param>
		/// <param name="upper"> Верхнее кольцо </param>
		/// <param name="indices"> Список индексов </param>
		static void ConnectRing(const std::vector<int>& lower, const std::vector<int>& upper, std::vector<int>& indices);

		static void AddTriangle(std::vector<int>& indices, int a, int b, int c);
	};
}
