#include "ExpressionParser.h"

#include <string>
#include <stdexcept>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <algorithm>

using namespace std;

namespace IceCream::Core::Expressions
{
    namespace
    {
        class Parser
        {
        private:
            const string& text;
            const ExpressionParser::Constants& constants;

            size_t position = 0;
            int depth = 0;

            void Space()
            {
                while (position < text.size() && isspace(static_cast<unsigned char>(text[position])))
                {
                    position++;
                }
            }

            bool Take(char c)
            {
                Space();

                if (position < text.size() && text[position] == c)
                {
                    position++;
                    return true;
                }
                return false;
            }

            [[noreturn]] void Error(const char* message) const
            {
                throw invalid_argument(string(message) + " (позиция " + to_string(position + 1) + ")");
            }

            CompiledExpression Compare()
            {
                auto left = Sum();

                Space();

                if (position >= text.size() || (text[position] != '<' && text[position] != '>'))
                {
                    return left;
                }

                char operation = text[position++];
                bool equal = Take('=');

                auto right = Sum();

                return [left, right, operation, equal](double x, double y, double z)
                {
                    double leftValue = left(x, y, z);
                    double rightValue = right(x, y, z);

                    if (operation == '<')
                    {
                        return double(equal ? leftValue <= rightValue : leftValue < rightValue);
                    }

                    return double(equal ? leftValue >= rightValue : leftValue > rightValue);
                };
            }
            CompiledExpression Sum()
            {
                auto left = Product();

                while (true)
                {
                    Space();

                    if (Take('+'))
                    {
                        auto right = Product();

                        left = [left, right](double x, double y, double z)
                            {
                                return left(x, y, z) + right(x, y, z);
                            };
                    }
                    else if (Take('-'))
                    {
                        auto right = Product();

                        left = [left, right](double x, double y, double z)
                            {
                                return left(x, y, z) - right(x, y, z);
                            };
                    }
                    else
                    {
                        break;
                    }
                }

                return left;
            }
            CompiledExpression Product()
            {
                auto left = Unary();

                while (true)
                {
                    Space();

                    if (Take('*'))
                    {
                        auto right = Unary();

                        left = [left, right](double x, double y, double z)
                            {
                                return left(x, y, z) * right(x, y, z);
                            };
                    }
                    else if (Take('/'))
                    {
                        auto right = Unary();

                        left = [left, right](double x, double y, double z)
                        {
                            return left(x, y, z) / right(x, y, z);
                        };
                    }
                    else
                    {
                        break;
                    }
                }

                return left;
            }
            CompiledExpression Unary()
            {
                ++depth;

                if (depth > 64)
                {
                    Error("Expression is too deeply nested");
                }

                CompiledExpression result;

                if (Take('+'))
                {
                    result = Unary();
                }
                else if (Take('-'))
                {
                    auto value = Unary();

                    result = [value](double x, double y, double z)
                    {
                        return -value(x, y, z);
                    };
                }
                else
                {
                    result = Atom();

                    if (Take('^'))
                    {
                        auto exponent = Unary();

                        result = [result, exponent](double x, double y, double z)
                        {
                            return std::pow(result(x, y, z), exponent(x, y, z));
                        };
                    }
                }
                --depth;
                return result;
            }
            CompiledExpression Atom()
            {
                // Выражение в скобках
                if (Take('('))
                {
                    auto result = Compare();

                    if (!Take(')')) Error("Expected )");

                    return result;
                }

                Space();

                if (position >= text.size())
                {
                    Error("Expected a value");
                }

                char current = text[position];

                // Число
                if (std::isdigit(static_cast<unsigned char>(current)) || current == '.')
                {
                    char* end = nullptr;
                    const char* begin = text.c_str() + position;

                    double value = std::strtod(begin, &end);

                    if (end == begin || !std::isfinite(value))
                    {
                        Error("Invalid number");
                    }

                    position += end - begin;

                    return [value](double, double, double) { return value; };
                }

                // Имя переменной, константы или функции
                size_t start = position;

                while (position < text.size() && std::isalpha(static_cast<unsigned char>(text[position])))
                {
                    position++;
                }

                std::string name = text.substr(start, position - start);

                if (name.empty())
                {
                    Error("Unexpected character");
                }

                // Координаты
                if (name == "x") return [](double x, double, double) { return x; };
                if (name == "y") return [](double, double y, double) { return y; };
                if (name == "z") return [](double, double, double z) { return z; };

                // Число Пи
                if (name == "pi")
                {
                    double value = std::acos(-1.0);
                    return [value](double, double, double) { return value; };
                }

                // Именованные константы
                auto it = constants.find(name);

                if (it != constants.end())
                {
                    double value = it->second;
                    return [value](double, double, double) { return value; };
                }

                // Проверяем открывающую скобку функции
                if (!Take('('))
                {
                    Error("Unknown variable or missing (");
                }

                // Разбираем аргументы функции
                std::vector<CompiledExpression> arguments;

                arguments.push_back(Compare());

                while (Take(','))
                {
                    if (arguments.size() >= 3)
                    {
                        Error("Too many arguments");
                    }

                    arguments.push_back(Compare());
                }

                if (!Take(')'))
                {
                    Error("Expected )");
                }

                // Функция с тремя аргументами
                if (name == "if" && arguments.size() == 3)
                {
                    return [arguments](double x, double y, double z)
                        {
                            return arguments[0](x, y, z) != 0
                                ? arguments[1](x, y, z)
                                : arguments[2](x, y, z);
                        };
                }

                // Функции с двумя аргументами
                if (arguments.size() == 2)
                {
                    auto first = arguments[0];
                    auto second = arguments[1];

                    if (name == "min")
                    {
                        return [first, second](double x, double y, double z)
                            {
                                return std::min(first(x, y, z), second(x, y, z));
                            };
                    }

                    if (name == "max")
                    {
                        return [first, second](double x, double y, double z)
                            {
                                return std::max(first(x, y, z), second(x, y, z));
                            };
                    }

                    if (name == "pow")
                    {
                        return [first, second](double x, double y, double z)
                            {
                                return std::pow(first(x, y, z), second(x, y, z));
                            };
                    }
                }

                // Функции с одним аргументом
                if (arguments.size() == 1)
                {
                    auto argument = arguments[0];

                    if (name == "sqrt")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::sqrt(argument(x, y, z));
                            };
                    }

                    if (name == "sin")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::sin(argument(x, y, z));
                            };
                    }

                    if (name == "cos")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::cos(argument(x, y, z));
                            };
                    }

                    if (name == "abs")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::abs(argument(x, y, z));
                            };
                    }

                    if (name == "exp")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::exp(argument(x, y, z));
                            };
                    }

                    if (name == "log")
                    {
                        return [argument](double x, double y, double z)
                            {
                                return std::log(argument(x, y, z));
                            };
                    }
                }

                Error("Unknown function or wrong argument count");
            }

        public:
            Parser(const std::string& text, const ExpressionParser::Constants& constants)
                : text(text), constants(constants) { }

            CompiledExpression Parse()
            {
                auto result = Compare();

                Space();

                if (position != text.size())
                {
                    Error("Unexpected text");
                }

                return result;
            }
        };
    }

    CompiledExpression ExpressionParser::Compile(const std::string& formula, const Constants& constants)
    {
        if (formula.empty() || formula.size() > 512)
        {
            throw std::invalid_argument("Формула должна содержать от 1 до 512 символов");
        }

        return Parser(formula, constants).Parse();
    }

    CompiledExpression ExpressionParser::CompileEquation(const string& formula, const Constants& constants)
    {
        size_t pos = formula.find('=');

        if (pos == string::npos) return Compile(formula, constants);

        if (pos == 0 || pos + 1 == formula.size() || formula.size() > 512 ||
            formula.find('=', pos + 1) != string::npos ||
            formula[pos - 1] == '<' || formula[pos - 1] == '>')
        {
            throw invalid_argument("Неверное уравнение");
        }

        auto left = Compile(formula.substr(0, pos), constants);
        auto right = Compile(formula.substr(pos + 1), constants);

        return [left, right](double x, double y, double z)
            {
                return left(x, y, z) - right(x, y, z);
            };
    }
}
