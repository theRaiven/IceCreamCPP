#include "HeightFieldBuilder.h"
#include "../Profiles/FormulaProfile.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <limits>
#include <stdexcept>

namespace IceCream::Core::Services
{
    HeightFieldSolid HeightFieldBuilder::Build(const std::string& formula,const HeightFieldBounds& b,int n) const
    {
        for(double value : {b.MinX,b.MaxX,b.MinY,b.MaxY})
            if(!std::isfinite(value) || std::abs(value)>10) throw std::invalid_argument("X/Y bounds must be in -10..10");
        if(b.MaxX-b.MinX<.1 || b.MaxY-b.MinY<.1 || n<2 || n>256)
            throw std::invalid_argument("Invalid domain or resolution");
        std::string expression=formula;
        expression.erase(0,expression.find_first_not_of(" \t\r\n"));
        if(!expression.empty() && expression[0]=='z') {
            size_t equals=expression.find_first_not_of(" \t",1);
            if(equals!=std::string::npos && expression[equals]=='=') expression=expression.substr(equals+1);
        }
        auto f=Profiles::CompileExpression(expression,{3,1,.5,1.2});
        const int row=n+1, count=row*row;
        std::vector<Geometry::Point3D> vertices; vertices.reserve(count*2);
        double minZ=std::numeric_limits<double>::max(), maxZ=-minZ;
        for(int j=0;j<=n;++j) for(int i=0;i<=n;++i) {
            double x=b.MinX+(b.MaxX-b.MinX)*i/n, y=b.MinY+(b.MaxY-b.MinY)*j/n;
            double z=f(x,y);
            if(!std::isfinite(z) || std::abs(z)>50) throw std::invalid_argument("Surface must be finite and within Z=-50..50");
            vertices.push_back({x,y,z}); minZ=std::min(minZ,z); maxZ=std::max(maxZ,z);
        }
        const double baseZ=minZ-std::max(.25,(maxZ-minZ)*.1);
        for(int k=0;k<count;++k) vertices.push_back({vertices[k].X,vertices[k].Y,baseZ});
        std::vector<int> indices; indices.reserve(12*n*n+24*n);
        auto triangle=[&](int a,int c,int d){indices.insert(indices.end(),{a,c,d});};
        double volume=0;
        const double area=(b.MaxX-b.MinX)*(b.MaxY-b.MinY)/(2*n*n);
        auto top=[&](int a,int c,int d) {
            triangle(a,c,d);
            volume+=area*((vertices[a].Z+vertices[c].Z+vertices[d].Z)/3-baseZ);
            triangle(a+count,d+count,c+count);
        };
        for(int j=0;j<n;++j) for(int i=0;i<n;++i) {
            int a=j*row+i, c=a+1, d=a+row;
            top(a,c,d+1); top(a,d+1,d);
        }
        std::vector<int> perimeter;
        for(int i=0;i<=n;++i) perimeter.push_back(i);
        for(int j=1;j<=n;++j) perimeter.push_back(j*row+n);
        for(int i=n-1;i>=0;--i) perimeter.push_back(n*row+i);
        for(int j=n-1;j>=1;--j) perimeter.push_back(j*row);
        std::vector<Geometry::Point3D> upper,lower;
        for(size_t k=0;k<perimeter.size();++k) {
            int a=perimeter[k], c=perimeter[(k+1)%perimeter.size()];
            triangle(a,a+count,c+count); triangle(a,c+count,c);
            upper.push_back(vertices[a]); lower.push_back(vertices[a+count]);
        }
        upper.push_back(upper.front()); lower.push_back(lower.front());
        std::vector<std::vector<Geometry::Point3D>> edges{upper,lower};
        for(int corner : {0,n,n*row,n*row+n}) edges.push_back({vertices[corner],vertices[corner+count]});
        return {Geometry::TriangleMesh(std::move(vertices),std::move(indices)),volume,baseZ,std::move(edges)};
    }
}
