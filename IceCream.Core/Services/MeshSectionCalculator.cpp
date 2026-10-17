#include "MeshSectionCalculator.h"

namespace IceCream::Core::Services
{
	SectionResult MeshSectionCalculator::Calculate(const TriangleMesh& mesh, const Plane3D& plane, double ESP) const
	{
		if (!std::isfinite(ESP) || ESP <= 0)
		{
			throw std::out_of_range("Допуск должен быть конечным положительным числом.");
		}

		SectionGraph graph = CollectIntersections(mesh, plane, ESP);
		std::vector<SectionContour> contours = BuildContours(graph, plane);

		return SectionResult(plane, std::move(contours));
	}

	MeshSectionCalculator::TriangleSection MeshSectionCalculator::IntersectTriangle(Point3D a, Point3D b, Point3D c, const Plane3D& plane, double ESP)
	{
		Point3D vertices[] = {a, b, c};

		double distances[] = {plane.SignedDistanceTo(a), plane.SignedDistanceTo(b), plane.SignedDistanceTo(c)};
		int sides[] = {Classify(distances[0], ESP), Classify(distances[1], ESP), Classify(distances[2], ESP)};

		// Все вершины лежат на плоскости с учётом допуска
		if (sides[0] == 0 && sides[1] == 0 && sides[2] == 0)
		{
			return {{plane.Project(a), plane.Project(b), plane.Project(c)}, true};
		}

		std::vector<Point3D> points;

		// Сначала сохраняем вершины, попавшие на плоскость.
		for (int i = 0; i < 3; i++)
		{
			if (sides[i] == 0)
			{
				AddUniquePoint(points, plane.Project(vertices[i]), ESP);
			}
		}

		// Проверяем рёбра AB, BC и CA.
		for (int i = 0; i < 3; i++)
		{
			int next = (i + 1) % 3;

			// Концы должны находиться строго по разные стороны
			if (sides[i] * sides[next] != -1)
			{
				continue;
			}

			double t = distances[i] / (distances[i] - distances[next]);
			Point3D point = vertices[i] + (vertices[next] - vertices[i]) * t;

			AddUniquePoint(points, plane.Project(point), ESP);
		}

		return {std::move(points), false};
	}

	int MeshSectionCalculator::Classify(double distance, double ESP)
	{
		if (distance > ESP)
		{
			return 1;
		}

		if (distance < -ESP)
		{
			return -1;
		}

		return 0;
	}

	void MeshSectionCalculator::AddUniquePoint(std::vector<Point3D>& points, Point3D candidate, double ESP)
	{
		for (Point3D point : points)
		{
			if (point.DistanceTo(candidate) <= ESP)
			{
				return;
			}
		}

		points.push_back(candidate);
	}

	MeshSectionCalculator::SectionGraph MeshSectionCalculator::CollectIntersections(const TriangleMesh& mesh, const Plane3D& plane, double ESP)
	{
		SectionGraph graph;

		std::map<std::pair<int, int>, int> coplanarEdges;
		std::set<int> coplanarPoints;

		for (size_t i = 0; i < mesh.TriangleCount(); i++)
		{
			auto [a, b, c] = mesh.GetTriangle(i);
			TriangleSection section = IntersectTriangle(a, b, c, plane, ESP);

			if (section.Points.empty())
			{
				continue;
			}

			std::vector<int> pointIndices(section.Points.size());

			for (size_t j = 0; j < section.Points.size(); j++)
			{
				pointIndices[j] = GetPointIndex(graph.Points, section.Points[j], ESP);
			}

			if (section.IsCoplanar)
			{
				for (int index : pointIndices)
				{
					coplanarPoints.insert(index);
				}

				for (int j = 0; j < 3; j++)
				{
					int next = (j + 1) % 3;

					int first = pointIndices[j];
					int second = pointIndices[next];

					if (first == second)
					{
						continue;
					}

					auto edge = MakeEdge(first, second);
					int count = coplanarEdges[edge];
					coplanarEdges[edge] = count + 1;
				}

				continue;
			}

			if (pointIndices.size() == 1)
			{
				graph.TouchPoints.insert(pointIndices[0]);
			}
			else if (pointIndices.size() == 2)
			{
				int first = pointIndices[0];
				int second = pointIndices[1];

				if (first == second)
				{
					graph.TouchPoints.insert(first);
				}
				else
				{
					graph.Edges.insert(MakeEdge(first, second));
				}
			}
		}

		// Убираем внутренние рёбра плоских участков.
		for (const auto& pair : coplanarEdges)
		{
			if (pair.second == 1)
			{
				graph.Edges.insert(pair.first);
			}
			else
			{
				graph.Edges.erase(pair.first);
			}
		}

		for (int index : coplanarPoints)
		{
			graph.TouchPoints.erase(index);
		}

		return graph;
	}

	int MeshSectionCalculator::GetPointIndex(std::vector<Point3D>& points, Point3D candidate, double ESP)
	{
		for (size_t i = 0; i < points.size(); i++)
		{
			if (points[i].DistanceTo(candidate) <= ESP)
			{
				return static_cast<int>(i);
			}
		}

		if (points.size() >= static_cast<size_t>(std::numeric_limits<int>::max()))
		{
			throw std::length_error("Количество точек не помещается в int.");
		}

		points.push_back(candidate);

		return static_cast<int>(points.size() - 1);
	}

	std::pair<int, int> MeshSectionCalculator::MakeEdge(int a, int b)
	{
		return a < b ? std::pair<int, int>{a, b} : std::pair<int, int>{b, a};
	}

	std::vector<SectionContour> MeshSectionCalculator::BuildContours(const SectionGraph& graph, const Plane3D& plane)
	{
		std::vector<SectionContour> contours;
		std::vector<std::vector<int>> neighbors(graph.Points.size());

		for (const auto& edge : graph.Edges)
		{
			neighbors[edge.first].push_back(edge.second);
			neighbors[edge.second].push_back(edge.first);
		}

		// Одинаковый порядок обхода при одинаковых входных данных.
		for (std::vector<int>& list : neighbors)
		{
			std::sort(list.begin(), list.end());
		}

		std::set<std::pair<int, int>> remainingEdges = graph.Edges;

		// Сначала цепочки от концов и точек ветвления.
		for (size_t start = 0; start < neighbors.size(); start++)
		{
			if (neighbors[start].size() == 2)
			{
				continue;
			}

			for (int next : neighbors[start])
			{
				if (!remainingEdges.count(MakeEdge(static_cast<int>(start), next)))
				{
					continue;
				}

				contours.push_back(TraceContour(static_cast<int>(start), next, graph.Points, neighbors, remainingEdges, plane));
			}
		}

		// Остались обычные замкнутые циклы.
		for (size_t start = 0; start < neighbors.size(); start++)
		{
			for (int next : neighbors[start])
			{
				if (!remainingEdges.count(MakeEdge(static_cast<int>(start), next)))
				{
					continue;
				}

				contours.push_back(TraceContour(static_cast<int>(start), next, graph.Points, neighbors, remainingEdges, plane));
			}
		}

		// Изолированные точки касания.
		for (size_t i = 0; i < graph.Points.size(); i++)
		{
			if (!neighbors[i].empty() || !graph.TouchPoints.count(static_cast<int>(i)))
			{
				continue;
			}

			Point3D point = graph.Points[i];

			contours.push_back(SectionContour({point}, {plane.ToLocal(point)}, false));
		}

		return contours;
	}

	SectionContour MeshSectionCalculator::TraceContour(int start, int next, const std::vector<Point3D>& points, const std::vector<std::vector<int>>& neighbors, std::set<std::pair<int, int>>& remainingEdges, const Plane3D& plane)
	{
		std::vector<int> orderedIndices = {start};

		int previous = start;
		int current = next;

		bool isClosed = false;

		while (true)
		{
			remainingEdges.erase(MakeEdge(previous, current));

			if (current == start)
			{
				isClosed = true;
				break;
			}

			orderedIndices.push_back(current);

			// Конец цепочки или точка ветвления.
			if (neighbors[current].size() != 2)
			{
				break;
			}

			int following = neighbors[current][0] == previous ? neighbors[current][1] : neighbors[current][0];

			if (!remainingEdges.count(MakeEdge(current, following)))
			{
				break;
			}

			previous = current;
			current = following;
		}

		std::vector<Point3D> points3D(orderedIndices.size());
		std::vector<Point2D> points2D(orderedIndices.size());

		for (size_t i = 0; i < orderedIndices.size(); i++)
		{
			Point3D point = points[orderedIndices[i]];

			points3D[i] = point;
			points2D[i] = plane.ToLocal(point);
		}

		return SectionContour(std::move(points3D), std::move(points2D), isClosed);
	}
}
