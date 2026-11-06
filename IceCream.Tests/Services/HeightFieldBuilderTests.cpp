#include "../pch.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Services;

namespace IceCream::Tests::Services
{
    TEST_CLASS(HeightFieldBuilderTests)
    {
    public:
        TEST_METHOD(Build_SurfaceWithXYZ)
        {
            HeightFieldBuilder builder;
            HeightFieldBounds bounds{ 0, 2, -1, 1 };

            auto solid = builder.Build("z=x+2*y", bounds, 8);

            Assert::AreEqual(-2.0, solid.Mesh.Vertices().front().Z, 1e-12);
            Assert::AreEqual(4.0, solid.Mesh.Vertices()[80].Z, 1e-12);
            Assert::AreEqual(-2.6, solid.BaseZ, 1e-12);
            Assert::AreEqual(14.4, solid.Volume, 1e-10);
        }

        TEST_METHOD(Build_FormulaWithoutPrefix)
        {
            HeightFieldBuilder builder;
            HeightFieldBounds bounds{ 0, 2, -1, 1 };

            auto first = builder.Build("z=x+2*y", bounds, 8);
            auto second = builder.Build("x+2*y", bounds, 8);

            Assert::AreEqual(first.Volume, second.Volume, 1e-12);
            Assert::AreEqual(first.BaseZ, second.BaseZ, 1e-12);
        }
    };
}