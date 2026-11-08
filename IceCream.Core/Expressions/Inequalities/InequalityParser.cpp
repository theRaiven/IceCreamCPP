#include "InequalityParser.h"

#include <stdexcept>
#include <string>
#include <vector>
using namespace std;

namespace IceCream::Core::Inequalities
{
    namespace
    {
        enum class ComparisonOperator
        {
			Less,          // g<0
			LessOrEqual,   // g<=0
			Greater,       // g>0
			GreaterOrEqual // g>=0
        };

        struct ComparisonParts
        {
            vector<string> expressions;
            vector<ComparisonOperator> operators;
        };

        string Trim(const string& text)
        {
            size_t first = text.find_first_not_of(" \t\n\r");

            if (first == string::npos) return "";

            size_t last = text.find_last_not_of(" \t\n\r");

            return text.substr(first, last - first + 1);
        }

        vector<string> SplitConditions(const string& text)
        {
            vector<string> conditions;

            size_t start = 0;
            int depth = 0;

            for (size_t i = 0; i < text.size(); ++i)
            {
                char c = text[i];

                if (c == '(')
                {
                    ++depth;
                }
                else if (c == ')')
                {
                    --depth;

                    if (depth < 0)
                    {
                        throw invalid_argument("Неожиданная закрывающая скобка");
                    }
                }
                else if (c == ',' && depth == 0)
                {
                    string condition = Trim(text.substr(start, i - start));

                    if (condition.empty())
                    {
                        throw invalid_argument("Пустое неравенство");
                    }

                    conditions.push_back(condition);
                    start = i + 1;
                }
            }

            if (depth != 0)
            {
                throw invalid_argument("Незакрытая скобка");
            }

            string condition = Trim(text.substr(start));

            if (condition.empty())
            {
                throw invalid_argument("Пустое неравенство");
            }

            conditions.push_back(condition);

            return conditions;
        }

        ComparisonParts SplitComparisons(const string& text)
        {
            ComparisonParts result;

            size_t start = 0;
            int depth = 0;

            for (size_t i = 0; i < text.size(); ++i)
            {
                char c = text[i];

                if (c == '(')
                {
                    ++depth;
                }
                else if (c == ')')
                {
                    --depth;
                }
                else if ((c == '<' || c == '>') && depth == 0)
                {
                    string expression = Trim(text.substr(start, i - start));

                    if (expression.empty())
                    {
                        throw invalid_argument("Пустое выражение в неравенстве");
                    }

                    result.expressions.push_back(expression);

                    bool equal = (i + 1 < text.size() && text[i + 1] == '=');

                    if (c == '<')
                    {
                        result.operators.push_back(equal ? ComparisonOperator::LessOrEqual : ComparisonOperator::Less);
                    }
                    else
                    {
                        result.operators.push_back(equal ? ComparisonOperator::GreaterOrEqual : ComparisonOperator::Greater);
                    }

                    if (equal) ++i;

                    start = i + 1;
                }
                else if (c == '=' && depth == 0)
                {
                    throw invalid_argument("Непредвиденный '=' в неравенстве");
                }
            }

            string expression = Trim(text.substr(start));

            if (expression.empty())
            {
                throw invalid_argument("Пустое выражение в неравенстве");
            }

            result.expressions.push_back(expression);

            if (result.operators.empty())
            {
                throw invalid_argument("Отсутствующий оператор сравнения");
            }

            return result;
        }

    }


    vector<Inequality> InequalityParser::Parse(const string& text, const Expressions::ExpressionParser::Constants& constants)
    {
        vector<Inequality> result;

        for (const string& condition : SplitConditions(text))
        {
            ComparisonParts parts = SplitComparisons(condition);

            vector<Expressions::CompiledExpression> expressions;

            for (const string& expression : parts.expressions)
            {
                expressions.push_back(Expressions::ExpressionParser::Compile(expression, constants));
            }

            for (size_t i = 0; i < parts.operators.size(); ++i)
            {
                auto left = expressions[i];
                auto right = expressions[i + 1];

                ComparisonOperator operation = parts.operators[i];

                bool reversed = operation == ComparisonOperator::Greater || operation == ComparisonOperator::GreaterOrEqual;

                InequalityRelation relation = operation == ComparisonOperator::Less || operation == ComparisonOperator::Greater
                    ? InequalityRelation::Less : InequalityRelation::LessOrEqual;

                Expressions::CompiledExpression difference = [left, right, reversed](double x, double y, double z)
                    {
                        return reversed ? right(x, y, z) - left(x, y, z) : left(x, y, z) - right(x, y, z);
                    };

                const char* symbol = operation == ComparisonOperator::Less ? " < " :
                    operation == ComparisonOperator::LessOrEqual ? " <= " :
                    operation == ComparisonOperator::Greater ? " > " : " >= ";
                result.emplace_back(difference, relation, parts.expressions[i] + symbol + parts.expressions[i + 1]);
            }
        }

        return result;
    }

}
