#pragma once

#include "../Geometry/TriangleMesh.h"
#include "../Models/SectionResult.h"
#include <set>
#include <map>
#include <algorithm>
#include <limits>

namespace IceCream::Core::Services
{
	using Geometry::Point3D;
	using Geometry::Point2D;
	using Geometry::Plane3D;
	using Geometry::TriangleMesh;
	using Models::SectionResult;
	using Models::SectionContour;

	class MeshSectionCalculator
	{
	private:
		// пересечение одного треугольника
		struct TriangleSection
		{
			std::vector<Point3D> Points;
			bool IsCoplanar;
		};

		// все точки сечения и связи между ними
		struct SectionGraph
		{
			std::vector<Point3D> Points;
			std::set<std::pair<int, int>> Edges;
			std::set<int> TouchPoints;
		};

	public:
		/// <summary>
		/// Вычисляет сечение треугольной сетки плоскостью.
		/// </summary>
		SectionResult Calculate(const TriangleMesh& mesh, const Plane3D& plane, double ESP = 1e-8) const;

	private:
		static TriangleSection IntersectTriangle(Point3D a, Point3D b, Point3D c, const Plane3D& plane, double ESP);

		static int Classify(double distance, double ESP);

		static void AddUniquePoint(std::vector<Point3D>& points, Point3D candidate, double ESP);

		static SectionGraph CollectIntersections(const TriangleMesh& mesh, const Plane3D& plane, double ESP);

		static int GetPointIndex(std::vector<Point3D>& points, Point3D candidate, double ESP);

		static std::pair<int, int> MakeEdge(int a, int b);

		static std::vector<SectionContour> BuildContours(const SectionGraph& graph, const Plane3D& plane);

		static SectionContour TraceContour(int start, int next, const std::vector<Point3D>& points, const std::vector<std::vector<int>>& neighbors, std::set<std::pair<int, int>>& remainingEdges, const Plane3D& plane);
	};
}
