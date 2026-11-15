#pragma once

#include "FigureData.h"
#include <string>

namespace IceCream::Core::Services::SaveData
{
    class FigureSerializer
    {
    public:
        /// <summary>
		/// Сериализация данных фигуры в JSON-строку
        /// </summary>
        /// <param name="figure">Данные фигуры</param>
        /// <returns>JSON-строка</returns>
        static std::string Serialize(const FigureData& figure);
        /// <summary>
		/// Десериализация данных фигуры из JSON-строки
        /// </summary>
        /// <param name="json">JSON-строка</param>
        /// <returns>Данные фигуры</returns>
        static FigureData Deserialize(const std::string& json);
    };
}