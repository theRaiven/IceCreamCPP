#include "Inequality.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace IceCream::Core::Inequalities
{
	/// <summary>
	/// Создает неравенство вида g(x,y,z) < 0 или g(x,y,z) <= 0
	/// </summary>
	/// <param name="expression"> Выражение g(x,y,z) </param>
	/// <param name="relation"> Отношение неравенства </param>
	Inequality::Inequality(CompiledExpression expression, InequalityRelation relation) : expression_(std::move(expression)), relation_(relation) 
	{
		if (!expression_)
		{
			throw std::invalid_argument("Неравенство не может быть пустым");
		}
	}

	/// <summary>
	/// Вычисляет значение выражения g(x,y,z)
	/// </summary>
	/// <param name="x"> Значение x </param>
	/// <param name="y"> Значение y </param>
	/// <param name="z"> Значение z </param>
	/// <returns> Значение выражения g(x,y,z) </returns>
	double Inequality::Evaluate(double x, double y, double z) const
	{
		double result = expression_(x, y, z);

		if (!std::isfinite(result))
		{
			throw std::domain_error("Ошибка вычисления неравенства");
		}
		return result;
	}
	/// <summary>
	/// Проверяет, выполняется ли неравенство g(x,y,z) < 0 или g(x,y,z) <= 0
	/// </summary>
	/// <param name="x"> Значение x </param>
	/// <param name="y"> Значение y </param>
	/// <param name="z"> Значение z </param>
	/// <returns> true, если неравенство выполняется, false в противном случае </returns>
	bool Inequality::IsSatisfied(double x, double y, double z) const
	{
		double value = Evaluate(x, y, z);

		switch (relation_)
		{
		case InequalityRelation::Less:
			return value < 0;

		case InequalityRelation::LessOrEqual:
			return value <= 0;

		default:
			throw std::logic_error("Неизвестное отношение неравенства");
		}
	}
}