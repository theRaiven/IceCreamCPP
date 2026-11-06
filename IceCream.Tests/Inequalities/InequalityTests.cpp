#include "../pch.h"
#include "../../IceCream.Core/Expressions/Inequalities/Inequality.h"
#include "../../IceCream.Core/Expressions/Parsing/ExpressionParser.h"

#include <limits>
#include <stdexcept>
#include <map>
#include <utility>
#include <algorithm>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Inequalities;
using namespace IceCream::Core::Expressions;
using namespace IceCream::Core::Services;
using namespace std;

namespace IceCream::Tests::Inequalities
{
    TEST_CLASS(InequalityTests)
    {
    public:
        TEST_METHOD(EvaluateExpression)
        {
            auto expression = ExpressionParser::Compile("y^2-2*x-2");
            Inequality inequality(expression, InequalityRelation::LessOrEqual);

            Assert::AreEqual(0.0, inequality.Evaluate(1, 2, 0), 1e-12);
            Assert::AreEqual(-4.0, inequality.Evaluate(3, 2, 0), 1e-12);
            Assert::AreEqual(4.0, inequality.Evaluate(-1, 2, 0), 1e-12);
        }

        TEST_METHOD(LessOrEqual)
        {
            auto expression = ExpressionParser::Compile("z-4");
            Inequality inequality(expression, InequalityRelation::LessOrEqual);

            Assert::IsTrue(inequality.IsSatisfied(0, 0, 3));
            Assert::IsTrue(inequality.IsSatisfied(0, 0, 4));
            Assert::IsFalse(inequality.IsSatisfied(0, 0, 5));
        }

        TEST_METHOD(StrictLess)
        {
            auto expression = ExpressionParser::Compile("z-4");
            Inequality inequality(expression, InequalityRelation::Less);

            Assert::IsTrue(inequality.IsSatisfied(0, 0, 3));
            Assert::IsFalse(inequality.IsSatisfied(0, 0, 4));
            Assert::IsFalse(inequality.IsSatisfied(0, 0, 5));
        }

        TEST_METHOD(EmptyExpression)
        {
            Assert::ExpectException<std::invalid_argument>([]()
                {
                    Inequality inequality({}, InequalityRelation::LessOrEqual);
                });
        }

        TEST_METHOD(NonFiniteResult)
        {
            auto expression = ExpressionParser::Compile("sqrt(-1)");
            Inequality inequality(expression, InequalityRelation::LessOrEqual);

            Assert::ExpectException<std::domain_error>([&inequality]()
                {
                    inequality.Evaluate(0, 0, 0);
                });
        }


        TEST_METHOD(BuildSphere)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-9");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -4, -4, -4 }, { 4, 4, 4 }, 40, 40, 40);

            Assert::IsTrue(mesh.Vertices().size() > 0);
            Assert::IsTrue(mesh.TriangleCount() > 0);
            Assert::AreEqual(mesh.TriangleCount() * 3, mesh.Indices().size());

            for (const Point3D& point : mesh.Vertices())
            {
                double radiusSquared = point.X * point.X + point.Y * point.Y + point.Z * point.Z;
                Assert::IsTrue(abs(radiusSquared - 9.0) < 0.1);
            }
            const auto& indices = mesh.Indices();

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                Assert::IsTrue(indices[i] != indices[i + 1]);
                Assert::IsTrue(indices[i + 1] != indices[i + 2]);
                Assert::IsTrue(indices[i] != indices[i + 2]);
            }
        }

        TEST_METHOD(NoDegenerateTriangles)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-9");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -4, -4, -4 }, { 4, 4, 4 }, 40, 40, 40);

            const auto& vertices = mesh.Vertices();
            const auto& indices = mesh.Indices();

            Assert::IsTrue(!indices.empty());

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const Point3D& a = vertices[indices[i]];
                const Point3D& b = vertices[indices[i + 1]];
                const Point3D& c = vertices[indices[i + 2]];

                double ux = b.X - a.X;
                double uy = b.Y - a.Y;
                double uz = b.Z - a.Z;

                double vx = c.X - a.X;
                double vy = c.Y - a.Y;
                double vz = c.Z - a.Z;

                double nx = uy * vz - uz * vy;
                double ny = uz * vx - ux * vz;
                double nz = ux * vy - uy * vx;

                double areaSquared4 = nx * nx + ny * ny + nz * nz;

                Assert::IsTrue(areaSquared4 > 1e-24 * 0.2 * 0.2 * 0.2 * 0.2);
            }
        }
        TEST_METHOD(SphereNormals)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-9");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -4, -4, -4 }, { 4, 4, 4 }, 40, 40, 40);

            const auto& vertices = mesh.Vertices();
            const auto& indices = mesh.Indices();

            int outward = 0;
            int inward = 0;

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const Point3D& a = vertices[indices[i]];
                const Point3D& b = vertices[indices[i + 1]];
                const Point3D& c = vertices[indices[i + 2]];

                double ux = b.X - a.X;
                double uy = b.Y - a.Y;
                double uz = b.Z - a.Z;

                double vx = c.X - a.X;
                double vy = c.Y - a.Y;
                double vz = c.Z - a.Z;

                double nx = uy * vz - uz * vy;
                double ny = uz * vx - ux * vz;
                double nz = ux * vy - uy * vx;

                double cx = (a.X + b.X + c.X) / 3.0;
                double cy = (a.Y + b.Y + c.Y) / 3.0;
                double cz = (a.Z + b.Z + c.Z) / 3.0;

                double dot = nx * cx + ny * cy + nz * cz;

                if (dot > 0) ++outward;
                else if (dot < 0) ++inward;
            }

            Logger::WriteMessage(("Outward: " + std::to_string(outward) + ", Inward: " + std::to_string(inward)).c_str());

            Assert::IsTrue(outward > 0);
            Assert::AreEqual(0, inward);
        }
        TEST_METHOD(SphereWatertight)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-9");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -4, -4, -4 }, { 4, 4, 4 }, 40, 40, 40);

            const auto& indices = mesh.Indices();

            map<pair<int, int>, int> edgeCounts;

            auto AddEdge = [&](int a, int b)
                {
                    if (a > b) swap(a, b);
                    ++edgeCounts[{a, b}];
                };

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                int a = indices[i];
                int b = indices[i + 1];
                int c = indices[i + 2];

                AddEdge(a, b);
                AddEdge(b, c);
                AddEdge(c, a);
            }

            int boundaryEdges = 0;
            int nonManifoldEdges = 0;

            for (const auto& [edge, count] : edgeCounts)
            {
                if (count == 1) ++boundaryEdges;
                else if (count != 2) ++nonManifoldEdges;
            }

            Logger::WriteMessage(("Boundary edges: " + to_string(boundaryEdges) +
                ", Non-manifold edges: " + to_string(nonManifoldEdges)).c_str());

            Assert::IsTrue(!edgeCounts.empty());
            Assert::AreEqual(0, boundaryEdges);
            Assert::AreEqual(0, nonManifoldEdges);
        }
        TEST_METHOD(SphereVolume)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2+z^2-9");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -4, -4, -4 }, { 4, 4, 4 }, 40, 40, 40);

            const auto& vertices = mesh.Vertices();
            const auto& indices = mesh.Indices();

            double volume = 0.0;

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const Point3D& a = vertices[indices[i]];
                const Point3D& b = vertices[indices[i + 1]];
                const Point3D& c = vertices[indices[i + 2]];

                double nx = b.Y * c.Z - b.Z * c.Y;
                double ny = b.Z * c.X - b.X * c.Z;
                double nz = b.X * c.Y - b.Y * c.X;

                volume += a.X * nx + a.Y * ny + a.Z * nz;
            }

            volume /= 6.0;

            double expected = 36.0 * acos(-1.0);
            double relativeError = abs(volume - expected) / expected;

            Logger::WriteMessage(("Calculated volume: " + to_string(volume) +
                ", Expected volume: " + to_string(expected) +
                ", Relative error: " + to_string(relativeError * 100) + "%").c_str());

            Assert::IsTrue(volume > 0);
            Assert::IsTrue(relativeError < 0.03);
        }
        TEST_METHOD(TorusWatertight)
        {
            auto expression = ExpressionParser::Compile("(x^2+y^2+z^2+3)^2-16*(x^2+y^2)");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -3.5, -3.5, -1.5 }, { 3.5, 3.5, 1.5 }, 70, 70, 40);

            const auto& indices = mesh.Indices();

            map<pair<int, int>, int> edgeCounts;

            auto AddEdge = [&](int a, int b)
                {
                    if (a > b) swap(a, b);
                    ++edgeCounts[{a, b}];
                };

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                int a = indices[i];
                int b = indices[i + 1];
                int c = indices[i + 2];

                AddEdge(a, b);
                AddEdge(b, c);
                AddEdge(c, a);
            }

            int boundaryEdges = 0;
            int nonManifoldEdges = 0;

            for (const auto& [edge, count] : edgeCounts)
            {
                if (count == 1) ++boundaryEdges;
                else if (count != 2) ++nonManifoldEdges;
            }

            Logger::WriteMessage(("Boundary edges: " + to_string(boundaryEdges) +
                ", Non-manifold edges: " + to_string(nonManifoldEdges)).c_str());

            Assert::IsTrue(!edgeCounts.empty());
            Assert::AreEqual(0, boundaryEdges);
            Assert::AreEqual(0, nonManifoldEdges);
        }
        TEST_METHOD(TorusVolume)
        {
            auto expression = ExpressionParser::Compile("(x^2+y^2+z^2+3)^2-16*(x^2+y^2)");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -3.5, -3.5, -1.5 }, { 3.5, 3.5, 1.5 }, 70, 70, 40);

            const auto& vertices = mesh.Vertices();
            const auto& indices = mesh.Indices();

            double volume = 0.0;

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                const Point3D& a = vertices[indices[i]];
                const Point3D& b = vertices[indices[i + 1]];
                const Point3D& c = vertices[indices[i + 2]];

                double nx = b.Y * c.Z - b.Z * c.Y;
                double ny = b.Z * c.X - b.X * c.Z;
                double nz = b.X * c.Y - b.Y * c.X;

                volume += a.X * nx + a.Y * ny + a.Z * nz;
            }

            volume /= 6.0;

            double expected = 4.0 * pow(acos(-1.0), 2);
            double relativeError = abs(volume - expected) / expected;

            Logger::WriteMessage(("Calculated volume: " + to_string(volume) +
                ", Expected volume: " + to_string(expected) +
                ", Relative error: " + to_string(relativeError * 100) + "%").c_str());

            Assert::IsTrue(volume > 0);
            Assert::IsTrue(relativeError < 0.03);
        }
        TEST_METHOD(AmbiguousFaceConsistency)
        {
            auto expression = ExpressionParser::Compile("x*y+0.1*z");

            ImplicitSurfaceBuilder builder;
            TriangleMesh mesh = builder.Build(expression, { -1, -1, -1 }, { 1, 1, 1 }, 1, 1, 2);

            const auto& vertices = mesh.Vertices();
            const auto& indices = mesh.Indices();

            map<pair<int, int>, int> sharedFaceEdges;

            auto AddEdge = [&](int a, int b)
                {
                    if (abs(vertices[a].Z) > 1e-10 || abs(vertices[b].Z) > 1e-10)
                    {
                        return;
                    }

                    if (a > b) swap(a, b);

                    ++sharedFaceEdges[{a, b}];
                };

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                int a = indices[i];
                int b = indices[i + 1];
                int c = indices[i + 2];

                AddEdge(a, b);
                AddEdge(b, c);
                AddEdge(c, a);
            }

            Assert::IsTrue(!sharedFaceEdges.empty());

            int cracks = 0;

            for (const auto& [edge, count] : sharedFaceEdges)
            {
                if (count != 2)
                {
                    ++cracks;
                }
            }

            Logger::WriteMessage(("Shared face edges: " + to_string(sharedFaceEdges.size()) +
                ", Cracks: " + to_string(cracks)).c_str());

            Assert::AreEqual(0, cracks);
        }

    };
}