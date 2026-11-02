#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;

namespace IceCream::Tests::Geometry
{
	TEST_CLASS(Point3DTests)
	{
	public:
		TEST_METHOD(DistanceTo_Points3And4Apart_Returns5)
		{
			Point3D first{ 0.0, 0.0, 0.0 };
			Point3D second{ 3.0, 4.0, 0.0 };
			double result = first.DistanceTo(second);
			Assert::AreEqual(5.0, result, 1e-12);
		}

		TEST_METHOD(DistanceTo_SamePoint_Returns0)
		{
			Point3D point{ 2.0, -3.0, 1.0 };
			double result = point.DistanceTo(point);
			Assert::AreEqual(0.0, result, 1e-12);
		}

		TEST_METHOD(DistanceTo_IsSymmetric)
		{
			Point3D first{ 1.0, 2.0, 3.0 };
			Point3D second{ 4.0, 5.0, 6.0 };

			double result1 = first.DistanceTo(second);
			double result2 = second.DistanceTo(first);

			Assert::AreEqual(result1, result2, 1e-12);
		}

		TEST_METHOD(DistanceTo_NegativeCoordinates_ReturnsCorrectDistance)
		{
			Point3D first{ -1.0, -2.0, -3.0 };
			Point3D second{ -4.0, -5.0, -6.0 };

			double result = first.DistanceTo(second);
			Assert::AreEqual(5.196152422706632, result, 1e-12);
		}

		TEST_METHOD(DistanceTo_VeryLargeCoordinates_DoesNotOverflow)
		{
			Point3D first{ 0.0, 0.0, 0.0 };
			Point3D second{ 1e308, 1e308, 1e308 };

			double result = first.DistanceTo(second);

			Assert::IsTrue(std::isfinite(result));
			Assert::AreEqual(1.7320508075688772e308, result, 1e296);
		}

		TEST_METHOD(DistanceTo_VerySmallCoordinates_DoesNotUnderflow)
		{
			Point3D first{ 0.0, 0.0, 0.0 };
			Point3D second{ 1e-200, 1e-200, 1e-200 };

			double result = first.DistanceTo(second);
			double expected = std::sqrt(3.0) * 1e-200;

			Assert::IsTrue(result > 0.0);
			Assert::AreEqual(expected, result, expected * 1e-12);
		}

		TEST_METHOD(DistanceTo_LargeCoordinatesOnAllAxes_DoesNotOverflow)
		{
			Point3D first{ 0.0, 0.0, 0.0 };
			Point3D second{ 1e200, 1e200, 1e200 };

			double result = first.DistanceTo(second);

			double expected = std::sqrt(3.0) * 1e200;

			Assert::IsTrue(std::isfinite(result));
			Assert::AreEqual(expected, result, expected * 1e-12);
		}

		TEST_METHOD(SubtractPoints_ReturnsCorrectVector)
		{
			Point3D first{ 5.0, 7.0, 9.0 };
			Point3D second{ 2.0, 3.0, 4.0 };

			Vector3D result = first - second;

			Assert::AreEqual(3.0, result.X);
			Assert::AreEqual(4.0, result.Y);
			Assert::AreEqual(5.0, result.Z);
		}

		TEST_METHOD(AddVector_ReturnsCorrectPoint)
		{
			Point3D point{ 1.0, 2.0, 3.0 };
			Vector3D vector{ 4.0, -2.0, 5.0 };

			Point3D result = point + vector;

			Assert::AreEqual(5.0, result.X);
			Assert::AreEqual(0.0, result.Y);
			Assert::AreEqual(8.0, result.Z);
		}

		TEST_METHOD(SubtractVector_ReturnsCorrectPoint)
		{
			Point3D point{ 5.0, 4.0, 3.0 };
			Vector3D vector{ 2.0, -1.0, 5.0 };

			Point3D result = point - vector;

			Assert::AreEqual(3.0, result.X);
			Assert::AreEqual(5.0, result.Y);
			Assert::AreEqual(-2.0, result.Z);
		}

		TEST_METHOD(AddThenSubtractSameVector_ReturnsOriginalPoint)
		{
			Point3D point{ 3.0, -7.0, 11.0 };
			Vector3D vector{ 5.0, 2.0, -4.0 };

			Point3D result = (point + vector) - vector;

			Assert::AreEqual(point.X, result.X);
			Assert::AreEqual(point.Y, result.Y);
			Assert::AreEqual(point.Z, result.Z);
		}

		TEST_METHOD(SubtractThenAddSameVector_ReturnsOriginalPoint)
		{
			Point3D point{ -4.0, 8.0, 2.0 };
			Vector3D vector{ 3.0, -5.0, 7.0 };

			Point3D result = (point - vector) + vector;

			Assert::AreEqual(point.X, result.X);
			Assert::AreEqual(point.Y, result.Y);
			Assert::AreEqual(point.Z, result.Z);
		}
	};
}