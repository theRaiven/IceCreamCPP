#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;

namespace IceCream::Tests::Geometry
{
	TEST_CLASS(Plane3DTests)
	{
	public:
		TEST_METHOD(Constructor_NormalizesNormal)
		{
			Plane3D plane
			{
				{ 1.0, 2.0, 3.0 },
				{ 0.0, 0.0, 5.0 }
			};

			Assert::AreEqual(0.0, plane.Normal().X, 1e-12);
			Assert::AreEqual(0.0, plane.Normal().Y, 1e-12);
			Assert::AreEqual(1.0, plane.Normal().Z, 1e-12);
			Assert::AreEqual(1.0, plane.Normal().Length(), 1e-12);
		}

		TEST_METHOD(Constructor_PreservesOrigin)
		{
			Plane3D plane
			{
				{ 1.0, -2.0, 3.0 },
				{ 0.0, 0.0, 1.0 }
			};

			Assert::AreEqual(1.0, plane.Origin().X);
			Assert::AreEqual(-2.0, plane.Origin().Y);
			Assert::AreEqual(3.0, plane.Origin().Z);
		}

		TEST_METHOD(Constructor_ZeroNormal_ThrowsException)
		{
			Assert::ExpectException<std::runtime_error>([]()
			{
				Plane3D plane
				{
					{ 0.0, 0.0, 0.0 },
					{ 0.0, 0.0, 0.0 }
				};
			});
		}

		TEST_METHOD(Constructor_InfiniteOrigin_ThrowsException)
		{
			double inf = std::numeric_limits<double>::infinity();

			Assert::ExpectException<std::invalid_argument>([inf]()
			{
				Plane3D plane
				{
					{ inf, 0.0, 0.0 },
					{ 0.0, 0.0, 1.0 }
				};
			});
		}
		
		TEST_METHOD(Constructor_NaNOrigin_ThrowsException)
		{
			double nan = std::numeric_limits<double>::quiet_NaN();

			Assert::ExpectException<std::invalid_argument>([nan]()
			{
				Plane3D plane
				{
					{ 0.0, nan, 0.0 },
					{ 0.0, 0.0, 1.0 }
				};
			});
		}

		TEST_METHOD(Axes_AreUnitVectors)
		{
			Plane3D plane
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 2.0, 3.0 }
			};

			Assert::AreEqual(1.0, plane.AxisU().Length(), 1e-12);
			Assert::AreEqual(1.0, plane.AxisV().Length(), 1e-12);
		}

		TEST_METHOD(Axes_ArePerpendicularToNormalAndEachOther)
		{
			Plane3D plane
			{
				{ 0.0, 0.0, 0.0 },
				{ 1.0, 2.0, 3.0 }
			};

			Assert::AreEqual(0.0, plane.AxisU().Dot(plane.Normal()), 1e-12);
			Assert::AreEqual(0.0, plane.AxisV().Dot(plane.Normal()), 1e-12);
			Assert::AreEqual(0.0, plane.AxisU().Dot(plane.AxisV()), 1e-12);
		}

		TEST_METHOD(SignedDistanceTo_PointOnPlane_Returns0)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point3D point{ 12.0, -7.0, 5.0 };

			double result = plane.SignedDistanceTo(point);

			Assert::AreEqual(0.0, result, 1e-12);
		}
		
		TEST_METHOD(SignedDistanceTo_PointAlongNormal_ReturnsPositiveDistance)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point3D point{ 0.0, 0.0, 8.0 };

			double result = plane.SignedDistanceTo(point);

			Assert::AreEqual(3.0, result, 1e-12);
		}
	
		TEST_METHOD(SignedDistanceTo_PointOppositeNormal_ReturnsNegativeDistance)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point3D point{ 0.0, 0.0, 1.0 };

			double result = plane.SignedDistanceTo(point);

			Assert::AreEqual(-4.0, result, 1e-12);
		}

		TEST_METHOD(Project_PointOntoPlane_ReturnsCorrectPoint)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point3D point{ 2.0, -3.0, 12.0 };

			Point3D result = plane.Project(point);

			Assert::AreEqual(2.0, result.X, 1e-12);
			Assert::AreEqual(-3.0, result.Y, 1e-12);
			Assert::AreEqual(5.0, result.Z, 1e-12);
		}

		TEST_METHOD(Project_ResultLiesOnPlane)
		{
			Plane3D plane
			{
				{ 1.0, 2.0, 3.0 },
				{ 2.0, -1.0, 4.0 }
			};

			Point3D point{ 10.0, -20.0, 7.0 };

			Point3D projected = plane.Project(point);

			Assert::AreEqual(
				0.0,
				plane.SignedDistanceTo(projected),
				1e-12
			);
		}


		TEST_METHOD(AtX_ReturnsPlaneAtGivenX)
		{
			Plane3D plane = Plane3D::AtX(4.0);

			Assert::AreEqual(4.0, plane.Origin().X);
			Assert::AreEqual(0.0, plane.Origin().Y);
			Assert::AreEqual(0.0, plane.Origin().Z);

			Assert::AreEqual(1.0, plane.Normal().X);
			Assert::AreEqual(0.0, plane.Normal().Y);
			Assert::AreEqual(0.0, plane.Normal().Z);
		}

		TEST_METHOD(AtY_ReturnsPlaneAtGivenY)
		{
			Plane3D plane = Plane3D::AtY(-3.0);

			Assert::AreEqual(-3.0, plane.Origin().Y);
			Assert::AreEqual(1.0, plane.Normal().Y);
		}

		TEST_METHOD(AtZ_ReturnsPlaneAtGivenZ)
		{
			Plane3D plane = Plane3D::AtZ(7.0);

			Assert::AreEqual(7.0, plane.Origin().Z);
			Assert::AreEqual(1.0, plane.Normal().Z);
		}

		TEST_METHOD(ToLocal_AtZPlane_ReturnsExpectedCoordinates)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point3D point{ 3.0, 4.0, 5.0 };

			Point2D result = plane.ToLocal(point);

			Assert::AreEqual(3.0, result.X, 1e-12);
			Assert::AreEqual(4.0, result.Y, 1e-12);
		}

		TEST_METHOD(ToGlobal_AtZPlane_ReturnsExpectedPoint)
		{
			Plane3D plane = Plane3D::AtZ(5.0);
			Point2D point{ 3.0, 4.0 };

			Point3D result = plane.ToGlobal(point);

			Assert::AreEqual(3.0, result.X, 1e-12);
			Assert::AreEqual(4.0, result.Y, 1e-12);
			Assert::AreEqual(5.0, result.Z, 1e-12);
		}

		TEST_METHOD(ToLocalThenToGlobal_PointOnPlane_ReturnsOriginalPoint)
		{
			Plane3D plane
			{
				{ 1.0, 2.0, 3.0 },
				{ 2.0, -1.0, 4.0 }
			};

			Point3D point =
				plane.Origin()
				+ plane.AxisU() * 3.5
				+ plane.AxisV() * -2.25;

			Point2D local = plane.ToLocal(point);
			Point3D result = plane.ToGlobal(local);

			Assert::AreEqual(point.X, result.X, 1e-12);
			Assert::AreEqual(point.Y, result.Y, 1e-12);
			Assert::AreEqual(point.Z, result.Z, 1e-12);
		}

		TEST_METHOD(ToLocal_PointOutsidePlane_IgnoresNormalComponent)
		{
			Plane3D plane = Plane3D::AtZ(5.0);

			Point2D first = plane.ToLocal({ 3.0, 4.0, 5.0 });
			Point2D second = plane.ToLocal({ 3.0, 4.0, 100.0 });

			Assert::AreEqual(first.X, second.X, 1e-12);
			Assert::AreEqual(first.Y, second.Y, 1e-12);
		}
	};
}