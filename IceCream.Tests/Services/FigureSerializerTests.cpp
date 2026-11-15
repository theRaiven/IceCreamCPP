#include "../pch.h"
#include "../../IceCream.Core/Services/SaveData/FigureSerializer.h"
#include "../../IceCream.Core/Services/SaveData/FigureFileService.h"
#include <filesystem>

using namespace std;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace IceCream::Core::Services;
using namespace IceCream::Core::Services::SaveData;

namespace IceCream::Tests::Services
{
    TEST_CLASS(FigureSerializerTests)
    {
    public:

        TEST_METHOD(Serialize_ImplicitFigure)
        {
            FigureData original;

            original.Type = FigureType::Implicit;
            original.Equation = "x^2+y^2+z^2-9";
            original.Min = { -5, -5, -5 };
            original.Max = { 5, 5, 5 };
            original.Cells = { 60, 60, 60 };

            string json = FigureSerializer::Serialize(original);
            FigureData restored = FigureSerializer::Deserialize(json);

            Assert::IsTrue(restored.Type == FigureType::Implicit);
            Assert::IsTrue(restored.Equation == original.Equation);
            Assert::IsTrue(restored.Min == original.Min);
            Assert::IsTrue(restored.Max == original.Max);
            Assert::IsTrue(restored.Cells == original.Cells);
        }

        TEST_METHOD(Serialize_ExplicitFigure)
        {
            FigureData original;
            original.Type = FigureType::Explicit;

            IntegralPart part;
            part.Name = "Параболоид";
            part.Definition = PartDefinition::Function;
            part.Function = "x^2+y^2";
            part.DependentAxis = 2;
            part.Color = { 1.0f, 0.5f, 0.2f, 1.0f };

            original.Parts.push_back(part);

            string json = FigureSerializer::Serialize(original);
            FigureData restored = FigureSerializer::Deserialize(json);

            Assert::IsTrue(restored.Type == FigureType::Explicit);
            Assert::AreEqual(size_t(1), restored.Parts.size());
            Assert::IsTrue(restored.Parts[0].Name == part.Name);
            Assert::IsTrue(restored.Parts[0].Function == part.Function);
            Assert::IsTrue(restored.Parts[0].Color == part.Color);
        }

        TEST_METHOD(Serialize_InequalityFigure)
        {
            FigureData original;

            original.Type = FigureType::Inequalities;
            original.Constraints = "x^2+y^2<=4, z>=0, z<=3";

            string json = FigureSerializer::Serialize(original);
            FigureData restored = FigureSerializer::Deserialize(json);

            Assert::IsTrue(restored.Type == FigureType::Inequalities);
            Assert::IsTrue(restored.Constraints == original.Constraints);
        }

        TEST_METHOD(Deserialize_InvalidType_ThrowsException)
        {
            FigureData figure;

            string json = FigureSerializer::Serialize(figure);

            size_t position = json.find("\"Type\": 0");
            Assert::IsTrue(position != string::npos);

            json.replace(position, 9, "\"Type\": 9");

            Assert::ExpectException<runtime_error>([&]()
                {
                    FigureSerializer::Deserialize(json);
                });
        }

        TEST_METHOD(SaveAndLoad_FigureFile)
        {
            FigureData original;

            original.Type = FigureType::Implicit;
            original.Equation = "x^2+y^2+z^2-9";
            original.Constraints = "z>=0";
            original.Min = { -5, -5, -5 };
            original.Max = { 5, 5, 5 };
            original.Cells = { 60, 60, 60 };

            filesystem::path path = filesystem::temp_directory_path() / L"IceCream_Test_Sphere.icefig";

            try
            {
                FigureFileService::Save(original, path);

                Assert::IsTrue(filesystem::exists(path));

                FigureData restored = FigureFileService::Load(path);

                Assert::IsTrue(restored.Type == original.Type);
                Assert::AreEqual(original.Equation, restored.Equation);
                Assert::AreEqual(original.Constraints, restored.Constraints);
                Assert::IsTrue(restored.Min == original.Min);
                Assert::IsTrue(restored.Max == original.Max);
                Assert::IsTrue(restored.Cells == original.Cells);
            }
            catch (...)
            {
                filesystem::remove(path);
                throw;
            }

            filesystem::remove(path);
        }
    };
}