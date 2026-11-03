#include "../pch.h"

#include <functional>
#include <limits>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Profiles;

namespace IceCream::Tests::Profiles
{
	class TestProfile final : public IRadialProfile // вчера узнал, что final в c++ это как sealed в c#
	{
	private:
		double minZ_;
		double maxZ_;
		std::function<double(double)> radius_;

	public:
		TestProfile(double minZ, double maxZ, std::function<double(double)> radius)	: minZ_(minZ), maxZ_(maxZ), radius_(std::move(radius))	{}

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

	TEST_CLASS(CompositeProfileTests)
	{
	private:
		static std::shared_ptr<const IRadialProfile> CreateFirst()
		{
			// [0; 1], r = z + 1
			// На стыке z = 1: r = 2
			return std::make_shared<TestProfile>(0.0, 1.0,
			[](double z)
			{
				return z + 1.0;
			});
		}

		static std::shared_ptr<const IRadialProfile> CreateSecond()
		{
			// [1; 2], r = 2z
			// r(1) = 2, r(2) = 4
			return std::make_shared<TestProfile>(1.0, 2.0,
			[](double z)
			{
				return 2.0 * z;
			});
		}

		static std::shared_ptr<const IRadialProfile> CreateThird()
		{
			// [2; 3], r = z + 2
			// На стыке z = 2: r = 4
			return std::make_shared<TestProfile>(2.0, 3.0,
			[](double z)
			{
				return z + 2.0;
			});
		}

	public:
		TEST_METHOD(Constructor_EmptyProfiles_ThrowsException)
		{
			Assert::ExpectException<std::invalid_argument>([]()
				{
					CompositeProfile profile({});
				});
		}

		TEST_METHOD(Constructor_NullProfile_ThrowsException)
		{
			std::vector<std::shared_ptr<const IRadialProfile>> profiles
			{
				CreateFirst(),
				nullptr
			};

			Assert::ExpectException<std::invalid_argument>([&profiles]()
				{
					CompositeProfile profile(profiles);
				});
		}

		TEST_METHOD(Constructor_SingleProfile_Works)
		{
			CompositeProfile profile({CreateFirst()});

			Assert::AreEqual(size_t(1), profile.ProfileCount());
			Assert::AreEqual(0.0, profile.MinZ());
			Assert::AreEqual(1.0, profile.MaxZ());
		}

		TEST_METHOD(Constructor_ThreeConnectedProfiles_Works)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond(), CreateThird()});

			Assert::AreEqual(size_t(3), profile.ProfileCount());
			Assert::AreEqual(0.0, profile.MinZ());
			Assert::AreEqual(3.0, profile.MaxZ());
		}

		TEST_METHOD(Constructor_ProfilesWithGap_ThrowsException)
		{
			auto first = CreateFirst();

			auto second = std::make_shared<TestProfile>(1.5, 2.5,
			[](double z)
			{
				return z + 0.5;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					CompositeProfile profile({first, second});
				});
		}

		TEST_METHOD(Constructor_ProfilesWithOverlap_ThrowsException)
		{
			auto first = CreateFirst();

			auto second = std::make_shared<TestProfile>(0.5, 2.0,
			[](double z)
			{
				return z + 1.5;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					CompositeProfile profile({first, second});
				});
		}

		TEST_METHOD(Constructor_DifferentRadiiAtJoin_ThrowsException)
		{
			auto first = CreateFirst();

			auto second = std::make_shared<TestProfile>(1.0, 2.0,
			[](double z)
			{
				return z + 10.0;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					CompositeProfile profile({first, second});
				});
		}

		TEST_METHOD(Constructor_ZeroRadiusAtJoin_ThrowsException)
		{
			auto first = std::make_shared<TestProfile>(	0.0,1.0,
			[](double z)
			{
				return 1.0 - z;
			});

			auto second = std::make_shared<TestProfile>(1.0, 2.0,
			[](double z)
			{
				return z - 1.0;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					CompositeProfile profile({first, second});
				});
		}

		TEST_METHOD(Constructor_InvalidProfileRange_ThrowsException)
		{
			auto invalid = std::make_shared<TestProfile>(2.0, 1.0,
			[](double)
			{
				return 1.0;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
				{
					CompositeProfile profile({invalid});
				});
		}

		TEST_METHOD(Constructor_InfiniteProfileRange_ThrowsException)
		{
			double inf = std::numeric_limits<double>::infinity();

			auto invalid = std::make_shared<TestProfile>(0.0, inf,
			[](double)
			{
				return 1.0;
			});

			Assert::ExpectException<std::invalid_argument>([&]()
			{
				CompositeProfile profile({invalid});
			});
		}

		TEST_METHOD(Profile_ReturnsCorrectPart)
		{
			auto first = CreateFirst();
			auto second = CreateSecond();

			CompositeProfile profile({first, second});

			Assert::IsTrue(&profile.Profile(0) == first.get());
			Assert::IsTrue(&profile.Profile(1) == second.get());
		}

		TEST_METHOD(Profile_InvalidIndex_ThrowsException)
		{
			CompositeProfile profile({CreateFirst()});

			Assert::ExpectException<std::out_of_range>([&profile]()
				{
					profile.Profile(1);
				});
		}

		TEST_METHOD(GetRadius_SelectsCorrectProfile)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond(), CreateThird()});

			Assert::AreEqual(1.5, profile.GetRadius(0.5), 1e-12);
			Assert::AreEqual(3.0, profile.GetRadius(1.5), 1e-12);
			Assert::AreEqual(4.5, profile.GetRadius(2.5), 1e-12);
		}

		TEST_METHOD(GetRadius_AtFirstBoundary_ReturnsCorrectRadius)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond()});

			Assert::AreEqual(1.0, profile.GetRadius(0.0), 1e-12);
		}

		TEST_METHOD(GetRadius_AtJoin_ReturnsCorrectRadius)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond()});

			Assert::AreEqual(2.0, profile.GetRadius(1.0), 1e-12);
		}

		TEST_METHOD(GetRadius_AtLastBoundary_ReturnsCorrectRadius)
		{
			CompositeProfile profile({CreateFirst(),CreateSecond(),	CreateThird()});

			Assert::AreEqual(5.0, profile.GetRadius(3.0), 1e-12);
		}

		TEST_METHOD(GetRadius_BelowMinimum_ThrowsException)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond()});

			Assert::ExpectException<std::out_of_range>([&profile]()
				{
					profile.GetRadius(-0.1);
				});
		}

		TEST_METHOD(GetRadius_AboveMaximum_ThrowsException)
		{
			CompositeProfile profile({CreateFirst(), CreateSecond()});

			Assert::ExpectException<std::out_of_range>([&profile]()
				{
					profile.GetRadius(2.1);
				});
		}

		TEST_METHOD(GetRadius_NaN_ThrowsException)
		{
			CompositeProfile profile({CreateFirst()});

			double nan = std::numeric_limits<double>::quiet_NaN();

			Assert::ExpectException<std::out_of_range>([&]()
				{
					profile.GetRadius(nan);
				});
		}

		TEST_METHOD(GetRadius_Infinity_ThrowsException)
		{
			CompositeProfile profile({CreateFirst()});

			double inf = std::numeric_limits<double>::infinity();

			Assert::ExpectException<std::out_of_range>([&]()
				{
					profile.GetRadius(inf);
				});
		}
	};
}