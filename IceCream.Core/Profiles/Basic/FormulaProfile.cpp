#include "FormulaProfile.h"
#include "../../Expressions/Parsing/ExpressionParser.h"

#include <cmath>
#include <stdexcept>

using namespace std;
using namespace IceCream::Core::Models;

namespace IceCream::Core::Profiles
{
    function<double(double, double, double)> CompileSpatialExpression(const string& formula, const IceCreamParameters& parameters)
    {
        Expressions::ExpressionParser::Constants constants = {
            {"H",      parameters.BaseHeight},
            {"R",      parameters.JoinRadius},
            {"radius", parameters.JoinRadius},
            {"p",      parameters.BaseExponent},
            {"h",      parameters.CapHeight},
            {"L",      parameters.BaseHeight + parameters.CapHeight},
            {"height", parameters.BaseHeight + parameters.CapHeight}
        };

        return Expressions::ExpressionParser::Compile(formula, constants);
    }

    function<double(double, double)> CompileExpression(const string& formula, const IceCreamParameters& parameters)
    {
        auto expression = CompileSpatialExpression(formula, parameters);

        return [expression](double z, double y)
            {
                return expression(z, y, z);
            };
    }

    const char* FormulaProfile::DefaultFormula()
    {
        return "if(z<=H,R*(z/H)^p,sqrt(max(0,(1-(z-H)/h)*(R^2+h*(z-H)))))";
    }
    
    FormulaProfile::FormulaProfile(const string& formula, const IceCreamParameters& parameters)
        : FormulaProfile( formula, parameters, 0, parameters.BaseHeight + parameters.CapHeight){}

    FormulaProfile::FormulaProfile(const string& formula, const IceCreamParameters& parameters, double minZ, double maxZ) : minZ_(minZ), maxZ_(maxZ)
    {
        if (!isfinite(minZ_) || !isfinite(maxZ_) || minZ_ >= maxZ_)
        {
            throw invalid_argument("Invalid formula profile range");
        }

        auto expression = CompileExpression(formula, parameters);

        expression_ = [expression](double z)
            {
                return expression(z, 0);
            };

        for (int i = 0; i <= 2048; ++i)
        {
            double z = minZ_ + (maxZ_ - minZ_) * i / 2048.0;

            double radius = GetRadius(z);

            if (i > 0 && i < 2048 && radius <= 1e-8)
            {
                throw invalid_argument("Radius must be positive inside formula profile range");
            }
        }
    }

    double FormulaProfile::GetRadius(double z) const
    {
        if (!isfinite(z) || z < minZ_ || z > maxZ_)
        {
            throw out_of_range("Height outside formula domain");
        }

        double radius = expression_(z);

        if (!isfinite(radius) ||radius < 0 ||  radius > 20)
        {
            throw invalid_argument("Formula radius must be finite and between 0 and 20");
        }

        if ((z == minZ_ || z == maxZ_) && radius < 1e-8)
        {
            return 0;
        }

        return radius;
    }
}
