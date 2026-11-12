#include "ParametersPanel.h"
#include <algorithm>
#include <cctype>
namespace IceCream::App::Views
{
namespace
{
using Core::Services::CoordinateSystem;
using Core::Services::IntegralPart;
using Core::Services::PartDefinition;
const std::array<std::array<int, 3>, 6> orders{
    {{{0, 1, 2}}, {{0, 2, 1}}, {{1, 0, 2}}, {{1, 2, 0}}, {{2, 0, 1}}, {{2, 1, 0}}}};
std::string Read(HWND window)
{
    int n = GetWindowTextLengthW(window);
    std::wstring text(n + 1, L' ');
    GetWindowTextW(window, text.data(), n + 1);
    int size = WideCharToMultiByte(CP_UTF8, 0, text.data(), n, nullptr, 0, nullptr, nullptr);
    std::string out(size, ' ');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), n, out.data(), size, nullptr, nullptr);
    return out;
}
std::wstring Wide(const std::string &text)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    std::wstring out(n, L' ');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, out.data(), n);
    return out;
}
void Write(HWND window, const std::string &text)
{
    SetWindowTextW(window, Wide(text).c_str());
}

} // namespace
void ParametersPanel::StoreDrafts()
{
    for (size_t i = 0; i < drafts_.size(); ++i)
    {
        auto &p = drafts_[i];
        int id = 1000 + static_cast<int>(i) * 32;
        p.Name = Read(Control(id));
        const auto formula = Read(Control(id + 1));
        p.Coordinates = static_cast<CoordinateSystem>(SendMessageW(Control(id + 2), CB_GETCURSEL, 0, 0));
        int mode = static_cast<int>(SendMessageW(Control(id + 14), CB_GETCURSEL, 0, 0));
        bool cart = p.Coordinates == CoordinateSystem::Cartesian;
        p.Definition = mode == (cart ? 3 : 1) ? PartDefinition::Integral : PartDefinition::Function;
        if (p.Definition == PartDefinition::Function)
        {
            int fill = static_cast<int>(SendMessageW(Control(id + 15), CB_GETCURSEL, 0, 0));
            p.AutoUpper = fill == 1;
        }
        p.SetFunction(formula);
        p.DependentAxis = !cart || mode == 0 ? 2 : mode == 1 ? 0 : 1;
        if (p.Definition == PartDefinition::Function)
            p.Order = p.FunctionOrder();
        else
        {
            int order = static_cast<int>(SendMessageW(Control(id + 3), CB_GETCURSEL, 0, 0));
            if (order >= 0)
                p.Order = p.Coordinates == CoordinateSystem::Polar
                              ? (order ? std::array<int, 3>{{1, 0, 2}} : std::array<int, 3>{{0, 1, 2}})
                              : orders.at(order);
        }
        for (int k = 0; k < 3; ++k)
        {
            if (p.Definition == PartDefinition::Function && k == p.DependentAxis)
            {
                p.DependentLower = Read(Control(id + 4 + 2 * k));
                p.DependentUpper = Read(Control(id + 5 + 2 * k));
            }
            else
            {
                p.Lower[k] = Read(Control(id + 4 + 2 * k));
                p.Upper[k] = Read(Control(id + 5 + 2 * k));
            }
        }
    }
}
void ParametersPanel::LoadDrafts()
{
    syncing_ = true;
    for (size_t i = 0; i < drafts_.size(); ++i)
    {
        const auto &p = drafts_[i];
        int id = 1000 + static_cast<int>(i) * 32;
        Write(Control(id), p.Name);
        Write(Control(id + 1), p.Function);
        SendMessageW(Control(id + 15), CB_RESETCONTENT, 0, 0);
        for (auto name : { L"Объём под графиком (до основания)", L"Автообъём (плоскость — до нуля)" })
            SendMessageW(Control(id + 15), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        SendMessageW(Control(id + 15), CB_SETCURSEL, p.AutoUpper ? 1 : 0, 0);
        SendMessageW(Control(id + 2), CB_SETCURSEL, static_cast<int>(p.Coordinates), 0);
        SendMessageW(Control(id + 14), CB_RESETCONTENT, 0, 0);
        if (p.Coordinates == CoordinateSystem::Cartesian)
        {
            for (auto name : {L"z = f(x, y)", L"x = f(y, z)", L"y = f(x, z)", L"Интеграл — дополнительно"})
                SendMessageW(Control(id + 14), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
            SendMessageW(Control(id + 14), CB_SETCURSEL,
                         p.Definition == PartDefinition::Integral ? 3
                         : p.DependentAxis == 2                   ? 0
                         : p.DependentAxis == 0                   ? 1
                                                                  : 2,
                         0);
        }
        else
        {
            for (auto name : {L"z = f(r, phi)", L"Интеграл — дополнительно"})
                SendMessageW(Control(id + 14), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
            SendMessageW(Control(id + 14), CB_SETCURSEL, p.Definition == PartDefinition::Integral ? 1 : 0, 0);
        }
        SendMessageW(Control(id + 3), CB_RESETCONTENT, 0, 0);
        const char *cart[] = {"x", "y", "z"};
        const char *radial[] = {"r", "phi", "z"};
        auto names = p.Coordinates == CoordinateSystem::Cartesian ? cart : radial;
        int count = p.Coordinates == CoordinateSystem::Polar ? 2 : 6, selected = 0;
        for (int j = 0; j < count; ++j)
        {
            auto order = p.Coordinates == CoordinateSystem::Polar
                             ? (j ? std::array<int, 3>{{1, 0, 2}} : std::array<int, 3>{{0, 1, 2}})
                             : orders[j];
            std::string title = "Порядок интегрирования: ";
            int dims = p.Coordinates == CoordinateSystem::Polar ? 2 : 3;
            for (int d = 0; d < dims; ++d)
            {
                if (d)
                    title += " → ";
                title += names[order[d]];
            }
            SendMessageW(Control(id + 3), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(Wide(title).c_str()));
            if (order == p.Order)
                selected = j;
        }
        SendMessageW(Control(id + 3), CB_SETCURSEL, selected, 0);
        for (int k = 0; k < 3; ++k)
        {
            bool dependent = p.Definition == PartDefinition::Function && k == p.DependentAxis;
            Write(Control(id + 4 + 2 * k), dependent ? p.DependentLower : p.Lower[k]);
            Write(Control(id + 5 + 2 * k), dependent ? p.DependentUpper : p.Upper[k]);
        }
        UpdateIntegralPreview(i);
    }
    syncing_ = false;
}
void ParametersPanel::UpdateIntegralPreview(size_t part)
{
    const auto &p = drafts_.at(part);
    int id = 1000 + static_cast<int>(part) * 32;
    const char *cart[] = {"x", "y", "z"};
    const char *radial[] = {"r", "phi", "z"};
    auto names = p.Coordinates == CoordinateSystem::Cartesian ? cart : radial;
    if (p.Definition == PartDefinition::Function)
    {
        std::string preview = "Пределы задают область двух независимых координат.";
        if (p.AutoUpper)
            preview += "\nЕсли график постоянный, объём до нуля; иначе над графиком до максимума.";
        else
            preview += std::string("\nОбъём между графиком и ") + names[p.DependentAxis] + "=" + p.Base + ".";
        preview += "\n";
        preview += names[p.DependentAxis];
        preview += " от ";
        preview += p.DependentLower.empty() ? "без ограничения" : p.DependentLower;
        preview += ", до ";
        preview += p.DependentUpper.empty() ? "без ограничения" : p.DependentUpper;
        preview += ".";
        Write(Control(id + 12), preview);
        return;
    }
    std::string preview;
    int dims = p.Coordinates == CoordinateSystem::Polar ? 2 : 3;
    for (int d = 0; d < dims; ++d)
    {
        int k = p.Order[d];
        preview += "∫[" + p.Lower[k] + ", " + p.Upper[k] + "] d" + names[k] + " ";
    }
    preview += "(" + p.Function + ")";
    if (p.Coordinates != CoordinateSystem::Cartesian)
        preview += " · r";
    Write(Control(id + 12), preview);
}
void ParametersPanel::IntegralCommand(int id, int notification)
{
    if (id == 900)
    {
        ApplyShape();
        return;
    }
    if (id == 901)
    {
        StoreDrafts();
        if (drafts_.size() >= 8)
        {
            SetError(L"Можно задать до восьми частей.");
            return;
        }
        IntegralPart p;
        p.Name = "Часть " + std::to_string(drafts_.size() + 1);
        p.Definition = PartDefinition::Function;
        drafts_.push_back(p);
        LoadDrafts();
        Layout();
        Scroll(contentHeight_);
        return;
    }
    size_t part = (id - 1000) / 32;
    int field = (id - 1000) % 32;
    if (part >= drafts_.size())
        return;
    if (notification == EN_CHANGE && (field == 0 || field == 1 || (field >= 4 && field <= 9)))
    {
        StoreDrafts();
        UpdateIntegralPreview(part);
        return;
    }
    if (field == 10)
    {
        EditColor(100 + static_cast<int>(part));
        return;
    }
    if (field == 11)
    {
        StoreDrafts();
        if (drafts_.size() == 1)
        {
            SetError(L"Нужна хотя бы одна часть.");
            return;
        }
        drafts_.erase(drafts_.begin() + part);
        LoadDrafts();
        Layout();
        return;
    }
    if (field == 15 && notification == CBN_SELCHANGE)
    {
        StoreDrafts();
        UpdateIntegralPreview(part);
        return;
    }
    if ((field == 2 || field == 3 || field == 14) && notification == CBN_SELCHANGE)
    {
        const auto before = drafts_[part];
        StoreDrafts();
        auto &p = drafts_[part];
        if (field == 14)
        {
            if (before.Definition == PartDefinition::Function && p.Definition == PartDefinition::Integral)
            {
                auto definition = before;
                definition.Name = p.Name;
                definition.Function = p.Function;
                definition.Base = p.Base;
                definition.AboveFunction = p.AboveFunction;
                definition.AutoUpper = p.AutoUpper;
                definition.DependentLower = p.DependentLower;
                definition.DependentUpper = p.DependentUpper;
                p = Core::Services::IntegralBodyBuilder::AsIntegral(definition);
            }
            else if (before.Definition == PartDefinition::Function && p.Definition == PartDefinition::Function &&
                     before.DependentAxis != p.DependentAxis)
                p = Core::Services::IntegralBodyBuilder::WithDependentAxis(before, p.DependentAxis);
            else if (before.Definition == PartDefinition::Integral && p.Definition == PartDefinition::Function)
            {
                p.Function = p.Upper[p.DependentAxis];
                p.Base = p.Lower[p.DependentAxis];
                p.AboveFunction = false;
                p.AutoUpper = false;
                p.DependentLower.clear();
                p.DependentUpper.clear();
            }
        }
        if (field == 2)
        {
            p.Definition = before.Definition;
            p.DependentAxis = 2;
            p.Order = p.Definition == PartDefinition::Function         ? p.FunctionOrder()
                      : p.Coordinates == CoordinateSystem::Cylindrical ? std::array<int, 3>{{2, 1, 0}}
                                                                       : std::array<int, 3>{{0, 1, 2}};
            p.Lower = {{"0", "0", "0"}};
            p.Upper = p.Coordinates == CoordinateSystem::Cartesian ? std::array<std::string, 3>{{"1", "1", "1"}}
                                                                   : std::array<std::string, 3>{{"1", "2*pi", "1"}};
        }
        LoadDrafts();
        Layout();
        return;
    }
}
} // namespace IceCream::App::Views
