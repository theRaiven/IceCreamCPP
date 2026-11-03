#include "../pch.h"

#include <functional>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Profiles;
using namespace IceCream::Core::Services;

namespace IceCream::Tests::Services
{
	class TestProfile final : public IRadialProfile
	{
	private:
		double minZ_;
		double maxZ_;
		std::function<double(double)> radius_;

	public:
		TestProfile(double minZ, double maxZ, std::function<double(double)> radius)
			: minZ_(minZ), maxZ_(maxZ), radius_(std::move(radius))
		{
		}

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

	TEST_CLASS(VolumeCalculatorTests)
	{
	public:
		TEST_METHOD(Calculate_Cylinder_ReturnsCorrectVolume)
		{
			TestProfile profile(0.0, 2.0, [](double) { return 3.0; });
			VolumeCalculator calculator;

			double result = calculator.Calculate(profile, 100);
			double expected = 18.0 * std::acos(-1.0);

			Assert::AreEqual(expected, result, 1e-10);
		}

		TEST_METHOD(Calculate_ThreePartCompositeProfile_ReturnsCorrectVolume)
		{
			auto first = std::make_shared<TestProfile>(0.0, 1.0, [](double z) { return z + 1.0; });
			auto second = std::make_shared<TestProfile>(1.0, 2.0, [](double z) { return 3.0 - z; });
			auto third = std::make_shared<TestProfile>(2.0, 3.0, [](double z) { return z - 1.0; });

			CompositeProfile profile({ first, second, third });
			VolumeCalculator calculator;

			double result = calculator.Calculate(profile, 300);
			double expected = 7.0 * std::acos(-1.0);

			Assert::AreEqual(expected, result, 1e-10);
		}

		TEST_METHOD(Calculate_IntervalsLessThanTwo_ThrowsException)
		{
			TestProfile profile(0.0, 1.0, [](double) { return 1.0; });
			VolumeCalculator calculator;

			Assert::ExpectException<std::out_of_range>([&]()
				{
					calculator.Calculate(profile, 0);
				});
		}

		TEST_METHOD(Calculate_OddIntervals_ThrowsException)
		{
			TestProfile profile(0.0, 1.0, [](double) { return 1.0; });
			VolumeCalculator calculator;

			Assert::ExpectException<std::out_of_range>([&]()
				{
					calculator.Calculate(profile, 99);
				});
		}

		TEST_METHOD(Calculate_NegativeRadius_ThrowsException)
		{
			TestProfile profile(0.0, 1.0, [](double) { return -1.0; });
			VolumeCalculator calculator;

			Assert::ExpectException<std::runtime_error>([&]()
				{
					calculator.Calculate(profile, 100);
				});
		}

		TEST_METHOD(Calculate_InfiniteRadius_ThrowsException)
		{
			TestProfile profile(0.0, 1.0, [](double)
				{
					return std::numeric_limits<double>::infinity();
				});

			VolumeCalculator calculator;

			Assert::ExpectException<std::runtime_error>([&]()
				{
					calculator.Calculate(profile, 100);
				});
		}
	};
}