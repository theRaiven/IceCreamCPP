#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;

namespace IceCream::Tests::Geometry
{
	TEST_CLASS(Vector3DTests)
	{
	public: 
		TEST_METHOD(Zero_ReturnsZeroVector)
		{
			Vector3D zero = Vector3D::Zero();
			Assert::AreEqual(0.0, zero.X);
			Assert::AreEqual(0.0, zero.Y);
			Assert::AreEqual(0.0, zero.Z);
		}

		TEST_METHOD(UnitVectors_ReturnCorrectVectors)
		{
			Vector3D x = Vector3D::UnitX();
			Vector3D y = Vector3D::UnitY();
			Vector3D z = Vector3D::UnitZ();

			Assert::AreEqual(1.0, x.X);
			Assert::AreEqual(0.0, x.Y);
			Assert::AreEqual(0.0, x.Z);

			Assert::AreEqual(0.0, y.X);
			Assert::AreEqual(1.0, y.Y);
			Assert::AreEqual(0.0, y.Z);

			Assert::AreEqual(0.0, z.X);
			Assert::AreEqual(0.0, z.Y);
			Assert::AreEqual(1.0, z.Z);
		}

		TEST_METHOD(LengthSquared_KnownVector_ReturnsCorrectValue)
		{
			Vector3D vector{ 2.0, 3.0, 6.0 };

			double result = vector.LengthSquared();

			Assert::AreEqual(49.0, result, 1e-12);
		}

		TEST_METHOD(Length_KnownVector_ReturnsCorrectValue)
		{
			Vector3D vector{ 2.0, 3.0, 6.0 };

			double result = vector.Length();

			Assert::AreEqual(7.0, result, 1e-12);
		}

		TEST_METHOD(Length_ZeroVector_Returns0)
		{
			Vector3D vector = Vector3D::Zero();

			double result = vector.Length();

			Assert::AreEqual(0.0, result, 1e-12);
		}

		TEST_METHOD(Length_VeryLargeVector_DoesNotOverflow)
		{
			Vector3D vector{ 1e200, 1e200, 1e200 };

			double result = vector.Length();
			double expected = std::sqrt(3.0) * 1e200;

			Assert::IsTrue(std::isfinite(result));
			Assert::AreEqual(expected, result, expected * 1e-12);
		}

		TEST_METHOD(Length_VerySmallVector_DoesNotUnderflow)
		{
			Vector3D vector{ 1e-200, 1e-200, 1e-200 };

			double result = vector.Length();
			double expected = std::sqrt(3.0) * 1e-200;

			Assert::IsTrue(result > 0.0);
			Assert::AreEqual(expected, result, expected * 1e-12);
		}

		TEST_METHOD(Normalized_KnownVector_ReturnsUnitVector)
		{
			Vector3D vector{ 3.0, 4.0, 0.0 };

			Vector3D result = vector.Normalized();

			Assert::AreEqual(0.6, result.X, 1e-12);
			Assert::AreEqual(0.8, result.Y, 1e-12);
			Assert::AreEqual(0.0, result.Z, 1e-12);
			Assert::AreEqual(1.0, result.Length(), 1e-12);
		}

		TEST_METHOD(Normalized_DoesNotChangeDirection)
		{
			Vector3D vector{ 2.0, -4.0, 6.0 };

			Vector3D result = vector.Normalized();

			Assert::AreEqual(vector.X / vector.Y, result.X / result.Y, 1e-12);
			Assert::AreEqual(vector.Z / vector.Y, result.Z / result.Y, 1e-12);
		}

		TEST_METHOD(Normalized_ZeroVector_ThrowsException)
		{
			Vector3D vector = Vector3D::Zero();

			Assert::ExpectException<std::runtime_error>([&vector]()
			{
				vector.Normalized();
			});
		}

		TEST_METHOD(Normalized_VeryLargeVector_DoesNotThrow)
		{
			Vector3D vector{ 1e200, 1e200, 1e200 };

			Vector3D result = vector.Normalized();

			Assert::AreEqual(1.0, result.Length(), 1e-12);
		}

		TEST_METHOD(Normalized_VerySmallVector_DoesNotThrow)
		{
			Vector3D vector{ 1e-200, 1e-200, 1e-200 };

			Vector3D result = vector.Normalized();

			Assert::AreEqual(1.0, result.Length(), 1e-12);
		}

		TEST_METHOD(Dot_PerpendicularVectors_Returns0)
		{
			Vector3D first = Vector3D::UnitX();
			Vector3D second = Vector3D::UnitY();

			double result = first.Dot(second);

			Assert::AreEqual(0.0, result, 1e-12);
		}

		TEST_METHOD(Dot_KnownVectors_ReturnsCorrectValue)
		{
			Vector3D first{ 1.0, 2.0, 3.0 };
			Vector3D second{ 4.0, 5.0, 6.0 };

			double result = first.Dot(second);

			Assert::AreEqual(32.0, result, 1e-12);
		}

		TEST_METHOD(Dot_IsCommutative)
		{
			Vector3D first{ 2.0, -3.0, 5.0 };
			Vector3D second{ -7.0, 11.0, 4.0 };

			Assert::AreEqual(first.Dot(second),	second.Dot(first), 1e-12);
		}

		TEST_METHOD(Cross_UnitXAndUnitY_ReturnsUnitZ)
		{
			Vector3D result = Vector3D::UnitX().Cross(Vector3D::UnitY());

			Assert::AreEqual(0.0, result.X, 1e-12);
			Assert::AreEqual(0.0, result.Y, 1e-12);
			Assert::AreEqual(1.0, result.Z, 1e-12);
		}

		TEST_METHOD(Cross_ReverseOrder_ReturnsOppositeVector)
		{
			Vector3D first{ 1.0, 2.0, 3.0 };
			Vector3D second{ 4.0, 5.0, 6.0 };

			Vector3D result1 = first.Cross(second);
			Vector3D result2 = second.Cross(first);

			Assert::AreEqual(result1.X, -result2.X, 1e-12);
			Assert::AreEqual(result1.Y, -result2.Y, 1e-12);
			Assert::AreEqual(result1.Z, -result2.Z, 1e-12);
		}

		TEST_METHOD(Cross_ResultIsPerpendicularToBothVectors)
		{
			Vector3D first{ 2.0, 3.0, 4.0 };
			Vector3D second{ -1.0, 5.0, 2.0 };

			Vector3D result = first.Cross(second);

			Assert::AreEqual(0.0, result.Dot(first), 1e-12);
			Assert::AreEqual(0.0, result.Dot(second), 1e-12);
		}

		TEST_METHOD(Add_ReturnsCorrectVector)
		{
			Vector3D first{ 1.0, 2.0, 3.0 };
			Vector3D second{ 4.0, -2.0, 5.0 };

			Vector3D result = first + second;

			Assert::AreEqual(5.0, result.X);
			Assert::AreEqual(0.0, result.Y);
			Assert::AreEqual(8.0, result.Z);
		}

		TEST_METHOD(Subtract_ReturnsCorrectVector)
		{
			Vector3D first{ 5.0, 4.0, 3.0 };
			Vector3D second{ 2.0, -1.0, 5.0 };

			Vector3D result = first - second;

			Assert::AreEqual(3.0, result.X);
			Assert::AreEqual(5.0, result.Y);
			Assert::AreEqual(-2.0, result.Z);
		}

		TEST_METHOD(UnaryMinus_ReturnsOppositeVector)
		{
			Vector3D vector{ 2.0, -3.0, 5.0 };

			Vector3D result = -vector;

			Assert::AreEqual(-2.0, result.X);
			Assert::AreEqual(3.0, result.Y);
			Assert::AreEqual(-5.0, result.Z);
		}

		TEST_METHOD(MultiplyByScalar_ReturnsCorrectVector)
		{
			Vector3D vector{ 1.0, -2.0, 3.0 };

			Vector3D result = vector * 4.0;

			Assert::AreEqual(4.0, result.X);
			Assert::AreEqual(-8.0, result.Y);
			Assert::AreEqual(12.0, result.Z);
		}

		TEST_METHOD(ScalarMultiplyVector_ReturnsSameResult)
		{
			Vector3D vector{ 1.0, -2.0, 3.0 };

			Vector3D result1 = vector * 4.0;
			Vector3D result2 = 4.0 * vector;

			Assert::AreEqual(result1.X, result2.X);
			Assert::AreEqual(result1.Y, result2.Y);
			Assert::AreEqual(result1.Z, result2.Z);
		}

		TEST_METHOD(DivideByScalar_ReturnsCorrectVector)
		{
			Vector3D vector{ 8.0, -4.0, 12.0 };

			Vector3D result = vector / 4.0;

			Assert::AreEqual(2.0, result.X);
			Assert::AreEqual(-1.0, result.Y);
			Assert::AreEqual(3.0, result.Z);
		}

		TEST_METHOD(DivideByZero_ThrowsException)
		{
			Vector3D vector{ 1.0, 2.0, 3.0 };

			Assert::ExpectException<std::invalid_argument>([&vector]()
				{
					auto result = vector / 0.0;
				});
		}
	};
}