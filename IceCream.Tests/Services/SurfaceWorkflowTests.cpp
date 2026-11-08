#include "../pch.h"
#include "../../IceCream.App/ViewModels/MainViewModel.h"
#include "../../IceCream.Core/Services/Meshing/MeshClipper.h"
#include "../../IceCream.Core/Expressions/Inequalities/InequalityParser.h"
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Services;
using namespace IceCream::Core::Expressions;
using namespace IceCream::Core::Inequalities;
using IceCream::App::ViewModels::MainViewModel;

namespace IceCream::Tests::Services
{
    TEST_CLASS(SurfaceWorkflowTests)
    {
    public:
        TEST_METHOD(EquationSphereHasVolumeAndRotatingSections)
        {
            MainViewModel model;
            auto light = model.Lighting();
            light.AmbientIntensity = .45f;
            model.SetLighting(light);
            auto appearance = model.Appearance();
            appearance.CoordinateSize = 28;
            model.SetAppearance(appearance);
            model.SetSectionPose({0, 0, 0}, {});
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 32,32,32);
            Assert::IsTrue(model.IsImplicitSurface() && model.HasVolume() && model.UsesMeshVolume());
            Assert::AreEqual(36 * std::acos(-1.0), model.Volume(), 1.0);
            for (auto rotation : {Vector3D{}, Vector3D{35,20,15}, Vector3D{90,0,0}})
            {
                model.SetSectionPose({0,0,0}, rotation);
                Assert::IsTrue(model.HasSectionArea());
                Assert::AreEqual(9 * std::acos(-1.0), model.SectionArea(), .3);
            }
            Assert::AreEqual(.45f, model.Lighting().AmbientIntensity);
            Assert::AreEqual(28, model.Appearance().CoordinateSize);
            Assert::IsFalse(model.IsIntegralBody() || model.HasPartJoin());
        }

        TEST_METHOD(EquationAndZeroFieldAgree)
        {
            MainViewModel model;
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 16,16,16);
            double volume = model.Volume();
            model.ApplyImplicitSurface("x^2+y^2+z^2-9", {-4,-4,-4}, {4,4,4}, 16,16,16);
            Assert::AreEqual(volume, model.Volume(), 1e-10);
        }

        TEST_METHOD(ResetFromImplicitReturnsToFormulaParts)
        {
            MainViewModel model;
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 12,12,12, "x>=0");
            model.ResetDefaults();
            Assert::IsTrue(model.IsIntegralBody() && model.HasVolume());
            Assert::IsFalse(model.IsImplicitSurface() || model.UsesMeshVolume());
            Assert::IsTrue(model.Constraints().empty());
            Assert::AreEqual(5 * std::acos(-1.0) / 3, model.Volume(), .001);
        }

        TEST_METHOD(ClippedSphereKeepsOpenSectionsWithoutInventedMeasurements)
        {
            MainViewModel model;
            model.SetSectionPose({0,0,1}, {});
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 20,20,20,
                "\n x>=0;\n 0<=z<=4-x \n");
            Assert::IsFalse(model.HasVolume());
            Assert::IsFalse(model.HasSectionArea());
            Assert::IsFalse(model.Section().IsEmpty());
            Assert::IsTrue(std::isnan(model.Volume()));
            for (const auto& point : model.Body().Mesh().Vertices())
            {
                Assert::IsTrue(point.X >= -1e-8 && point.Z >= -1e-8 && point.Z <= 4-point.X+1e-8);
            }
            model.SetSectionRotation(0, 30); // Open contours must not reject plane movement.
            model.SetSectionPreset(0);
        }

        TEST_METHOD(OpenBoundingBoxAlsoDisablesVolume)
        {
            MainViewModel model;
            model.SetSectionPose({0,0,0}, {});
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {0,-4,-4}, {4,4,4}, 16,20,20);
            Assert::IsFalse(model.HasVolume());
            Assert::IsFalse(model.HasSectionArea());
        }

        TEST_METHOD(NonIntersectingConstraintPreservesClosedVolume)
        {
            MainViewModel model;
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 16,16,16, "x<=4\ny^2<=20");
            Assert::IsTrue(model.HasVolume());
            Assert::AreEqual(36 * std::acos(-1.0), model.Volume(), 3.0);
        }

        TEST_METHOD(ClosedSectionOfAnOpenMeshStillHasArea)
        {
            MainViewModel model;
            model.SetSectionPose({0,0,1}, {});
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 24,24,24, "z>=0");
            Assert::IsFalse(model.HasVolume());
            Assert::IsTrue(model.HasSectionArea());
            Assert::AreEqual(8 * std::acos(-1.0), model.SectionArea(), .5);
        }

        TEST_METHOD(InvalidBuildPreservesAcceptedGeometryAndMode)
        {
            MainViewModel model;
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 12,12,12);
            auto revision = model.GeometryRevision();
            double volume = model.Volume();
            for (const auto& constraint : {"x>=10", "x<=", "x<1 2"})
                Assert::ExpectException<std::invalid_argument>([&] {
                    model.ApplyImplicitSurface("x^2+y^2+z^2=1", {-4,-4,-4}, {4,4,4}, 12,12,12, constraint);
                });
            Assert::ExpectException<std::invalid_argument>([&] {
                model.ApplyImplicitSurface("x+y=", {-4,-4,-4}, {4,4,4}, 12,12,12);
            });
            Assert::AreEqual(revision, model.GeometryRevision());
            Assert::AreEqual(volume, model.Volume());
            Assert::AreEqual(std::string("x^2+y^2+z^2=9"), model.Formula());
            Assert::IsTrue(model.IsImplicitSurface() && model.Constraints().empty());
        }

        TEST_METHOD(GridValidationHappensBeforeBuilding)
        {
            MainViewModel model;
            for (int count : {0, 1, 129})
                Assert::ExpectException<std::invalid_argument>([&] {
                    model.ApplyImplicitSurface("x", {-4,-4,-4}, {4,4,4}, count,8,8);
                });
            Assert::ExpectException<std::invalid_argument>([&] {
                model.ApplyImplicitSurface("x", {-4,-4,-4}, {4,4,4}, 128,128,128);
            });
            Assert::ExpectException<std::invalid_argument>([&] {
                model.ApplyImplicitSurface("x", {4,-4,-4}, {-4,4,4}, 8,8,8);
            });
        }

        TEST_METHOD(IntegralPartsClipTogetherAndRetainTheirColors)
        {
            MainViewModel model;
            auto parts = IntegralBodyBuilder::DefaultParts();
            model.SetSectionPose({0,0,2}, {});
            model.ApplyIntegralParts(parts, "x>=0");
            Assert::IsTrue(model.IsIntegralBody() && model.UsesMeshVolume());
            Assert::IsFalse(model.HasVolume() || model.HasSectionArea());
            const auto& colors = model.TriangleColors();
            Assert::AreEqual(model.Body().Mesh().TriangleCount(), colors.size());
            for (const auto& part : parts)
                Assert::IsTrue(std::find(colors.begin(), colors.end(), part.Color) != colors.end());
            for (const auto& point : model.Body().Mesh().Vertices()) Assert::IsTrue(point.X >= -1e-8);
            model.ApplyIntegralParts(parts);
            Assert::IsTrue(model.HasVolume() && model.HasSectionArea());
            Assert::IsFalse(model.UsesMeshVolume() || model.IsImplicitSurface());
            Assert::AreEqual(5 * std::acos(-1.0) / 3, model.Volume(), .001);
        }

        TEST_METHOD(AllDependentAxesAndRadialCoordinatesStillWork)
        {
            MainViewModel model;
            IntegralPart part;
            part.Definition = PartDefinition::Function;
            part.Function = "1";
            for (int axis : {0,1,2})
            {
                part.DependentAxis = axis;
                model.ApplyIntegralParts({part});
                Assert::AreEqual(1.0, model.Volume(), 1e-10);
                model.ApplyIntegralParts({part}, "x<=0.5");
                Assert::IsFalse(model.HasVolume());
            }
            part.DependentAxis = 2;
            part.Function = "2+r+cos(phi)";
            part.Upper = {{"1","2*pi","1"}};
            for (auto coordinates : {CoordinateSystem::Cylindrical, CoordinateSystem::Polar})
            {
                part.Coordinates = coordinates;
                model.ApplyIntegralParts({part});
                Assert::AreEqual(8 * std::acos(-1.0) / 3, model.Volume(), 1e-7);
            }
        }

        TEST_METHOD(LegacyHeightFieldAndRadialBodyAcceptConstraints)
        {
            MainViewModel model;
            model.ApplyShape(model.Parameters(), model.Formula(), "x>=0");
            Assert::IsFalse(model.HasVolume() || model.HasPartJoin());
            model.SetDependentAxis(0);
            model.ApplyHeightField({-1,1,-1,1}, "x+y+3", "x<=3.5");
            Assert::IsFalse(model.HasVolume());
            for (const auto& p : model.Body().Mesh().Vertices())
            {
                auto world = model.SurfaceRotation() * glm::dvec3(p.X,p.Y,p.Z);
                Assert::IsTrue(world.x <= 3.5+1e-8);
            }
        }

        TEST_METHOD(ClipperProvenanceTracksSubdividedFaces)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-1");
            auto source = ImplicitSurfaceBuilder().Build(expression, {-2,-2,-2}, {2,2,2}, 12,12,12);
            std::vector<size_t> provenance;
            auto clipped = MeshClipper().Clip(source, InequalityRegion(InequalityParser::Parse("x>=0.15")), {}, &provenance);
            Assert::AreEqual(clipped.TriangleCount(), provenance.size());
            for (size_t triangle = 0; triangle < provenance.size(); ++triangle)
            {
                auto [a,b,c] = source.GetTriangle(provenance[triangle]);
                auto normal = (b-a).Cross(c-a).Normalized();
                auto [p,q,r] = clipped.GetTriangle(triangle);
                for (const auto& point : {p,q,r}) Assert::AreEqual(0.0, normal.Dot(point-a), 1e-8);
            }
        }

        TEST_METHOD(ConstraintSurfacesKeepEachBoundaryOfAChain)
        {
            MainViewModel model;
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 16,16,16,
                "0<=z<=4-x, y^2<=2*x+2");
            const auto& surfaces = model.ConstraintSurfaces();
            Assert::AreEqual(size_t(3), surfaces.size());
            const char* labels[] = {"0 <= z", "z <= 4-x", "y^2 <= 2*x+2"};
            const char* fields[] = {"z", "z-4+x", "y^2-2*x-2"};
            for (size_t i = 0; i < surfaces.size(); ++i)
            {
                const auto& surface = surfaces[i];
                Assert::AreEqual(std::string(labels[i]), surface.Condition);
                Assert::IsFalse(surface.Undefined);
                Assert::IsFalse(surface.Triangles.empty());
                auto field = ExpressionParser::Compile(fields[i]);
                for (size_t k = 0; k < surface.Triangles.size(); k += 3)
                    Assert::AreEqual(0.0, field(surface.Triangles[k], surface.Triangles[k+1], surface.Triangles[k+2]), .15);
            }
            Assert::IsFalse(model.HasVolume()); // Guides are not caps for the clipped body.
        }

        TEST_METHOD(ConstraintVisibilityDoesNotRebuildOrChangeMeasurements)
        {
            MainViewModel model;
            model.SetSectionPose({0,0,0}, {});
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 16,16,16, "x<=4");
            auto revision = model.GeometryRevision();
            auto triangles = model.Body().Mesh().TriangleCount();
            double volume = model.Volume(), area = model.SectionArea();
            auto appearance = model.Appearance();
            for (bool visible : {false, true})
            {
                appearance.ShowConstraintSurfaces = visible;
                model.SetAppearance(appearance);
                Assert::AreEqual(revision, model.GeometryRevision());
                Assert::AreEqual(triangles, model.Body().Mesh().TriangleCount());
                Assert::AreEqual(volume, model.Volume());
                Assert::AreEqual(area, model.SectionArea());
                Assert::AreEqual(size_t(1), model.ConstraintSurfaces().size());
            }
        }

        TEST_METHOD(ChangingModesClearsOldConstraintSurfaces)
        {
            MainViewModel model;
            model.ApplyIntegralParts(IntegralBodyBuilder::DefaultParts(), "x>=0");
            Assert::AreEqual(size_t(1), model.ConstraintSurfaces().size());
            model.ApplyImplicitSurface("x^2+y^2+z^2=9", {-4,-4,-4}, {4,4,4}, 12,12,12);
            Assert::IsTrue(model.ConstraintSurfaces().empty());
            model.ApplyIntegralParts(IntegralBodyBuilder::DefaultParts());
            Assert::IsTrue(model.ConstraintSurfaces().empty());
        }

        TEST_METHOD(UndefinedGuideOutsideTheBodyDoesNotRejectValidGeometry)
        {
            MainViewModel model;
            IntegralPart part;
            part.Definition = PartDefinition::Function;
            part.Function = "1";
            part.Lower[0] = "1";
            part.Upper[0] = "2";
            model.ApplyIntegralParts({part}, "sqrt(x-1)<=2");
            Assert::IsTrue(model.HasVolume());
            Assert::AreEqual(1.0, model.Volume(), 1e-10);
            Assert::AreEqual(size_t(1), model.ConstraintSurfaces().size());
            Assert::IsTrue(model.ConstraintSurfaces()[0].Undefined);
            Assert::IsTrue(model.ConstraintSurfaces()[0].Triangles.empty());
        }

        TEST_METHOD(MeshVolumeRejectsMissingOrMisorientedFaces)
        {
            auto mesh = ImplicitSurfaceBuilder().Build(ExpressionParser::Compile("x^2+y^2+z^2-1"),
                {-2,-2,-2}, {2,2,2}, 12,12,12);
            VolumeCalculator volume;
            Assert::IsTrue(volume.TryCalculate(mesh).has_value());
            auto indices = mesh.Indices();
            std::swap(indices[0], indices[1]);
            Assert::IsFalse(volume.TryCalculate(TriangleMesh(mesh.Vertices(), indices)).has_value());
            indices = mesh.Indices();
            indices.resize(indices.size()-3);
            TriangleMesh open(mesh.Vertices(), indices);
            Assert::IsFalse(volume.TryCalculate(open).has_value());
            Assert::ExpectException<std::invalid_argument>([&] { volume.Calculate(open); });
        }

        TEST_METHOD(EditingPresetGraphDiscardsItsHiddenBase)
        {
            auto part = IntegralBodyBuilder::DefaultParts()[1];
            part.SetFunction("x^3+y");
            part.Lower = {{"0","0","0"}};
            part.Upper = {{"1","1","1"}};
            Assert::AreEqual(std::string("0"), part.Base);
            MainViewModel model;
            model.ApplyIntegralParts({part});
            Assert::IsTrue(model.HasVolume());
            Assert::AreEqual(.75, model.Volume(), 1e-9);
            model.ApplyIntegralParts({part}, "0<=z<10\ny<=4*z\ny^2<=-2*x+2");
            Assert::IsFalse(model.Body().Mesh().Indices().empty());
            for (const auto& p : model.Body().Mesh().Vertices())
            {
                Assert::IsTrue(p.Z >= -1e-8 && p.Z < 10);
                Assert::IsTrue(p.Y <= 4*p.Z+1e-8);
                Assert::IsTrue(p.Y*p.Y <= -2*p.X+2+1e-8);
            }
        }

        TEST_METHOD(GraphsBelowOrAcrossZeroHaveGeometricVolume)
        {
            auto part = IntegralBodyBuilder::DefaultParts()[1];
            part.Lower = {{"0","0","0"}};
            part.Upper = {{"1","1","1"}};
            part.SetFunction("-2");
            MainViewModel model;
            model.ApplyIntegralParts({part});
            Assert::AreEqual(2.0, model.Volume(), 1e-9);
            part.SetFunction("x-y");
            model.ApplyIntegralParts({part});
            Assert::AreEqual(1.0/3, model.Volume(), 1e-4);
            model.ApplyIntegralParts({IntegralBodyBuilder::AsIntegral(part)});
            Assert::AreEqual(1.0/3, model.Volume(), .001);
            auto revision = model.GeometryRevision();
            part.SetFunction("sqrt(-1)");
            Assert::ExpectException<std::invalid_argument>([&] { model.ApplyIntegralParts({part}); });
            Assert::AreEqual(revision, model.GeometryRevision());
        }

        TEST_METHOD(BoundErrorsIdentifyTheAxisAndPreserveIntegralOrdering)
        {
            IntegralPart part;
            part.Name = "Example";
            part.Lower[0] = "1";
            part.Upper[0] = "0";
            try
            {
                IntegralBodyBuilder().Build({part});
                Assert::Fail(L"Reversed explicit integral bounds must be rejected");
            }
            catch (const std::invalid_argument& error)
            {
                std::string message = error.what();
                Assert::IsTrue(message.find("Границы по x перепутаны") != std::string::npos);
                Assert::IsTrue(message.find("Example") != std::string::npos);
            }
            part.Lower[0] = "0";
            part.Upper[0] = "101";
            try
            {
                IntegralBodyBuilder().Build({part});
                Assert::Fail(L"Out-of-range bounds must be rejected");
            }
            catch (const std::invalid_argument& error)
            {
                std::string message = error.what();
                Assert::IsTrue(message.find("Границы по x выходят за диапазон") != std::string::npos);
            }
        }

        TEST_METHOD(UneditedPresetsKeepTheirJoinAndIntegralEditsKeepBounds)
        {
            auto parts = IntegralBodyBuilder::DefaultParts();
            for (auto& part : parts) part.SetFunction(part.Function);
            auto solid = IntegralBodyBuilder().Build(parts);
            Assert::AreEqual(5*std::acos(-1.0)/3, solid.Integral, .001);
            Assert::IsFalse(solid.Seams.empty());
            auto integral = IntegralBodyBuilder::AsIntegral(parts[0]);
            auto low = integral.Lower, high = integral.Upper;
            integral.SetFunction("x+y");
            Assert::IsTrue(low == integral.Lower && high == integral.Upper);
        }

        TEST_METHOD(EditedGraphsUseTheSameRulesForEveryDependentCoordinate)
        {
            const char* positive[] = {"y^3+z", "x^3+z", "x^3+y"};
            const char* crossing[] = {"y-z", "x-z", "x-y"};
            for (int axis = 0; axis < 3; ++axis)
            {
                auto part = IntegralBodyBuilder::DefaultParts()[1];
                part.DependentAxis = axis;
                part.Lower = {{"0","0","0"}};
                part.Upper = {{"1","1","1"}};
                part.SetFunction(positive[axis]);
                Assert::AreEqual(.75, IntegralBodyBuilder().Build({part}).Integral, 1e-9);
                part.SetFunction("-2");
                Assert::AreEqual(2.0, IntegralBodyBuilder().Build({part}).Integral, 1e-9);
                part.SetFunction(crossing[axis]);
                Assert::AreEqual(1.0/3, IntegralBodyBuilder().Build({part}).Integral, 1e-4);
            }
        }

        TEST_METHOD(RadialGraphsAlsoHandleBothSidesOfZero)
        {
            auto part = IntegralBodyBuilder::DefaultParts()[1];
            part.Lower = {{"0","0","0"}};
            part.Upper = {{"1","2*pi","1"}};
            part.SetFunction("r-0.5");
            for (auto coordinates : {CoordinateSystem::Cylindrical, CoordinateSystem::Polar})
            {
                part.Coordinates = coordinates;
                Assert::AreEqual(std::acos(-1.0)/4, IntegralBodyBuilder().Build({part}).Integral, 1e-7);
            }
        }

        TEST_METHOD(InvalidFunctionInputsPreserveTheAcceptedBody)
        {
            MainViewModel model;
            IntegralPart accepted;
            accepted.Definition = PartDefinition::Function;
            accepted.Name = "Input case";
            model.ApplyIntegralParts({accepted});
            auto revision = model.GeometryRevision();
            for (const char* formula : {"", "x+", "z+1", "unknown(x)", "sqrt(-1)", "1/(x-0.5)", "log(x)", "150"})
            {
                auto draft = accepted;
                draft.SetFunction(formula);
                try
                {
                    model.ApplyIntegralParts({draft});
                    Assert::Fail(L"Invalid input must not replace the accepted body");
                }
                catch (const std::invalid_argument& error)
                {
                    Assert::IsTrue(std::string(error.what()).find("Input case") != std::string::npos);
                }
                Assert::AreEqual(revision, model.GeometryRevision());
                Assert::AreEqual(1.0, model.Volume(), 1e-9);
                Assert::IsTrue(model.IsIntegralBody() && model.HasVolume());
            }
        }

        TEST_METHOD(FullLengthFormulaSurvivesIntegralConversion)
        {
            IntegralPart part;
            part.Definition = PartDefinition::Function;
            part.SetFunction("x" + std::string(511, ' '));
            Assert::AreEqual(.5, IntegralBodyBuilder().Build({part}).Integral, 1e-9);
            auto integral = IntegralBodyBuilder::AsIntegral(part);
            Assert::IsTrue(integral.Upper[2].size() > 512);
            Assert::AreEqual(.5, IntegralBodyBuilder().Build({integral}).Integral, 1e-9);
            integral.Lower[0] = "0" + std::string(2047, ' ');
            Assert::AreEqual(.5, IntegralBodyBuilder().Build({integral}).Integral, 1e-9);
            integral.Lower[0] += ' ';
            Assert::ExpectException<std::invalid_argument>([&] { IntegralBodyBuilder().Build({integral}); });
            part.SetFunction("x" + std::string(512, ' '));
            Assert::ExpectException<std::invalid_argument>([&] { IntegralBodyBuilder().Build({part}); });
        }

        TEST_METHOD(WhitespaceEditsPreservePresetGeometry)
        {
            auto parts = IntegralBodyBuilder::DefaultParts();
            parts[1].SetFunction(" 3 + sqrt( max( 0, 1 - x^2 - y^2 ) ) ");
            Assert::AreEqual(std::string("3"), parts[1].Base);
            auto solid = IntegralBodyBuilder().Build(parts);
            Assert::AreEqual(5*std::acos(-1.0)/3, solid.Integral, .001);
            Assert::IsFalse(solid.Seams.empty());
        }

        TEST_METHOD(SectionCanReachLargeCoordinateBodies)
        {
            MainViewModel model;
            model.SetSectionPose({0,0,300}, {});
            model.ApplyImplicitSurface("(x-200)^2+(y+100)^2+(z-300)^2=9",
                {196,-104,296},{204,-96,304},24,24,24);
            Assert::IsTrue(model.HasSectionArea());
            Assert::AreEqual(9*std::acos(-1.0),model.SectionArea(),.6);
            model.SetSectionPosition(2,301);
            Assert::AreEqual(8*std::acos(-1.0),model.SectionArea(),.6);
        }

        TEST_METHOD(ExtendedSectionPositionStillValidatesAndRollsBack)
        {
            MainViewModel model;
            model.SetSectionPose({300,-500,800}, {});
            model.SetSectionPosition(1,-1000);
            Assert::AreEqual(-1000.0,model.SectionPosition().Y);
            Assert::ExpectException<std::out_of_range>([&] { model.SetSectionPosition(1,-1001); });
            Assert::ExpectException<std::out_of_range>([&] { model.SetSectionPose({0,0,1001}, {}); });
            Assert::AreEqual(300.0,model.SectionPosition().X);
            Assert::AreEqual(-1000.0,model.SectionPosition().Y);
            Assert::AreEqual(800.0,model.SectionPosition().Z);
        }
    };
}
