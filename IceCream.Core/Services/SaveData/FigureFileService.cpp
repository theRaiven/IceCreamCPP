#include "FigureFileService.h"
#include "FigureSerializer.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

using namespace std;

namespace IceCream::Core::Services::SaveData
{
    void FigureFileService::Save(const FigureData& figure, const filesystem::path& path)
    {
        string data = FigureSerializer::Serialize(figure);

        ofstream file(path, ios::binary | ios::trunc);

        if (!file.is_open())
        {
            throw runtime_error("Не удалось открыть файл для сохранения фигуры");
        }

        file.write(data.data(), static_cast<streamsize>(data.size()));

        if (!file)
        {
            throw runtime_error("Ошибка записи фигуры в файл");
        }

        file.close();

        if (!file)
        {
            throw runtime_error("Не удалось завершить сохранение фигуры");
        }
    }

    FigureData FigureFileService::Load(const filesystem::path& path)
    {
        ifstream file(path, ios::binary);

        if (!file.is_open())
        {
            throw runtime_error("Не удалось открыть файл фигуры");
        }

        ostringstream buffer;
        buffer << file.rdbuf();

        if (file.bad())
        {
            throw runtime_error("Ошибка чтения файла фигуры");
        }

        if (buffer.str().empty())
        {
            throw runtime_error("Файл фигуры пуст");
        }

        return FigureSerializer::Deserialize(buffer.str());
    }
}