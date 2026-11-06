#pragma once

#include "../Parsing/CompiledExpression.h"

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
        Inequality(CompiledExpression expression, InequalityRelation relation);

        double Evaluate(double x, double y, double z) const;
        bool IsSatisfied(double x, double y, double z) const;

    private:
        CompiledExpression expression_;
        InequalityRelation relation_;
    };
}