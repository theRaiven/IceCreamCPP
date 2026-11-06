#pragma once

#include <functional>

namespace IceCream::Core::Expressions
{
	using CompiledExpression = std::function<double(double, double, double)>;


}