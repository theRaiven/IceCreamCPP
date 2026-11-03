#include "../pch.h"

#include <cmath>
#include <functional>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;
using namespace IceCream::Core::Profiles;
using namespace IceCream::Core::Services;

namespace IceCream::Tests::Services
{
	class TestRadialProfile final : public IRadialProfile
	{
	private:
		double minZ_;
		double maxZ_;
		std::function<double(double)> radius_;

	public:
		TestRadialProfile(double minZ, double maxZ,	std::function<double(double)> radius) : minZ_(minZ), maxZ_(maxZ), radius_(std::move(radius)) {}

		double MinZ() const override
		{
			return minZ_;
		}

		double MaxZ() const override
		{
			return maxZ_;
		}

		double GetRadius(double z) const override
		{
			return radius_(z);
		}
	};

	TEST_CLASS(SurfaceMeshBuilderTests)
	{
	private:
		static CompositeProfile CreateThreePartProfile()
		{
			// [0; 1]   r = z + 1
			// r(1) = 2
			auto first = std::make_shared<TestRadialProfile>(0.0, 1.0,
				[](double z)
				{
					return z + 1.0;
				});

			// [1; 2]   r = 2z
			// r(1) = 2, r(2) = 4
			auto second = std::make_shared<TestRadialProfile>(1.0, 2.0,
				[](double z)
				{
					return 2.0 * z;
				});

			// [2; 3]   r = 10 - 3z
			// r(2) = 4, r(3) = 1
			auto third = std::make_shared<TestRadialProfile>(2.0, 3.0,
				[](double z)
				{
					return 10.0 - 3.0 * z;
				});

			return CompositeProfile({first, second, third});
		}

		static void AssertRingRadius(const TriangleMesh& mesh, double z, double expectedRadius,	int expectedVertices)
		{
			int count = 0;

			for (const auto& vertex : mesh.Vertices())
			{
				if (std::abs(vertex.Z - z) > 1e-12)	continue;

				double radius = std::hypot(vertex.X, vertex.Y);

				Assert::AreEqual(expectedRadius, radius, 1e-10);

				count++;
			}

			Assert::AreEqual(expectedVertices, count);
		}

	public:
		TEST_METHOD(Build_ThreePartCompositeProfile_CreatesMesh)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			TriangleMesh mesh = builder.Build(profile, 6, 8);

			Assert::IsTrue(mesh.Vertices().size() > 0);
			Assert::IsTrue(mesh.Indices().size() > 0);
			Assert::IsTrue(mesh.TriangleCount() > 0);
		}

		TEST_METHOD(Build_ThreePartCompositeProfile_CreatesExpectedNumberOfTriangles)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			TriangleMesh mesh = builder.Build(profile,6,8);

			Assert::AreEqual(size_t(112), mesh.TriangleCount());
		}

		TEST_METHOD(Build_ThreePartCompositeProfile_UsesAllProfiles)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			TriangleMesh mesh = builder.Build(profile, 6, 8);

			AssertRingRadius(mesh, 0.5, 1.5, 8);
			AssertRingRadius(mesh, 1.5, 3.0, 8);
			AssertRingRadius(mesh, 2.5, 2.5, 8);
		}

		TEST_METHOD(Build_ThreePartCompositeProfile_IncludesJoinRings)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			TriangleMesh mesh = builder.Build(profile, 6, 8);

			AssertRingRadius(mesh, 1.0, 2.0, 8);
			AssertRingRadius(mesh, 2.0, 4.0, 8);
		}

		TEST_METHOD(Build_InvalidHeightSegments_ThrowsException)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					builder.Build(profile, 1, 8);
				});
		}

		TEST_METHOD(Build_InvalidRadialSegments_ThrowsException)
		{
			CompositeProfile profile = CreateThreePartProfile();
			SurfaceMeshBuilder builder;

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					builder.Build(profile, 6, 2);
				});
		}
	};
}