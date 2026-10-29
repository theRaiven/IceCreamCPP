#include "FormulaProfile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <vector>

using namespace std;
using namespace IceCream::Core::Models;

namespace IceCream::Core::Profiles
{
    namespace
    {
        using Function = function<double(double, double)>;

        class Parser
        {
        private:
            const string& text;
            IceCreamParameters parameters;

            size_t position = 0;
            int depth = 0;

            /// <summary>
            /// обраточка пробелов
            /// </summary>
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
                throw invalid_argument(string(message) + " (position " + to_string(position + 1) + ")");
            }

            Function Compare()
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

                return [left, right, operation, equal](double z, double y)
                {
                    double leftValue = left(z, y);
                    double rightValue = right(z, y);

                    if (operation == '<')
                    {
                        return double(equal ? leftValue <= rightValue : leftValue < rightValue);
                    }

                    return double(equal ? leftValue >= rightValue : leftValue > rightValue);
                };
            }
            Function Sum()
            {
                auto result = Product();

                while (true)
                {
                    if (Take('+'))
                    {
                        auto right = Product();

                        result = [result, right](double z, double y)
                        {
                            return result(z, y) + right(z, y);
                        };
                    }
                    else if (Take('-'))
                    {
                        auto right = Product();

                        result = [result, right](double z, double y)
                        {
                            return result(z, y) - right(z, y);
                        };
                    }
                    else
                    {
                        return result;
                    }
                }
            }
            Function Product()
            {
                auto result = Unary();

                while(true)
                {
                    if (Take('*'))
                    {
                        auto right = Unary();

                        result = [result, right](double z, double y)
                        {
                            return result(z, y) * right(z, y);
                        };
                    }
                    else if (Take('/'))
                    {
                        auto right = Unary();

                        result = [result, right](double z, double y)
                        {
                            return result(z, y) / right(z, y);
                        };
                    }
                    else
                    {
                        return result;
                    }
                }
            }
            Function Unary()
            {
                ++depth;

                if (depth > 64)
                {
                    Error("Expression is too deeply nested");
                }

                Function result;

                if (Take('+'))
                {
                    result = Unary();
                }
                else if (Take('-'))
                {
                    auto value = Unary();

                    result = [value](double z, double y)
                    {
                        return -value(z, y);
                    };
                }
                else
                {
                    result = Atom();

                    if (Take('^'))
                    {
                        auto exponent = Unary();

                        result = [result, exponent](double z, double y)
                        {
                            return pow(result(z, y), exponent(z, y));
                        };
                    }
                }

                --depth;
                return result;
            }
            Function Atom()
            {
                if (Take('('))
                {
                    auto result = Compare();

                    if (!Take(')')) Error("Expected )");

                    return result;
                }

                Space();

                if (position == text.size())
                {
                    Error("Expected a value");
                }

                char current = text[position];

                if (isdigit(static_cast<unsigned char>(current)) || current == '.')
                {
                    char* end = nullptr;
                    const char* begin = text.c_str() + position;

                    double value = strtod(begin, &end);

                    if (end == begin || !isfinite(value))
                    {
                        Error("Invalid number");
                    }

                    position += end - begin;

                    return [value](double, double)
                        {
                            return value;
                        };
                }

                size_t start = position;

                while (position < text.size() && isalpha(static_cast<unsigned char>(text[position])))
                {
                    ++position;
                }

                string name = text.substr(start, position - start);

                if (name.empty())
                {
                    Error("Unexpected character");
                }

                const auto param = parameters;

                if (name == "y")
                {
                    return [](double, double y)
                        {
                            return y;
                        };
                }

                if (name == "z" || name == "x")
                {
                    return [](double z, double)
                        {
                            return z;
                        };
                }

                if (name == "H")
                {
                    return [param](double, double)
                        {
                            return param.BaseHeight;
                        };
                }

                if (name == "R" || name == "radius")
                {
                    return [param](double, double)
                        {
                            return param.JoinRadius;
                        };
                }

                if (name == "p")
                {
                    return [param](double, double)
                        {
                            return param.BaseExponent;
                        };
                }

                if (name == "h")
                {
                    return [param](double, double)
                        {
                            return param.CapHeight;
                        };
                }

                if (name == "L" || name == "height")
                {
                    return [param](double, double)
                        {
                            return param.BaseHeight +  param.CapHeight;
                        };
                }

                if (name == "pi")
                {
                    return [](double, double)
                        {
                            return acos(-1.0);
                        };
                }

                if (!Take('('))
                {
                    Error("Unknown variable or missing (");
                }

                vector<Function> arguments;

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

                if (name == "if" && arguments.size() == 3)
                {
                    return [arguments](double z, double y)
                        {
                            return arguments[0](z, y) != 0 ? arguments[1](z, y) : arguments[2](z, y);
                        };
                }

                if (arguments.size() == 2)
                {
                    auto first = arguments[0];
                    auto second = arguments[1];

                    if (name == "min")
                    {
                        return [first, second](double z, double y)
                            {
                                return min(first(z, y), second(z, y));
                            };
                    }

                    if (name == "max")
                    {
                        return [first, second](double z, double y)
                            {
                                return max(first(z, y), second(z, y));
                            };
                    }

                    if (name == "pow")
                    {
                        return [first, second](double z, double y)
                            {
                                return pow(first(z, y), second(z, y));
                            };
                    }
                }

                if (arguments.size() == 1)
                {
                    auto argument = arguments[0];

                    if (name == "sqrt")
                    {
                        return [argument](double z, double y)
                            {
                                return sqrt(argument(z, y));
                            };
                    }

                    if (name == "sin")
                    {
                        return [argument](double z, double y)
                            {
                                return sin(argument(z, y));
                            };
                    }

                    if (name == "cos")
                    {
                        return [argument](double z, double y)
                            {
                                return cos(argument(z, y));
                            };
                    }

                    if (name == "abs")
                    {
                        return [argument](double z, double y)
                            {
                                return abs(argument(z, y));
                            };
                    }

                    if (name == "exp")
                    {
                        return [argument](double z, double y)
                            {
                                return exp(argument(z, y));
                            };
                    }

                    if (name == "log")
                    {
                        return [argument](double z, double y)
                            {
                                return log(argument(z, y));
                            };
                    }
                }

                Error("Unknown function or wrong argument count");
            }
        public:
            Parser(const string& text, IceCreamParameters parameters) : text(text), parameters(parameters) { }

            Function Parse()
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
    function<double(double, double)> CompileExpression(const string& formula, const IceCreamParameters& parameters)
    {
        if (formula.empty() || formula.size() > 512)
        {
            throw invalid_argument("Formula must contain 1 to 512 characters");
        }

        return Parser(formula, parameters).Parse();
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
