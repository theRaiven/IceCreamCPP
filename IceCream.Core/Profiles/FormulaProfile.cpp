#include "FormulaProfile.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace IceCream::Core::Profiles
{
    namespace
    {
        using Function = std::function<double(double,double)>;
        class Parser
        {
            const std::string& text_;
            Models::IceCreamParameters parameters_;
            size_t position_ = 0;
            int depth_ = 0;
            void Space() { while (position_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[position_]))) ++position_; }
            bool Take(char c) { Space(); if (position_ < text_.size() && text_[position_] == c) { ++position_; return true; } return false; }
            [[noreturn]] void Error(const char* message) const
            { throw std::invalid_argument(std::string(message) + " (position " + std::to_string(position_ + 1) + ")"); }
            Function Compare()
            {
                auto a = Sum(); Space();
                if (position_ >= text_.size() || (text_[position_] != '<' && text_[position_] != '>')) return a;
                char op = text_[position_++]; bool equal = Take('='); auto b = Sum();
                return [=](double z,double y) { double left=a(z,y), right=b(z,y); return double(op=='<' ? (equal ? left<=right : left<right) : (equal ? left>=right : left>right)); };
            }
            Function Sum()
            {
                auto a = Product();
                for (;;) { if (Take('+')) { auto b=Product(); a=[a,b](double z,double y){return a(z,y)+b(z,y);}; }
                    else if (Take('-')) { auto b=Product(); a=[a,b](double z,double y){return a(z,y)-b(z,y);}; } else return a; }
            }
            Function Product()
            {
                auto a=Unary();
                for (;;) { if (Take('*')) { auto b=Unary(); a=[a,b](double z,double y){return a(z,y)*b(z,y);}; }
                    else if (Take('/')) { auto b=Unary(); a=[a,b](double z,double y){return a(z,y)/b(z,y);}; } else return a; }
            }
            Function Unary()
            {
                if (++depth_ > 64) Error("Expression is too deeply nested");
                Function a;
                if (Take('+')) a=Unary();
                else if (Take('-')) { auto b=Unary(); a=[b](double z,double y){return -b(z,y);}; }
                else { a=Atom(); if (Take('^')) { auto b=Unary(); a=[a,b](double z,double y){return std::pow(a(z,y),b(z,y));}; } }
                --depth_; return a;
            }
            Function Atom()
            {
                if (Take('(')) { auto f=Compare(); if (!Take(')')) Error("Expected )"); return f; }
                Space();
                if (position_ == text_.size()) Error("Expected a value");
                char c=text_[position_];
                if (std::isdigit(static_cast<unsigned char>(c)) || c=='.') {
                    char* end=nullptr; const char* begin=text_.c_str()+position_;
                    double value=std::strtod(begin,&end);
                    if (end==begin || !std::isfinite(value)) Error("Invalid number");
                    position_+=end-begin; return [value](double,double){return value;};
                }
                size_t start=position_;
                while (position_<text_.size() && std::isalpha(static_cast<unsigned char>(text_[position_]))) ++position_;
                std::string name=text_.substr(start,position_-start);
                if (name.empty()) Error("Unexpected character");
                const auto p=parameters_;
                if (name=="y") return [](double,double y){return y;};
                if (name=="z" || name=="x") return [](double z,double){return z;};
                if (name=="H") return [p](double,double){return p.BaseHeight;};
                if (name=="R" || name=="radius") return [p](double,double){return p.JoinRadius;};
                if (name=="p") return [p](double,double){return p.BaseExponent;};
                if (name=="h") return [p](double,double){return p.CapHeight;};
                if (name=="L" || name=="height") return [p](double,double){return p.BaseHeight+p.CapHeight;};
                if (name=="pi") return [](double,double){return std::acos(-1.0);};
                if (!Take('(')) Error("Unknown variable or missing (");
                std::vector<Function> args;
                args.push_back(Compare());
                while (Take(',')) { if (args.size()>=3) Error("Too many arguments"); args.push_back(Compare()); }
                if (!Take(')')) Error("Expected )");
                if (name=="if" && args.size()==3) return [args](double z,double y){return args[0](z,y)!=0 ? args[1](z,y) : args[2](z,y);};
                if (args.size()==2) {
                    auto a=args[0], b=args[1];
                    if(name=="min") return [a,b](double z,double y){return std::min(a(z,y),b(z,y));};
                    if(name=="max") return [a,b](double z,double y){return std::max(a(z,y),b(z,y));};
                    if(name=="pow") return [a,b](double z,double y){return std::pow(a(z,y),b(z,y));};
                }
                if(args.size()==1) {
                    auto a=args[0];
                    if(name=="sqrt") return [a](double z,double y){return std::sqrt(a(z,y));};
                    if(name=="sin") return [a](double z,double y){return std::sin(a(z,y));};
                    if(name=="cos") return [a](double z,double y){return std::cos(a(z,y));};
                    if(name=="abs") return [a](double z,double y){return std::abs(a(z,y));};
                    if(name=="exp") return [a](double z,double y){return std::exp(a(z,y));};
                    if(name=="log") return [a](double z,double y){return std::log(a(z,y));};
                }
                Error("Unknown function or wrong argument count");
            }
        public:
            Parser(const std::string& text, Models::IceCreamParameters p):text_(text),parameters_(p) {}
            Function Parse() { auto result=Compare(); Space(); if(position_!=text_.size()) Error("Unexpected text"); return result; }
        };
    }
    std::function<double(double,double)> CompileExpression(const std::string& formula,const Models::IceCreamParameters& p)
    {
        if(formula.empty() || formula.size()>512) throw std::invalid_argument("Formula must contain 1 to 512 characters");
        return Parser(formula,p).Parse();
    }
    const char* FormulaProfile::DefaultFormula()
    { return "if(z<=H,R*(z/H)^p,sqrt(max(0,(1-(z-H)/h)*(R^2+h*(z-H)))))"; }
    FormulaProfile::FormulaProfile(const std::string& formula,const Models::IceCreamParameters& p)
        : height_(p.BaseHeight+p.CapHeight)
    {
        if(formula.empty() || formula.size()>512) throw std::invalid_argument("Formula must contain 1 to 512 characters");
        if(!std::isfinite(height_) || height_<=0) throw std::invalid_argument("Invalid height");
        auto expression=CompileExpression(formula,p);
        expression_=[expression](double z){return expression(z,0);};
        // Reject invalid radii and interior pinches before replacing the current solid.
        for(int i=0;i<=2048;++i) {
            double radius=GetRadius(height_*i/2048);
            if(i>0 && i<2048 && radius<=1e-8) throw std::invalid_argument("Radius must be positive inside 0 < z < H+h");
        }
    }
    double FormulaProfile::GetRadius(double z) const
    {
        if(!std::isfinite(z) || z<0 || z>height_) throw std::out_of_range("Height outside formula domain");
        double r=expression_(z);
        if(!std::isfinite(r) || r<0 || r>20) throw std::invalid_argument("Formula radius must be finite and between 0 and 20");
        if ((z==0 || z==height_) && r<1e-8) return 0;
        return r;
    }
}
