#include "VolumeCalculator.h"
#include <cmath>
#include <stdexcept>
#include <map>
#include <tuple>
#include <limits>
#include <algorithm>

namespace IceCream::Core::Services
{
	double VolumeCalculator::Calculate(const IRadialProfile& profile, int intervals) const
	{
		if (intervals < 2 || intervals % 2 != 0)
		{
			throw std::out_of_range("Количество промежутков должно быть чётным и не меньше двух.");
		}

		double minZ = profile.MinZ();
		double maxZ = profile.MaxZ();
		double height = maxZ - minZ;

		if (!std::isfinite(minZ) || !std::isfinite(maxZ) || !std::isfinite(height) || height <= 0)
		{
			throw std::invalid_argument("Профиль должен иметь конечный положительный диапазон высот.");
		}

		double step = height / intervals;

		double sum = GetSectionArea(profile, minZ) + GetSectionArea(profile, maxZ);

		for (int i = 1; i < intervals; i++)
		{
			double z = minZ + height * (static_cast<double>(i) / intervals);

			int weight = i % 2 == 0 ? 2 : 4;

			sum += weight * GetSectionArea(profile, z);
		}

		double volume = step / 3 * sum;

		if (!std::isfinite(volume))
		{
			throw std::runtime_error("При вычислении объёма произошло переполнение.");
		}

		return volume;
	}

	double VolumeCalculator::Calculate(const Geometry::TriangleMesh& mesh) const
	{
		auto result = TryCalculate(mesh);
		if (!result) throw std::invalid_argument("Для объёма нужна замкнутая согласованно ориентированная сетка.");
		return *result;
	}

	std::optional<double> VolumeCalculator::TryCalculate(const Geometry::TriangleMesh& mesh) const
	{
		if (mesh.TriangleCount() == 0) return std::nullopt;
		// Coincident vertices can have different indices at material boundaries.
		std::map<std::tuple<double, double, double>, int> vertices;
		std::vector<int> canonical;
		canonical.reserve(mesh.Vertices().size());
		for (const auto& p : mesh.Vertices())
		{
			auto [it, inserted] = vertices.emplace(std::make_tuple(p.X, p.Y, p.Z), static_cast<int>(vertices.size()));
			canonical.push_back(it->second);
		}
		struct Edge { int Count = 0, Orientation = 0; };
		std::map<std::pair<int, int>, Edge> edges;
		const auto& indices = mesh.Indices();
		for (size_t i = 0; i < indices.size(); i += 3)
		{
			auto [a, b, c] = mesh.GetTriangle(i / 3);
			if ((b - a).Cross(c - a).Length() == 0) return std::nullopt;
			for (size_t k = 0; k < 3; ++k)
			{
				int aIndex = canonical[indices[i + k]], bIndex = canonical[indices[i + (k + 1) % 3]];
				if (aIndex == bIndex) return std::nullopt;
				auto& edge = edges[std::minmax(aIndex, bIndex)];
				++edge.Count;
				edge.Orientation += aIndex < bIndex ? 1 : -1;
			}
		}
		for (const auto& [key, edge] : edges)
			if (edge.Count != 2 || edge.Orientation != 0) return std::nullopt;

		// Translate near the origin and accumulate in extended precision.
		const auto origin = mesh.Vertices()[indices.front()] - Geometry::Point3D::Origin();
		long double volume = 0.0;

		for (size_t i = 0; i < mesh.TriangleCount(); ++i)
		{
			auto [a, b, c] = mesh.GetTriangle(i);

			a = a - origin;
			b = b - origin;
			c = c - origin;
			volume += static_cast<long double>(a.X) * (static_cast<long double>(b.Y) * c.Z - static_cast<long double>(b.Z) * c.Y);
			volume += static_cast<long double>(a.Y) * (static_cast<long double>(b.Z) * c.X - static_cast<long double>(b.X) * c.Z);
			volume += static_cast<long double>(a.Z) * (static_cast<long double>(b.X) * c.Y - static_cast<long double>(b.Y) * c.X);
		}

		double result = static_cast<double>(std::abs(volume / 6.0L));
		return std::isfinite(result) ? std::optional<double>(result) : std::nullopt;
	}

	double VolumeCalculator::GetSectionArea(const IRadialProfile& profile, double z)
	{
		double radius = profile.GetRadius(z);

		if (!std::isfinite(radius) || radius < 0)
		{
			throw std::runtime_error("Профиль должен возвращать конечный неотрицательный радиус.");
		}

		return std::acos(-1.0) * radius * radius;
	}
}
