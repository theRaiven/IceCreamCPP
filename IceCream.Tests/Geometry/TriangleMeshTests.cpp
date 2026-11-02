#include "../pch.h"

#include <limits>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;

namespace IceCream::Tests::Geometry
{
	TEST_CLASS(TriangleMeshTests)
	{
	public:
		TEST_METHOD(Constructor_ValidData_PreservesVerticesAndIndices)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			std::vector<int> indices{ 0, 1, 2 };

			TriangleMesh mesh(vertices, indices);

			Assert::AreEqual(size_t(3), mesh.Vertices().size());
			Assert::AreEqual(size_t(3), mesh.Indices().size());

			Assert::AreEqual(1.0, mesh.Vertices()[1].X);
			Assert::AreEqual(2, mesh.Indices()[2]);
		}

		TEST_METHOD(Constructor_EmptyData_CreatesEmptyMesh)
		{
			TriangleMesh mesh({}, {});

			Assert::AreEqual(size_t(0), mesh.Vertices().size());
			Assert::AreEqual(size_t(0), mesh.Indices().size());
			Assert::AreEqual(size_t(0), mesh.TriangleCount());
		}

		TEST_METHOD(Constructor_IndicesCountNotMultipleOfThree_ThrowsException)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			Assert::ExpectException<std::invalid_argument>([&vertices]()
			{
				TriangleMesh mesh(vertices, { 0, 1 });
			});
		}

		TEST_METHOD(Constructor_NegativeIndex_ThrowsException)
		{
			std::vector<Point3D> vertices{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			Assert::ExpectException<std::out_of_range>([&vertices]()
			{
				TriangleMesh mesh(vertices, { 0, 1, -1 });
			});
		}

		TEST_METHOD(Constructor_IndexEqualToVerticesCount_ThrowsException)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			Assert::ExpectException<std::out_of_range>([&vertices]()
			{
				TriangleMesh mesh(vertices, { 0, 1, 3 });
			});
		}

		TEST_METHOD(Constructor_InfiniteVertexCoordinate_ThrowsException)
		{
			double inf = std::numeric_limits<double>::infinity();

			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ inf, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			Assert::ExpectException<std::invalid_argument>([&vertices]()
			{
				TriangleMesh mesh(vertices, { 0, 1, 2 });
			});
		}

		TEST_METHOD(Constructor_NaNVertexCoordinate_ThrowsException)
		{
			double nan = std::numeric_limits<double>::quiet_NaN();

			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, nan, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			Assert::ExpectException<std::invalid_argument>([&vertices]()
			{
				TriangleMesh mesh(vertices, { 0, 1, 2 });
			});
		}

		TEST_METHOD(TriangleCount_OneTriangle_Returns1)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			TriangleMesh mesh(vertices, { 0, 1, 2 });

			Assert::AreEqual(size_t(1), mesh.TriangleCount());
		}

		TEST_METHOD(TriangleCount_TwoTriangles_Returns2)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 1.0, 1.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			TriangleMesh mesh(vertices, { 0, 1, 2, 0, 2, 3 });

			Assert::AreEqual(size_t(2), mesh.TriangleCount());
		}

		TEST_METHOD(GetTriangle_ReturnsVerticesInCorrectOrder)
		{
			std::vector<Point3D> vertices
			{
				{ 10.0, 0.0, 0.0 },
				{ 20.0, 0.0, 0.0 },
				{ 30.0, 0.0, 0.0 }
			};

			TriangleMesh mesh(vertices, { 2, 0, 1 });

			auto [first, second, third] = mesh.GetTriangle(0);

			Assert::AreEqual(30.0, first.X);
			Assert::AreEqual(10.0, second.X);
			Assert::AreEqual(20.0, third.X);
		}

		TEST_METHOD(GetTriangle_SecondTriangle_ReturnsCorrectVertices)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 1.0, 1.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			TriangleMesh mesh(vertices, { 0, 1, 2, 0, 2, 3 });

			auto [first, second, third] = mesh.GetTriangle(1);

			Assert::AreEqual(0.0, first.X);
			Assert::AreEqual(0.0, first.Y);

			Assert::AreEqual(1.0, second.X);
			Assert::AreEqual(1.0, second.Y);

			Assert::AreEqual(0.0, third.X);
			Assert::AreEqual(1.0, third.Y);
		}

		TEST_METHOD(GetTriangle_IndexEqualToTriangleCount_ThrowsException)
		{
			std::vector<Point3D> vertices
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 0.0, 0.0 },
				{ 0.0, 1.0, 0.0 }
			};

			TriangleMesh mesh(vertices, { 0, 1, 2 });

			Assert::ExpectException<std::out_of_range>([&mesh]()
			{
				mesh.GetTriangle(1);
			});
		}

		TEST_METHOD(GetTriangle_EmptyMesh_ThrowsException)
		{
			TriangleMesh mesh({}, {});

			Assert::ExpectException<std::out_of_range>([&mesh]()
			{
				mesh.GetTriangle(0);
			});
		}
	};
}