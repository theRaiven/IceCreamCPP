#include "ParametersPanel.h"
#include "../../../IceCream.Core/Services/SaveData/FigureFileService.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <commdlg.h>
#include <filesystem>

#pragma comment(lib, "Comdlg32.lib")

using namespace std;
using namespace IceCream::Core::Services::SaveData;

namespace IceCream::App::Views
{
    namespace
    {
        string ReadUtf8(HWND window)
        {
            int length = GetWindowTextLengthW(window);
            wstring wide(length + 1, L'\0');

            int actual = GetWindowTextW(window, wide.data(), length + 1);
            int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), actual, nullptr, 0, nullptr, nullptr);

            string result(size, '\0');

            if (size > 0)
            {
                WideCharToMultiByte(CP_UTF8, 0, wide.data(), actual, result.data(), size, nullptr, nullptr);
            }

            return result;
        }
        wstring FromUtf8Figure(const string& text)
        {
            if (text.empty()) return L"";

            int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), nullptr, 0);

            if (size == 0)
                throw runtime_error("Некорректная кодировка текста в файле фигуры");

            wstring result(size, L'\0');

            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), result.data(), size);

            return result;
        }
    }

    FigureData ParametersPanel::CaptureFigure()
    {
        FigureData figure;

        figure.Type = static_cast<FigureType>(geometryMode_);

        switch (figure.Type)
        {
        case FigureType::Explicit:
            StoreDrafts();
            figure.Parts = drafts_;
            figure.Constraints = ReadUtf8(Control(903));
            break;

        case FigureType::Implicit:
            figure.Equation = ReadUtf8(Control(904));
            figure.Constraints = ReadUtf8(Control(903));
            break;

        case FigureType::Inequalities:
            figure.Constraints = ReadUtf8(Control(918));
            break;

        default:
            throw invalid_argument("Неизвестный тип фигуры");
        }

        if (figure.Type != FigureType::Explicit)
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                figure.Min[axis] = Number(905 + 3 * axis);
                figure.Max[axis] = Number(906 + 3 * axis);

                if (figure.Min[axis] >= figure.Max[axis])
                {
                    throw invalid_argument("Некорректные границы построения фигуры");
                }

                double cells = Number(907 + 3 * axis);

                if (cells < 2 || cells > 128 || cells != floor(cells))
                {
                    throw invalid_argument("Количество ячеек должно быть от 2 до 128");
                }

                figure.Cells[axis] = static_cast<int>(cells);
            }

            figure.AutoBounds = SendMessageW(Control(919), BM_GETCHECK, 0, 0) == BST_CHECKED;
        }

        const auto& color = model_->Appearance().Color(ViewModels::ColorRole::Body);
        figure.Color = { color.r, color.g, color.b, color.a };

        return figure;
    }

    void ParametersPanel::SaveFigure()
    {
        try
        {
            wchar_t filename[32768] = L"Figure.icefig";

            OPENFILENAMEW dialog{};

            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = parent_;
            dialog.lpstrFilter = L"Фигуры IceCream (*.icefig)\0*.icefig\0Все файлы (*.*)\0*.*\0\0";
            dialog.lpstrFile = filename;
            dialog.nMaxFile = sizeof(filename) / sizeof(filename[0]);
            dialog.lpstrDefExt = L"icefig";

            dialog.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST |
                OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

            if (!GetSaveFileNameW(&dialog))
            {
                if (CommDlgExtendedError() != 0)
                    throw runtime_error("Не удалось открыть диалог сохранения");

                return;
            }

            auto figure = CaptureFigure();

            Core::Services::SaveData::FigureFileService::Save(figure, filesystem::path(filename));

            SetError(L"Фигура успешно сохранена");
        }
        catch (const exception& error)
        {
            int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, error.what(), -1, nullptr, 0);

            wstring message;

            if (length > 0)
            {
                message.resize(length);
                MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, error.what(), -1, message.data(), length);
            }
            else
            {
                message = L"Не удалось сохранить фигуру";
            }

            MessageBoxW(parent_, message.c_str(), L"Ошибка сохранения", MB_OK | MB_ICONERROR);
        }
    }
    void ParametersPanel::OpenFigure()
    {
        try
        {
            wchar_t filename[32768]{};

            OPENFILENAMEW dialog{};

            dialog.lStructSize = sizeof(dialog);
            dialog.hwndOwner = parent_;
            dialog.lpstrFilter = L"Фигуры IceCream (*.icefig)\0*.icefig\0Все файлы (*.*)\0*.*\0\0";
            dialog.lpstrFile = filename;
            dialog.nMaxFile = sizeof(filename) / sizeof(filename[0]);
            dialog.lpstrDefExt = L"icefig";
            dialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

            if (!GetOpenFileNameW(&dialog))
            {
                if (CommDlgExtendedError() != 0)
                    throw runtime_error("Не удалось открыть окно выбора файла");

                return;
            }

            FigureData figure = FigureFileService::Load(filesystem::path(filename));

            if (figure.Type == FigureType::Explicit && (figure.Parts.empty() || figure.Parts.size() > 8))
                throw runtime_error("Количество частей фигуры должно быть от 1 до 8");

            if (figure.Type != FigureType::Explicit)
            {
                for (int cells : figure.Cells)
                    if (cells < 2 || cells > 128)
                        throw runtime_error("Количество ячеек должно быть от 2 до 128");
            }

            wstring equation = FromUtf8Figure(figure.Equation);
            wstring constraints = FromUtf8Figure(figure.Constraints);

            for (const auto& part : figure.Parts)
            {
                FromUtf8Figure(part.Name);
                FromUtf8Figure(part.Function);
                FromUtf8Figure(part.Base);
                FromUtf8Figure(part.DependentLower);
                FromUtf8Figure(part.DependentUpper);

                for (const auto& value : part.Lower) FromUtf8Figure(value);
                for (const auto& value : part.Upper) FromUtf8Figure(value);
            }

            auto appearance = model_->Appearance();

            appearance.SetColor(ViewModels::ColorRole::Body,
                { figure.Color[0], figure.Color[1], figure.Color[2], figure.Color[3] });

            // Сначала пробуем построить фигуру.
            switch (figure.Type)
            {
            case FigureType::Explicit:
                model_->ApplyIntegralParts(figure.Parts, figure.Constraints);
                break;

            case FigureType::Implicit:
            case FigureType::Inequalities:
            {
                Core::Geometry::Point3D min{ figure.Min[0], figure.Min[1], figure.Min[2] };
                Core::Geometry::Point3D max{ figure.Max[0], figure.Max[1], figure.Max[2] };

                if (figure.Type == FigureType::Implicit)
                    model_->ApplyImplicitSurface(figure.Equation, min, max,
                        figure.Cells[0], figure.Cells[1], figure.Cells[2], figure.Constraints);
                else
                    model_->ApplyInequalityBody(figure.Constraints, min, max,
                        figure.Cells[0], figure.Cells[1], figure.Cells[2]);

                break;
            }

            default:
                throw runtime_error("Неизвестный тип фигуры");
            }

            model_->SetAppearance(appearance);

            // Построение успешно. Теперь восстанавливаем интерфейс.
            syncing_ = true;

            geometryMode_ = static_cast<int>(figure.Type);
            SendMessageW(Control(902), CB_SETCURSEL, geometryMode_, 0);

            SetText(904, equation.c_str());
            SetText(903, figure.Type == FigureType::Inequalities ? L"" : constraints.c_str());
            SetText(918, figure.Type == FigureType::Inequalities ? constraints.c_str() : L"");

            SendMessageW(Control(919), BM_SETCHECK,
                figure.AutoBounds ? BST_CHECKED : BST_UNCHECKED, 0);

            if (figure.Type == FigureType::Explicit)
            {
                drafts_ = figure.Parts;
                LoadDrafts();
                syncing_ = true;
            }
            else
            {
                for (int axis = 0; axis < 3; ++axis)
                {
                    SetNumber(905 + 3 * axis, figure.Min[axis]);
                    SetNumber(906 + 3 * axis, figure.Max[axis]);
                    SetNumber(907 + 3 * axis, figure.Cells[axis]);
                }
            }

            activePage_ = 0;
            appearanceTab_ = false;
            expanded_ = { {true, false, false, true} };
            constraintsOpen_ = !constraints.empty() && figure.Type != FigureType::Inequalities;
            scroll_ = 0;
            panelOpen_ = true;

            Sync();
            syncing_ = false;

            SetViewport(viewportX_, viewportY_, viewportWidth_, viewportHeight_);
            SetError(L"Фигура успешно загружена");
            Layout();

            InvalidateRect(Control(914), nullptr, FALSE);
            Notify();
        }
        catch (const exception&)
        {
            syncing_ = false;

            MessageBoxW(parent_,
                L"Не удалось загрузить фигуру.\nФайл повреждён, содержит некорректные параметры или не может быть построен.",
                L"Ошибка загрузки",
                MB_OK | MB_ICONERROR);
        }
    }
}