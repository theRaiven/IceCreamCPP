#include "../pch.h"
#include "../../IceCream.Core/Expressions/Parsing/ExpressionParser.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Expressions;

namespace IceCream::Tests::Expressions
{
    TEST_CLASS(ExpressionParserTests)
    {
    public:
        TEST_METHOD(IndependentCoordinates)
        {
            auto expression = ExpressionParser::Compile("x^2+y^2-z");

            Assert::AreEqual(9.0, expression(2, 3, 4), 1e-12);
            Assert::AreEqual(14.0, expression(1, 4, 3), 1e-12);
        }

        TEST_METHOD(NamedConstants)
        {
            auto expression = ExpressionParser::Compile("a*x+b*y", {
                {"a", 2.0},
                {"b", 3.0}
            });

            Assert::AreEqual(23.0, expression(4, 5, 0), 1e-12);
        }

        TEST_METHOD(NestedFunctions)
        {
            auto expression = ExpressionParser::Compile("if(x>0,sqrt(x),abs(x))");

            Assert::AreEqual(2.0, expression(4, 0, 0), 1e-12);
            Assert::AreEqual(3.0, expression(-3, 0, 0), 1e-12);
        }

        TEST_METHOD(InvalidExpression)
        {
            Assert::ExpectException<std::invalid_argument>([]()
            {
                ExpressionParser::Compile("x+");
            });
        }
    };
}