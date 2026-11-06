#include "InequalityRegion.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

using namespace std;

namespace IceCream::Core::Inequalities
{
    InequalityRegion::InequalityRegion(vector<Inequality> inequalities)
        : inequalities_(move(inequalities))
    {
        if (inequalities_.empty())
        {
            throw invalid_argument("Inequality region cannot be empty");
        }
    }

    bool InequalityRegion::Contains(double x, double y, double z) const
    {
        for (const Inequality& inequality : inequalities_)
        {
            if (!inequality.IsSatisfied(x, y, z))
            {
                return false;
            }
        }

        return true;
    }

    double InequalityRegion::Evaluate(double x, double y, double z) const
    {
        double value = inequalities_.front().Evaluate(x, y, z);

        for (size_t i = 1; i < inequalities_.size(); ++i)
        {
            value = max(value, inequalities_[i].Evaluate(x, y, z));
        }

        return value;
    }

    const vector<Inequality>& InequalityRegion::GetInequalities() const noexcept
    {
        return inequalities_;
    }
}
