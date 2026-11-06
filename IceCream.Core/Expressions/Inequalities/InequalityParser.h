#pragma once

#include "Inequality.h"
#include "../Parsing/ExpressionParser.h"

#include <string>
#include <vector>

namespace IceCream::Core::Inequalities
{
    class InequalityParser
    {
    public:
        static std::vector<Inequality> Parse(const std::string& text, const Expressions::ExpressionParser::Constants& constants = {});
    };
}