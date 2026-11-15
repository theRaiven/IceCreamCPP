#include "FigureSerializer.h"
#include <nlohmann/json.hpp>
#include <stdexcept>

using namespace std;
using json = nlohmann::json;

namespace IceCream::Core::Services::SaveData
{
	string FigureSerializer::Serialize(const FigureData& figure)
	{
		json data;

		data["Version"] = figure.Version;
		data["Type"] = static_cast<int>(figure.Type); 

		data["Equation"] = figure.Equation;
		data["Constraints"] = figure.Constraints; 

		data["Bounds"] =
		{
			{"Min", figure.Min},
			{"Max", figure.Max}
		};

		data["Cells"] = figure.Cells; 
		data["Color"] = figure.Color; 
		data["AutoBounds"] = figure.AutoBounds; 

		data["Parts"] = json::array(); 

		for (const auto& part : figure.Parts)
		{
			json item;

			item["Name"] = part.Name;
			item["Coordinates"] = static_cast<int>(part.Coordinates);
			item["Definition"] = static_cast<int>(part.Definition);

			item["Function"] = part.Function;
			item["DependentAxis"] = part.DependentAxis;

			item["Base"] = part.Base;
			item["AboveFunction"] = part.AboveFunction;
			item["AutoUpper"] = part.AutoUpper;

			item["DependentLower"] = part.DependentLower;
			item["DependentUpper"] = part.DependentUpper;

			item["Order"] = part.Order;
			item["Lower"] = part.Lower;
			item["Upper"] = part.Upper;
			item["Color"] = part.Color;

			data["Parts"].push_back(item);
		}

		return data.dump(4);
	}
	FigureData FigureSerializer::Deserialize(const string& text)
	{
        json data = json::parse(text);

        FigureData figure;

        figure.Version = data.at("Version").get<int>();

        if (figure.Version != 1)
        {
            throw runtime_error("Неподдерживаемая версия формата фигуры");
        }

        int type = data.at("Type").get<int>();

        if (type < 0 || type > 2)
        {
            throw runtime_error("Неизвестный тип фигуры");
        }

        figure.Type = static_cast<FigureType>(type);

        figure.Equation = data.at("Equation").get<string>();
        figure.Constraints = data.at("Constraints").get<string>();

        figure.Min = data.at("Bounds").at("Min").get<array<double, 3>>();
        figure.Max = data.at("Bounds").at("Max").get<array<double, 3>>();

        figure.Cells = data.at("Cells").get<array<int, 3>>();
        figure.Color = data.at("Color").get<array<float, 4>>();
        figure.AutoBounds = data.at("AutoBounds").get<bool>();

        for (int i = 0; i < 3; ++i)
        {
            if (figure.Min[i] >= figure.Max[i])
            {
                throw runtime_error("Некорректные границы построения фигуры");
            }

            if (figure.Cells[i] <= 0)
            {
                throw runtime_error("Разрешение сетки должно быть положительным");
            }
        }

        const json& parts = data.at("Parts");

        if (!parts.is_array())
        {
            throw runtime_error("Некорректный формат списка частей фигуры");
        }

        for (const auto& item : parts)
        {
            IntegralPart part;

            part.Name = item.at("Name").get<string>();

            int coordinates = item.at("Coordinates").get<int>();
            int definition = item.at("Definition").get<int>();

            if (coordinates < 0 || coordinates > 2)
            {
                throw runtime_error("Неизвестная система координат");
            }

            if (definition < 0 || definition > 1)
            {
                throw runtime_error("Неизвестный способ задания части фигуры");
            }

            part.Coordinates = static_cast<CoordinateSystem>(coordinates);
            part.Definition = static_cast<PartDefinition>(definition);

            part.Function = item.at("Function").get<string>();
            part.DependentAxis = item.at("DependentAxis").get<int>();

            if (part.DependentAxis < 0 || part.DependentAxis > 2)
            {
                throw runtime_error("Некорректная зависимая ось");
            }

            part.Base = item.at("Base").get<string>();
            part.AboveFunction = item.at("AboveFunction").get<bool>();
            part.AutoUpper = item.at("AutoUpper").get<bool>();

            part.DependentLower = item.at("DependentLower").get<string>();
            part.DependentUpper = item.at("DependentUpper").get<string>();

            part.Order = item.at("Order").get<array<int, 3>>();
            part.Lower = item.at("Lower").get<array<string, 3>>();
            part.Upper = item.at("Upper").get<array<string, 3>>();
            part.Color = item.at("Color").get<array<float, 4>>();

            figure.Parts.push_back(part);
        }

        return figure;
	}
}