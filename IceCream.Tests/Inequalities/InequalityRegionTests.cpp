
#include "../pch.h"
#include "../../IceCream.Core/Expressions/Inequalities/InequalityRegion.h"
#include "../../IceCream.Core/Expressions/Inequalities/InequalityParser.h"

#include <stdexcept>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Inequalities;

namespace IceCream::Tests::Inequalities
{
    TEST_CLASS(InequalityRegionTests)
    {
    public:
        TEST_METHOD(InsideRegion)
        {
            auto inequalities = InequalityParser::Parse("0<=z<=4-x, y^2<=2*x+2");
            InequalityRegion region(move(inequalities));

            Assert::IsTrue(region.Contains(0, 0, 2));
            Assert::IsTrue(region.Contains(0, 1, 2));
        }

        TEST_METHOD(OutsideRegion)
        {
            auto inequalities = InequalityParser::Parse("0<=z<=4-x, y^2<=2*x+2");
            InequalityRegion region(move(inequalities));

            Assert::IsFalse(region.Contains(0, 0, -1));
            Assert::IsFalse(region.Contains(0, 0, 5));
            Assert::IsFalse(region.Contains(0, 3, 2));
        }

        TEST_METHOD(BoundaryRegion)
        {
            auto inequalities = InequalityParser::Parse("0<=z<=4-x, y^2<=2*x+2");
            InequalityRegion region(move(inequalities));

            Assert::IsTrue(region.Contains(0, 0, 0));
            Assert::IsTrue(region.Contains(0, 0, 4));
        }

        TEST_METHOD(EmptyRegion)
        {
            Assert::ExpectException<invalid_argument>([]()
                {
                    InequalityRegion region({});
                });
        }

        TEST_METHOD(StrictBoundary)
        {
            auto inequalities = InequalityParser::Parse("0<z<4");
            InequalityRegion region(move(inequalities));

            Assert::IsFalse(region.Contains(0, 0, 0));
            Assert::IsTrue(region.Contains(0, 0, 2));
            Assert::IsFalse(region.Contains(0, 0, 4));
        }


        TEST_METHOD(EvaluateRegion)
        {
            auto inequalities = InequalityParser::Parse("0<=z<=4-x, y^2<=2*x+2");
            InequalityRegion region(move(inequalities));

            Assert::AreEqual(-2.0, region.Evaluate(0, 0, 2), 1e-9);
            Assert::AreEqual(7.0, region.Evaluate(0, 3, 2), 1e-9);
            Assert::AreEqual(0.0, region.Evaluate(0, 0, 0), 1e-9);
            Assert::AreEqual(0.0, region.Evaluate(0, 0, 4), 1e-9);
        }

    };
}
