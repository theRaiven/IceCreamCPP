#pragma once

#include "../Geometry/TriangleMesh.h"
#include "../Profiles/IceCreamProfile.h"
#include <limits>

namespace IceCream::Core::Services
{
	using Geometry::TriangleMesh;
	using Geometry::Point3D;
	using Profiles::IceCreamProfile;
	using Profiles::IRadialProfile;

	class SurfaceMeshBuilder
	{
	public:
		/// <summary>
		/// Строит треугольную сетку для мороженого на основе профиля.
		/// </summary>
		/// <param name="profile"> Профиль мороженого </param>
		/// <param name="baseSegments"> Количество сегментов основания </param>
		/// <param name="capSegments"> Количество сегментов крышки </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <returns> Треугольная сетка </returns>
		TriangleMesh Build(const IceCreamProfile& profile, int baseSegments = 40, int capSegments = 40, int radialSegments = 64) const;

		TriangleMesh Build(const IRadialProfile& profile, int heightSegments = 160, int radialSegments = 96) const;

	private:
		/// <summary>
		/// Строит основание мороженого на основе профиля.
		/// </summary>
		/// <param name="profile"> Профиль </param>
		/// <param name="heightSegments"> Количество сегментов по высоте </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <param name="vertices"> Список вершин </param>
		/// <param name="indices"> Список индексов </param>
		/// <returns> Массив индексов для первого кольца </returns>
		static std::vector<int> BuildBase(const IRadialProfile& profile, int heightSegments, int radialSegments, std::vector<Point3D>& vertices, std::vector<int>& indices);

		/// <summary>
		/// Строит крышку мороженого на основе профиля.
		/// </summary>
		/// <param name="profile"> Профиль </param>
		/// <param name="heightSegments"> Количество сегментов по высоте </param>
		/// <param name="radialSegments"> Количество радиальных сегментов </param>
		/// <param name="vertices"> Список вершин </param>
		/// <param name="indices"> Список индексов </param>
		/// <param name="joinRing"> Массив индексов для кольца, которое соединяется с крышкой </param>
		static void BuildCap(const IRadialProfile& profile, int heightSegments, int radialSegments, std::vector<Point3D>& vertices, std::vector<int>& indices, const std::vector<int>& joinRing);

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
