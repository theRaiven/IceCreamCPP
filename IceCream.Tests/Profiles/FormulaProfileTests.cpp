#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Profiles;
using namespace IceCream::Core::Models;

namespace IceCream::Tests::Profiles
{
    TEST_CLASS(FormulaProfileTests)
    {
    public:
        TEST_METHOD(LegacyCoordinates)
        {
            IceCreamParameters parameters{ 3, 1, 0.5, 1.2 };

            auto expression = CompileExpression("x+z+y", parameters);

            Assert::AreEqual(7.0, expression(2, 3), 1e-12);
        }

        TEST_METHOD(LegacyParameters)
        {
            IceCreamParameters parameters{ 3, 1, 0.5, 1.2 };

            auto expression = CompileExpression("R*(z/H)^p", parameters);

            Assert::AreEqual(0.5, expression(0.75, 0), 1e-12);
            Assert::AreEqual(1.0, expression(3, 0), 1e-12);
        }

        TEST_METHOD(DefaultFormula)
        {
            IceCreamParameters parameters{ 3, 1, 0.5, 1.2 };

            FormulaProfile profile(FormulaProfile::DefaultFormula(), parameters);

            Assert::AreEqual(0.0, profile.GetRadius(0), 1e-12);
            Assert::AreEqual(1.0, profile.GetRadius(3), 1e-12);
            Assert::AreEqual(0.0, profile.GetRadius(4.2), 1e-12);
        }
    };
}