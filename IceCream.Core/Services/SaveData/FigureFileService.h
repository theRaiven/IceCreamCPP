#pragma once

#include "FigureData.h"
#include <filesystem>

namespace IceCream::Core::Services::SaveData
{
    class FigureFileService
    {
    public:
        /// <summary>
		/// Сохранение данных фигуры в файл
        /// </summary>
        /// <param name="figure">Данные фигуры</param>
        /// <param name="path">Путь к файлу</param>
        static void Save(const FigureData& figure, const std::filesystem::path& path);
        /// <summary>
		/// Загрузка данных фигуры из файла
        /// </summary>
        /// <param name="path">Путь к файлу</param>
        /// <returns>Данные фигуры</returns>
        static FigureData Load(const std::filesystem::path& path);
    };
}