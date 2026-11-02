#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Geometry;

namespace IceCream::Tests::Geometry
{
    TEST_CLASS(Point2DTests)
    {
    public:
        TEST_METHOD(DistanceTo_Points3And4Apart_Returns5)
        {
            Point2D first{ 0.0, 0.0 };
            Point2D second{ 3.0, 4.0 };

            double result = first.DistanceTo(second);

            Assert::AreEqual(5.0, result, 1e-12);
        }

        TEST_METHOD(DistanceTo_SamePoint_Returns0)
        {
            Point2D point{ 2.0, -3.0 };

            double result = point.DistanceTo(point);

            Assert::AreEqual(0.0, result, 1e-12);
        }

        TEST_METHOD(DistanceTo_IsSymmetric)
        {
            Point2D first{ -2.0, 5.0 };
            Point2D second{ 4.0, -1.0 };

            double firstToSecond = first.DistanceTo(second);
            double secondToFirst = second.DistanceTo(first);

            Assert::AreEqual(firstToSecond, secondToFirst, 1e-12);
        }

        TEST_METHOD(DistanceTo_NegativeCoordinates_ReturnsCorrectDistance)
        {
            Point2D first{ -1.0, -2.0 };
            Point2D second{ -4.0, -6.0 };

            double result = first.DistanceTo(second);

            Assert::AreEqual(5.0, result, 1e-12);
        }

        TEST_METHOD(DistanceTo_VeryLargeCoordinates_DoesNotOverflow)
        {
            Point2D first{ 0.0, 0.0 };
            Point2D second{ 1e308, 0.0 };

            double result = first.DistanceTo(second);

            Assert::IsTrue(std::isfinite(result));
            Assert::AreEqual(1e308, result, 1e296);
        }

        TEST_METHOD(DistanceTo_VerySmallCoordinates_DoesNotUnderflow)
        {
            Point2D first{ 0.0, 0.0 };
            Point2D second{ 1e-200, 0.0 };

            double result = first.DistanceTo(second);

            Assert::IsTrue(result > 0.0);
            Assert::AreEqual(1e-200, result, 1e-212);
        }

        TEST_METHOD(DistanceTo_LargeCoordinatesOnBothAxes_DoesNotOverflow)
        {
            Point2D first{ 0.0, 0.0 };
            Point2D second{ 1e200, 1e200 };

            double result = first.DistanceTo(second);

            double expected = std::sqrt(2.0) * 1e200;

            Assert::IsTrue(std::isfinite(result));
            Assert::AreEqual(expected, result, expected * 1e-12);
        }
    };
}