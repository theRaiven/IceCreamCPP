#include "../IceCream.App/ViewModels/MainViewModel.h"
#include <iostream>
#include "../IceCream.App/Views/Graphics/OrbitCamera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <map>
#include <cmath>

#include "../IceCream.Core/Services/VolumeCalculator.h"
#include "../IceCream.Core/Services/MeshSectionCalculator.h"
#include "../IceCream.Core/Services/SectionAreaCalculator.h"
#include <algorithm>
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Models;
using namespace IceCream::Core::Profiles;
using namespace IceCream::Core::Services;
using namespace IceCream::App::ViewModels;
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
	for (Point2D point : points) points3d.push_back({point.X, point.Y, 0});
	return {points3d, points, true};
}
}
int main()
{
	try
	{
        IceCream::App::Views::Graphics::OrbitCamera camera;
        const glm::mat4 projection=glm::perspective(glm::radians(45.0f),1280.0f/720.0f,.1f,100.0f);
        auto screenBefore=projection*camera.ViewMatrix()*glm::vec4(0,0,0,1);
        camera.BeginPan(100,100);
        camera.Pan(140,70,720);
        auto screenAfter=projection*camera.ViewMatrix()*glm::vec4(0,0,0,1);
        near((screenAfter.x/screenAfter.w-screenBefore.x/screenBefore.w)*640,40,1e-3,"RMB pan follows horizontal mouse pixels");
        near((screenAfter.y/screenAfter.w-screenBefore.y/screenBefore.w)*360,30,1e-3,"RMB pan follows vertical mouse pixels");
        near(glm::length(camera.Position()-camera.Target()),10,1e-5,"Pan preserves orbit distance");
        auto panTarget=camera.Target();
        camera.EndPan(); camera.Pan(500,500,720);
        near(glm::length(camera.Target()-panTarget),0,0,"Releasing RMB ends pan");
        camera.BeginRotate(0,0); camera.Rotate(100,40); camera.EndRotate();
        near(glm::length(camera.Target()-panTarget),0,0,"Orbit retains panned target");
        camera.Zoom(1);
        near(glm::length(camera.Position()-camera.Target()),9,1e-5,"Zoom retains panned target");
        screenBefore=projection*camera.ViewMatrix()*glm::vec4(0,0,0,1);
        // Check displacement of the orbit target itself after a second drag at a rotated view.
        auto worldPoint=glm::vec4(camera.Target(),1);
        screenBefore=projection*camera.ViewMatrix()*worldPoint;
        camera.BeginPan(0,0); camera.Pan(30,20,720);
        screenAfter=projection*camera.ViewMatrix()*worldPoint;
        near((screenAfter.x/screenAfter.w-screenBefore.x/screenBefore.w)*640,30,1e-3,"Pan stays screen-aligned after rotation and zoom");
        near((screenAfter.y/screenAfter.w-screenBefore.y/screenBefore.w)*360,-20,1e-3,"Rotated pan vertical direction");
        camera.Reset(); camera.Pan(100,100,720);
        near(glm::length(camera.Target()),0,0,"Reset clears translation and active drag");
        near(camera.Distance(),10,0,"Reset restores camera distance");
		const double pi = std::acos(-1.0);
		Plane3D plane({2, 3, 4}, {1, 2, 3});
		Point2D local{0.75, -2};
		Point3D global = plane.ToGlobal(local);
		near(plane.SignedDistanceTo(global), 0, 1e-12, "Plane coordinate roundtrip");
		near(plane.ToLocal(global).DistanceTo(local), 0, 1e-12, "Local coordinate roundtrip");
		near(plane.SignedDistanceTo(plane.Origin()+plane.Normal()*2), 2, 1e-12, "Signed distance direction");
		near(plane.SignedDistanceTo(plane.Project({8, 9, 10})), 0, 1e-12, "Projection");
		PowerProfile cone(3, 1, 1), paraboloid(3, 1, 0.5);
		VolumeCalculator volume;
		near(volume.Calculate(cone, 1000), pi, 1e-10, "Cone analytic volume");
		near(volume.Calculate(paraboloid, 1000), 1.5*pi, 1e-10, "Paraboloid analytic volume");
		for (double cap_height : {0.25, 1.0, 2.0})
		{
			SphericalCapProfile cap(3, 1, cap_height);
			near(cap.GetRadius(3), 1, 1e-12, "Cap join radius");
			near(cap.GetRadius(cap.MaxZ()), 0, 1e-12, "Cap pole");
			double expected = pi*cap_height*(3+cap_height*cap_height)/6;
			near(volume.Calculate(cap, 1000), expected, 1e-10, "Cap analytic volume");
		}
		SurfaceMeshBuilder builder;
		IceCreamBody body({3, 1, 1, 1}, builder);
		const auto& mesh = body.Mesh();
		check(mesh.Vertices().size() == 5058, "Mesh vertex count");
		check(mesh.TriangleCount() == 10112, "Mesh triangle count");
		// Каждый край замкнутой сетки встречается дважды с противоположным направлением.
		std::map<std::pair<int, int>, std::pair<int, int>> edges;
		for (size_t i = 0; i < mesh.Indices().size(); i += 3)
		{
			auto [ta, tb, tc] = mesh.GetTriangle(i/3);
			check((tb-ta).Cross(tc-ta).Length() > 0, "Degenerate triangle");
			for (int j = 0; j < 3; ++j)
			{
				int a = mesh.Indices()[i+j], b = mesh.Indices()[i+(j+1)%3];
				auto key = std::minmax(a, b);
				auto& entry = edges[{key.first, key.second}];
				++entry.first;
				entry.second += a < b ? 1 : -1;
			}
		}
		for (const auto& entry : edges)
			check(entry.second.first == 2 && entry.second.second == 0, "Mesh is not consistently closed");
		near(volume.Calculate(body.Profile()), 5*pi/3, 1e-10, "Whole analytic volume");
		const IRadialProfile& abstract_profile = body.Profile();
		near(volume.Calculate(abstract_profile, 1000), 5*pi/3, 1e-10, "Composite volume via interface");
		MeshSectionCalculator sections;
		SectionAreaCalculator areas;
		for (double z : {0.123, 1.5, 3.0, 3.5})
		{
			auto section = sections.Calculate(mesh, Plane3D::AtZ(z));
			check(section.Contours().size() == 1 && section.Contours()[0].IsClosed(), "Horizontal section closure");
			double radius = body.Profile().GetRadius(z);
			// Горизонтальная сеточная окружность — правильный 64-угольник.
			near(areas.Calculate(section), 32*radius*radius*std::sin(2*pi/64), 1e-10, "Polygon section area");
		}
		for (double z : {-1.0, 5.0}) check(sections.Calculate(mesh, Plane3D::AtZ(z)).IsEmpty(), "Outside section");
		for (double z : {0.0, 4.0})
		{
			auto section = sections.Calculate(mesh, Plane3D::AtZ(z));
			check(section.Contours().size() == 1 && section.Contours()[0].Points3D().size() == 1, "Pole tangency");
			near(areas.Calculate(section), 0, 0, "Point area");
		}
		for (Plane3D cut : {Plane3D::AtX(0), Plane3D::AtY(0), Plane3D({0, 0, 2}, {1, 2, 3})})
		{
			auto section = sections.Calculate(mesh, cut);
			check(section.Contours().size() == 1 && section.Contours()[0].IsClosed(), "Oblique or axial closure");
			check(areas.Calculate(section) > 0, "Oblique area");
			for (Point3D point : section.Contours()[0].Points3D())
				near(cut.SignedDistanceTo(point), 0, 1e-10, "Section point off plane");
		}
		TriangleMesh flat({{0,0,0}, {1,0,0}, {1,1,0}, {0,1,0}}, {0,1,2, 0,2,3});
		auto flat_section = sections.Calculate(flat, Plane3D::AtZ(0));
		check(flat_section.Contours().size() == 1, "Coplanar contour count");
		check(flat_section.Contours()[0].Points3D().size() == 4, "Coplanar internal diagonal removed");
		near(areas.Calculate(flat_section), 1, 1e-12, "Coplanar area");
		near(areas.Calculate(SectionResult(Plane3D::AtZ(0), {square(0,4), square(1,3)})), 12, 1e-12, "Hole area");
		near(areas.Calculate(SectionResult(Plane3D::AtZ(0), {square(0,6), square(1,5), square(2,4)})), 24, 1e-12, "Nested island area");
		near(areas.Calculate(SectionResult(Plane3D::AtZ(0), {square(0,1), square(2,3)})), 2, 1e-12, "Disjoint contour area");
		SectionContour chain({{0,0,0},{1,0,0},{1,1,0}}, {{0,0},{1,0},{1,1}}, false);
		rejects([&] { areas.Calculate(SectionResult(Plane3D::AtZ(0), {chain})); });
		auto copy = body;
		near(copy.Profile().GetRadius(1), 1.0/3, 1e-12, "Profile ownership on copy");
        MainViewModel vm;
        const Point3D* oldVertices = vm.Body().Mesh().Vertices().data();
        vm.SetSectionPosition(0, 0.2);
        vm.SetSectionPosition(1, -0.3);
        vm.SetSectionPosition(2, 1.5);
        for (int axis = 0; axis < 3; ++axis)
        {
            vm.SetSectionPreset(axis);
            const auto n = vm.SectionPlane().Normal();
            near(axis == 0 ? n.X : axis == 1 ? n.Y : n.Z, 1, 1e-12, "Axis preset normal");
            check(!vm.Section().IsEmpty() && vm.SectionArea() > 0, "Axis cut");
        }
        vm.SetSectionRotation(0, 30);
        vm.SetSectionRotation(1, 45);
        vm.SetSectionRotation(2, 60);
        const auto n = vm.SectionPlane().Normal();
        near(n.X, std::sqrt(6.0)/8 + std::sqrt(3.0)/4, 1e-12, "Combined rotation X");
        near(n.Y, 3*std::sqrt(2.0)/8 - 0.25, 1e-12, "Combined rotation Y");
        near(n.Z, std::sqrt(6.0)/4, 1e-12, "Combined rotation Z");
        near(vm.SectionPlane().Origin().X, 0.2, 0, "Anchor X");
        near(vm.SectionPlane().Origin().Y, -0.3, 0, "Anchor Y");
        near(vm.SectionPlane().Origin().Z, 1.5, 0, "Anchor Z");
        check(!vm.Section().IsEmpty(), "Rotated section");
        for (const auto& contour : vm.Section().Contours())
            for (auto point : contour.Points3D())
                near(vm.SectionPlane().SignedDistanceTo(point), 0, 1e-10, "Rotated point off plane");
        check(oldVertices == vm.Body().Mesh().Vertices().data(), "Plane change rebuilt mesh");
        rejects([&] { vm.SetSectionPosition(3, 0); });
        rejects([&] { vm.SetSectionPosition(0, 21); });
        rejects([&] { vm.SetSectionRotation(0, std::numeric_limits<double>::quiet_NaN()); });
        rejects([&] { vm.SetBaseHeight(std::numeric_limits<double>::quiet_NaN()); });
        near(vm.SectionPosition().X, 0.2, 0, "Invalid input mutated anchor");
        vm.SetSectionPreset(2);
        vm.SetSectionPosition(2, 20);
        check(vm.Section().IsEmpty() && vm.SectionArea() == 0, "Outside plane");
        vm.ResetDefaults();
        near(vm.SectionPosition().Z, 2.1, 0, "Reset anchor");
        near(vm.SectionRotation().Y, 0, 0, "Reset rotation");
        check(vm.SectionArea() > 0, "Reset section");
        vm.SetBaseHeight(4);
        near(vm.Parameters().BaseHeight, 4, 0, "Text parameter path");
        near(vm.SectionPosition().Z, 2.1, 0, "Shape update moved anchor");
        const IceCreamParameters formulaParameters{3,1,0.5,1.2};
        FormulaProfile defaultFormula(FormulaProfile::DefaultFormula(), formulaParameters);
        IceCreamBody original(formulaParameters,builder);
        for(int i=0;i<=100;++i) {
            double z=4.2*i/100;
            near(defaultFormula.GetRadius(z),original.Profile().GetRadius(z),1e-7,"Default formula equivalence");
        }
        FormulaProfile cylinder("R",formulaParameters);
        FormulaProfile formulaCone("R*z/L",formulaParameters);
        near(volume.Calculate(cylinder),4.2*pi,1e-10,"Formula cylinder volume");
        near(volume.Calculate(formulaCone),1.4*pi,1e-10,"Formula cone volume");
        FormulaProfile namedCone("radius*z/height",formulaParameters);
        near(volume.Calculate(namedCone),1.4*pi,1e-10,"Human-readable formula variables");
        FormulaProfile sphere("sqrt(max(0,radius^2-(z-radius)^2))",{1.8,1,.5,.2});
        near(volume.Calculate(sphere),4*pi/3,1e-10,"Ready sphere formula");
        FormulaProfile precedence("2^3^2 / 64 - -1",formulaParameters);
        near(precedence.GetRadius(1),9,1e-12,"Right associative powers and unary minus");
        FormulaProfile negativePower("2^-2",formulaParameters);
        near(negativePower.GetRadius(1),.25,1e-12,"Negative exponent");
        FormulaProfile conditional("if(z<H, sqrt(H-z)+1, R)",formulaParameters);
        near(conditional.GetRadius(4),1,0,"Conditional evaluates only selected branch");
        FormulaProfile functions("abs(-1)+min(2,3)+max(1,2)+cos(0)+sin(0)+log(exp(1))",formulaParameters);
        near(functions.GetRadius(1),7,1e-12,"Formula functions");
        for(const char* invalid : {"", "z+", "sqrt(-1)", "1/0", "-1", "unknown(z)", "sin(z,1)", "2**z", "z);system(1)", "0", "100", "if(1,2)", "z^2-1"})
            rejects([&] { FormulaProfile profile(invalid,formulaParameters); });
        rejects([&] { FormulaProfile profile(std::string(513,'1'),formulaParameters); });
        rejects([&] { FormulaProfile profile(std::string(70,'(')+"R"+std::string(70,')'),formulaParameters); });
        auto cappedCylinder=builder.Build(cylinder,20,48);
        auto middle=sections.Calculate(cappedCylinder,Plane3D::AtZ(2));
        near(areas.Calculate(middle),24*std::sin(2*pi/48),1e-10,"Formula cylinder cut");
        auto bottom=sections.Calculate(cappedCylinder,Plane3D::AtZ(0));
        near(areas.Calculate(bottom),areas.Calculate(middle),1e-10,"Flat cap cut");
        std::map<std::pair<int,int>,int> formulaEdges;
        for(size_t i=0;i<cappedCylinder.Indices().size();i+=3)
            for(int j=0;j<3;++j) {
                int a=cappedCylinder.Indices()[i+j], b=cappedCylinder.Indices()[i+(j+1)%3];
                ++formulaEdges[{std::min(a,b),std::max(a,b)}];
            }
        for(const auto& entry:formulaEdges) check(entry.second==2,"Formula mesh has an open edge");
        vm.ApplyShape(formulaParameters,"R");
        near(vm.Volume(),4.2*pi,1e-10,"Apply formula to viewmodel");
        const auto revision=vm.GeometryRevision();
        const auto* acceptedVertices=vm.Body().Mesh().Vertices().data();
        rejects([&] { vm.ApplyShape({4,2,.5,1},"sqrt(-1)"); });
        check(vm.GeometryRevision()==revision && vm.Formula()=="R","Invalid formula changed revision");
        check(vm.Body().Mesh().Vertices().data()==acceptedVertices,"Invalid formula replaced solid");
        near(vm.Parameters().BaseHeight,3,0,"Invalid formula changed parameters");
        vm.ApplyShape({4,2,.5,1},"R");
        near(vm.Volume(),20*pi,1e-10,"Formula parameter update");
        vm.ToggleTransparentSide(); check(!vm.TransparentPositive(),"Toggle transparent side");
        vm.ResetDefaults(); check(vm.TransparentPositive(),"Reset transparent side");
        check(vm.Formula()==FormulaProfile::DefaultFormula(),"Reset formula");
        vm.ResetDefaults();
        check(vm.HasPartJoin() && vm.JoinBoundary().size()==96,"Ice cream has a real shared seam");
        for(auto point:vm.JoinBoundary()) {
            near(point.Z,3,1e-12,"Seam height"); near(std::hypot(point.X,point.Y),1,1e-12,"Seam radius");
        }
        check(vm.JoinInSection().empty(),"Horizontal cut below seam must not show a fake joint");
        vm.SetSectionPreset(0);
        check(!vm.JoinInSection().empty(),"Axial cut must show the joint chord");
        for(const auto& trace:vm.JoinInSection()) for(auto point:trace.Points3D()) {
            near(point.Z,3,1e-12,"Joint chord height");
            near(vm.SectionPlane().SignedDistanceTo(point),0,1e-10,"Joint chord lies on cutting plane");
        }
        vm.SetSectionPreset(2); vm.SetSectionPosition(2,3);
        check(!vm.JoinInSection().empty() && vm.JoinInSection()[0].IsClosed(),"Coincident cut shows complete joint boundary");
        vm.ApplyShape(vm.Parameters(),"radius");
        check(!vm.HasPartJoin() && vm.JoinInSection().empty(),"Cylinder must not show a fake cone/cap joint");
        auto graph=HeightFieldBuilder().Build("z=x^2+y^2",{-1,1,-1,1},16);
        near(graph.BaseZ,-.25,0,"Automatic graph base");
        near(graph.Volume,11.0/3+16.0/(3*16*16),1e-10,"Paraboloid mesh volume");
        auto graphCut=sections.Calculate(graph.Mesh,Plane3D::AtX(0));
        check(!graphCut.IsEmpty() && graphCut.Contours()[0].IsClosed(),"Height field section closes");
        near(areas.Calculate(graphCut),7.0/6+4.0/(3*16*16),1e-10,"Parabolic vertical section area");
        std::map<std::pair<int,int>,std::pair<int,int>> graphEdges;
        double signedVolume=0;
        for(size_t i=0;i<graph.Mesh.Indices().size();i+=3) {
            auto [a,b,c]=graph.Mesh.GetTriangle(i/3);
            signedVolume+=(a.X*(b.Y*c.Z-b.Z*c.Y)-a.Y*(b.X*c.Z-b.Z*c.X)+a.Z*(b.X*c.Y-b.Y*c.X))/6;
            for(int j=0;j<3;++j) {
                int aIndex=graph.Mesh.Indices()[i+j], bIndex=graph.Mesh.Indices()[i+(j+1)%3];
                auto& edge=graphEdges[{std::min(aIndex,bIndex),std::max(aIndex,bIndex)}];
                ++edge.first; edge.second+=aIndex<bIndex?1:-1;
            }
        }
        for(const auto& edge:graphEdges) check(edge.second.first==2 && edge.second.second==0,"Height field mesh is consistently closed");
        near(signedVolume,graph.Volume,1e-9,"Height field volume agrees with oriented mesh");
        auto horizontalGraphCut=sections.Calculate(graph.Mesh,Plane3D::AtZ(.5));
        check(horizontalGraphCut.Contours().size()==2,"Horizontal graph cut preserves the interior hole");
        check(areas.Calculate(horizontalGraphCut)>2 && areas.Calculate(horizontalGraphCut)<3,"Graph section subtracts the hole");
        auto flatGraph=HeightFieldBuilder().Build("z=1",{-1,1,-1,1},4);
        near(flatGraph.Volume,1,1e-12,"Flat graph forms a closed box");
        for(const char* invalid:{"x+","sqrt(-1)","1/(x-x)","1000"})
            rejects([&]{ HeightFieldBuilder().Build(invalid,{-1,1,-1,1},4); });
        rejects([&]{ HeightFieldBuilder().Build("x+y",{1,-1,-1,1}); });
        vm.ApplyHeightField({-1,1,-1,1},"x^2+y^2");
        check(vm.IsHeightField() && !vm.HasPartJoin(),"Surface uses actual x/y rather than radial mapping");
        near(vm.Body().MinZ(),-.25,0,"Surface body minimum height");
        near(vm.Body().MaxZ(),2,1e-12,"Surface body maximum height");
        auto graphRevision=vm.GeometryRevision();
        rejects([&]{vm.ApplyHeightField({-1,1,-1,1},"sqrt(-1)");});
        check(vm.GeometryRevision()==graphRevision && vm.Formula()=="x^2+y^2","Invalid surface preserves accepted state");
        vm.SetSectionPreset(0); vm.SetSectionPosition(0,0);
        check(!vm.Section().IsEmpty(),"Viewmodel surface section");
        vm.ResetDefaults(); check(!vm.IsHeightField() && vm.HasPartJoin(),"Reset restores composite figure and seam");
		rejects([] { Vector3D{}.Normalized(); });
		rejects([] { Plane3D({0,0,0}, {0,0,0}); });
		rejects([] { PowerProfile(1, -1, 1); });
		rejects([] { SphericalCapProfile(0, 1, 0); });
		rejects([] { IceCreamProfile(nullptr, 1); });
		rejects([] { TriangleMesh({{0,0,0}}, {0,1,0}); });
		rejects([&] { volume.Calculate(cone, 3); });
		rejects([&] { sections.Calculate(mesh, plane, 0); });
		rejects([&] { cone.GetRadius(std::numeric_limits<double>::quiet_NaN()); });
		rejects([&] { cone.GetRadius(4); });
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
