#pragma once
#include "IRadialProfile.h"
#include "../Models/IceCreamParameters.h"
#include <functional>
#include <string>

namespace IceCream::Core::Profiles
{
    std::function<double(double,double)> CompileExpression(const std::string& formula, const Models::IceCreamParameters& parameters);
    // Safe arithmetic expression for the radius of the entire solid, 0 <= z <= H+h.
    class FormulaProfile final : public IRadialProfile
    {
        std::function<double(double)> expression_;
        double height_;
    public:
        FormulaProfile(const std::string& formula, const Models::IceCreamParameters& parameters);
        double MinZ() const override { return 0; }
        double MaxZ() const override { return height_; }
        double GetRadius(double z) const override;
        static const char* DefaultFormula();
    };
}
