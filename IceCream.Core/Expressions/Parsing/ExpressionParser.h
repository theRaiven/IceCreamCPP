#pragma once

#include "CompiledExpression.h"
#include <string>
#include <unordered_map>

namespace IceCream::Core::Expressions
{
	class ExpressionParser
	{
	public:
		using Constants = std::unordered_map<std::string, double>;

		static CompiledExpression Compile(const std::string& formula, const Constants& constants = {});
		// Bounds may compose two full formulas when a graph becomes an integral.
		static CompiledExpression CompileBound(const std::string& formula, const Constants& constants = {});

		static CompiledExpression CompileEquation(const std::string& formula, const Constants& constants = {});
	};
}
