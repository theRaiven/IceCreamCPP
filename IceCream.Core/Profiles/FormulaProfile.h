#pragma once
#include "IRadialProfile.h"
#include "../Models/IceCreamParameters.h"
#include <functional>
#include <string>

namespace IceCream::Core::Profiles
{
    std::function<double(double,double)> CompileExpression(const std::string& formula, const Models::IceCreamParameters& parameters);

    class FormulaProfile final : public IRadialProfile
    {
    private:
        std::function<double(double)> expression_;

        double minZ_, maxZ_;

    public:
        FormulaProfile(const std::string& formula, const Models::IceCreamParameters& parameters);
        FormulaProfile(const std::string& formula, const Models::IceCreamParameters& parameters, double minZ, double maxZ);

        double MinZ() const override { return minZ_; }
        double MaxZ() const override { return maxZ_; }

        double GetRadius(double z) const override;
        static const char* DefaultFormula();
    };
}
