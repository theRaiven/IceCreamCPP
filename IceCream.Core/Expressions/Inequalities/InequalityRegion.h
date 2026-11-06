#pragma once

#include "Inequality.h"

#include <vector>

namespace IceCream::Core::Inequalities
{
    class InequalityRegion
    {
    private:
        std::vector<Inequality> inequalities_;

    public:
        explicit InequalityRegion(std::vector<Inequality> inequalities);

        bool Contains(double x, double y, double z) const;

        double Evaluate(double x, double y, double z) const;

        const std::vector<Inequality>& GetInequalities() const noexcept;
    };
}
