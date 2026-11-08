#pragma once

#include "../Parsing/CompiledExpression.h"
#include <string>

namespace IceCream::Core::Inequalities
{
	using namespace Expressions;
    enum class InequalityRelation
    {
        Less,       // g<0
        LessOrEqual // g<=0
    };

    class Inequality
    {
    public:
        Inequality(CompiledExpression expression, InequalityRelation relation, std::string label = "");

        double Evaluate(double x, double y, double z) const;
        bool IsSatisfied(double x, double y, double z) const;
        const std::string& Label() const { return label_; }

    private:
        CompiledExpression expression_;
        InequalityRelation relation_;
        std::string label_;
    };
}
