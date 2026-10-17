#include "../IceCream.App/ViewModels/MainViewModel.h"
#include <iostream>
#include <limits>
#include <map>
#include <cmath>

using namespace icecream;
namespace
{
int checks = 0;
void check(bool condition, const char* message)
{
	++checks;
	if (!condition) throw std::runtime_error(message);
}
void near(double value, double expected, double tolerance, const char* message)
{
	check(std::isfinite(value) && std::abs(value-expected) <= tolerance, message);
}
template<class Function>
void rejects(Function action)
{
	bool failed = false;
	try { action(); } catch (const std::exception&) { failed = true; }
	check(failed, "Invalid input was accepted");
}
SectionContour square(double low, double high)
{
	std::vector<Point2D> points{{low, low}, {high, low}, {high, high}, {low, high}};
	std::vector<Point3D> points3d;
	for (Point2D point : points) points3d.push_back({point.x, point.y, 0});
	return {points3d, points, true};
}
}
int main()
{
	try
	{
		const double pi = std::acos(-1.0);
		Plane3D plane({2, 3, 4}, {1, 2, 3});
		Point2D local{0.75, -2};
		Point3D global = plane.to_global(local);
		near(plane.signed_distance_to(global), 0, 1e-12, "Plane coordinate roundtrip");
		near(plane.to_local(global).distance_to(local), 0, 1e-12, "Local coordinate roundtrip");
		near(plane.signed_distance_to(plane.origin()+plane.normal()*2), 2, 1e-12, "Signed distance direction");
		near(plane.signed_distance_to(plane.project({8, 9, 10})), 0, 1e-12, "Projection");
		PowerProfile cone(3, 1, 1), paraboloid(3, 1, 0.5);
		VolumeCalculator volume;
		near(volume.calculate(cone, 1000), pi, 1e-10, "Cone analytic volume");
		near(volume.calculate(paraboloid, 1000), 1.5*pi, 1e-10, "Paraboloid analytic volume");
		for (double cap_height : {0.25, 1.0, 2.0})
		{
			SphericalCapProfile cap(3, 1, cap_height);
			near(cap.get_radius(3), 1, 1e-12, "Cap join radius");
			near(cap.get_radius(cap.max_z()), 0, 1e-12, "Cap pole");
			double expected = pi*cap_height*(3+cap_height*cap_height)/6;
			near(volume.calculate(cap, 1000), expected, 1e-10, "Cap analytic volume");
		}
		SurfaceMeshBuilder builder;
		IceCreamBody body({3, 1, 1, 1}, builder);
		const auto& mesh = body.mesh();
		check(mesh.vertices().size() == 5058, "Mesh vertex count");
		check(mesh.triangle_count() == 10112, "Mesh triangle count");
		// Каждый край замкнутой сетки встречается дважды с противоположным направлением.
		std::map<std::pair<int, int>, std::pair<int, int>> edges;
		for (size_t i = 0; i < mesh.indices().size(); i += 3)
		{
			auto tri = mesh.get_triangle(i/3);
			check((tri[1]-tri[0]).cross(tri[2]-tri[0]).length() > 0, "Degenerate triangle");
			for (int j = 0; j < 3; ++j)
			{
				int a = mesh.indices()[i+j], b = mesh.indices()[i+(j+1)%3];
				auto key = std::minmax(a, b);
				auto& entry = edges[{key.first, key.second}];
				++entry.first;
				entry.second += a < b ? 1 : -1;
			}
		}
		for (const auto& entry : edges)
			check(entry.second.first == 2 && entry.second.second == 0, "Mesh is not consistently closed");
		near(volume.calculate(body.profile()), 5*pi/3, 1e-10, "Whole analytic volume");
		const IRadialProfile& abstract_profile = body.profile();
		near(volume.calculate(abstract_profile, 1000), 5*pi/3, 1e-10, "Composite volume via interface");
		MeshSectionCalculator sections;
		SectionAreaCalculator areas;
		for (double z : {0.123, 1.5, 3.0, 3.5})
		{
			auto section = sections.calculate(mesh, Plane3D::at_z(z));
			check(section.contours().size() == 1 && section.contours()[0].is_closed(), "Horizontal section closure");
			double radius = body.profile().get_radius(z);
			// Горизонтальная сеточная окружность — правильный 64-угольник.
			near(areas.calculate(section), 32*radius*radius*std::sin(2*pi/64), 1e-10, "Polygon section area");
		}
		for (double z : {-1.0, 5.0}) check(sections.calculate(mesh, Plane3D::at_z(z)).is_empty(), "Outside section");
		for (double z : {0.0, 4.0})
		{
			auto section = sections.calculate(mesh, Plane3D::at_z(z));
			check(section.contours().size() == 1 && section.contours()[0].points_3d().size() == 1, "Pole tangency");
			near(areas.calculate(section), 0, 0, "Point area");
		}
		for (Plane3D cut : {Plane3D::at_x(0), Plane3D::at_y(0), Plane3D({0, 0, 2}, {1, 2, 3})})
		{
			auto section = sections.calculate(mesh, cut);
			check(section.contours().size() == 1 && section.contours()[0].is_closed(), "Oblique or axial closure");
			check(areas.calculate(section) > 0, "Oblique area");
			for (Point3D point : section.contours()[0].points_3d())
				near(cut.signed_distance_to(point), 0, 1e-10, "Section point off plane");
		}
		TriangleMesh flat({{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}}, {0,1,2, 0,2,3});
		auto flat_section = sections.calculate(flat, Plane3D::at_z(0));
		check(flat_section.contours().size() == 1, "Coplanar contour count");
		check(flat_section.contours()[0].points_3d().size() == 4, "Coplanar internal diagonal removed");
		near(areas.calculate(flat_section), 1, 1e-12, "Coplanar area");
		near(areas.calculate(SectionResult(Plane3D::at_z(0), {square(0,4), square(1,3)})), 12, 1e-12, "Hole area");
		near(areas.calculate(SectionResult(Plane3D::at_z(0), {square(0,6), square(1,5), square(2,4)})), 24, 1e-12, "Nested island area");
		near(areas.calculate(SectionResult(Plane3D::at_z(0), {square(0,1), square(2,3)})), 2, 1e-12, "Disjoint contour area");
		SectionContour chain({{0,0,0},{1,0,0},{1,1,0}}, {{0,0},{1,0},{1,1}}, false);
		rejects([&] { areas.calculate(SectionResult(Plane3D::at_z(0), {chain})); });
		auto copy = body;
		near(copy.profile().get_radius(1), 1.0/3, 1e-12, "Profile ownership on copy");
		MainViewModel vm;
		int notifications = 0;
		vm.set_on_changed([&] { ++notifications; });
		const Point3D* old_vertices = vm.body().mesh().vertices().data();
		vm.set_plane(Plane3D::at_z(3));
		check(old_vertices == vm.body().mesh().vertices().data(), "Plane change rebuilt mesh");
		check(notifications == 1, "View notification");
		double old_area = vm.section_area();
		rejects([&] { vm.set_parameters({-1, 1, 1, 1}); });
		rejects([&] { vm.set_segments(1, 1, 2); });
		near(vm.section_area(), old_area, 0, "Failed update changed state");
		check(notifications == 1, "Failed update notified view");
		vm.set_parameters({4, 2, 0.5, 1});
		near(vm.volume(), 8*pi+13*pi/6, 1e-9, "ViewModel parameter update");
		vm.set_segments(10, 10, 32);
		check(vm.radial_segments() == 32 && notifications == 3, "Mesh settings update");
		vm.set_on_changed({});
		vm.set_plane(Plane3D::at_z(100));
		check(vm.section().is_empty() && vm.section_area() == 0, "ViewModel empty section");
		rejects([] { Vector3D{}.normalized(); });
		rejects([] { Plane3D({0,0,0}, {0,0,0}); });
		rejects([] { PowerProfile(1, -1, 1); });
		rejects([] { SphericalCapProfile(0, 1, 0); });
		rejects([] { IceCreamProfile(nullptr, 1); });
		rejects([] { TriangleMesh({{0,0,0}}, {0,1,0}); });
		rejects([&] { volume.calculate(cone, 3); });
		rejects([&] { sections.calculate(mesh, plane, 0); });
		rejects([&] { cone.get_radius(std::numeric_limits<double>::quiet_NaN()); });
		rejects([&] { cone.get_radius(4); });
		rejects([] { IceCreamProfile(std::make_shared<PowerProfile>(3,1,1), SphericalCapProfile(4,1,1)); });
		rejects([] { IceCreamProfile(std::make_shared<PowerProfile>(3,1,1), SphericalCapProfile(3,2,1)); });
		std::cout << "Passed " << checks << " checks.\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "FAILED: " << error.what() << '\n';
		return 1;
	}
}
