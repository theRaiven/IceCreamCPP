#include "ParametersPanel.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

namespace IceCream::App::Views
{
    void ParametersPanel::Layout()
    {
        if (!content_)
            return;
        HWND focus = GetFocus();
        std::map<HWND, RECT> desired;
        int y = 12, label = 20000;
        int right = width_ - 36;
        auto place = [&](int id, int x, int top, int width, int h = 28) {
            desired[Control(id)] = { x, top - scroll_, x + width, top - scroll_ + h };
            };
        auto text = [&](const wchar_t* s, int x, int top, int width, int h = 28) {
            int id = label++;
            SetText(id, s);
            place(id, x, top, width, h);
            };
        auto row = [&](const wchar_t* s, int id) {
            text(s, 20, y, 205);
            place(id, 230, y, right - 230);
            y += 38;
            };
        auto header = [&](int i, const wchar_t* title) {
            std::wstring s = expanded_[i] ? L"−  " : L"+  ";
            s += title;
            SetText(20 + i, s.c_str());
            place(20 + i, 12, y, right - 12, 36);
            y += 48;
            return expanded_[i];
            };
        if (!appearanceTab_)
        {
            if (activePage_ == 0)
            {
                place(902, 20, y, right - 20, 160);
                y += 44;
                place(900, 20, y, right - 20, 34);
                y += 42;
                place(801, 20, y, right - 20, 48);
                y += 54;
                if (geometryMode_ == 2)
                {
                    text(L"Система неравенств (каждое с новой строки или через запятую)", 20, y, right - 20, 48);
                    y += 52;
                    place(918, 20, y, right - 20, 116);
                    y += 124;
                    text(L"Строится вся граница области, включая верх, низ и боковые стенки.", 20, y, right - 20, 44);
                    y += 50;
                }
                else
                {
                    SetText(916, constraintsOpen_ ? L"− Ограничения (для всей поверхности)" : L"+ Ограничения (для всей поверхности)");
                    place(916, 20, y, right - 20, 32);
                    y += 42;
                    if (constraintsOpen_)
                    {
                        place(903, 20, y, right - 20, 82);
                        y += 90;
                        text(L"Условия по X, Y, Z: по одному на строку или через запятую. Пусто — без обрезки.", 20, y, right - 20, 48);
                        y += 58;
                        place(917, 20, y, right - 20, 30);
                        y += 40;
                    }
                }
                if (geometryMode_ != 0)
                {
                    if (geometryMode_ == 1)
                    {
                        text(L"Уравнение поверхности", 20, y, right - 20);
                        y += 28;
                        place(904, 20, y, right - 68, 30);
                        place(914, right - 38, y, 38, 30);
                        y += 42;
                    }
                    else
                    {
                        text(L"Цвет поверхности", 20, y, right - 68, 30);
                        place(914, right - 38, y, 38, 30);
                        y += 42;
                    }
                    if (geometryMode_ != 0)
                    {
                        place(919, 20, y, right - 20, 28);
                        y += 36;
                    }
                    text(L"Ось", 20, y, 36);
                    text(L"От", 60, y, 100);
                    text(L"До", 172, y, 100);
                    text(L"Ячеек", 284, y, right - 284);
                    y += 30;
                    for (int axis = 0; axis < 3; ++axis)
                    {
                        text(axis == 0 ? L"X" : axis == 1 ? L"Y" : L"Z", 20, y, 36);
                        place(905 + 3 * axis, 60, y, 100);
                        place(906 + 3 * axis, 172, y, 100);
                        place(907 + 3 * axis, 284, y, right - 284);
                        y += 38;
                    }
                    if (geometryMode_ != 0)
                    {
                        place(920, 20, y, right - 20, 32);
                        y += 40;
                    }
                    text(geometryMode_ == 1 ? L"Например: x^2+y^2+z^2=9. Автоподбор ищет нулевой уровень и может расширить область поиска." :
                        L"Пример: y^2<=2*x+2 и 0<=z<=4-x. Поля X/Y/Z задают начальную область поиска; автоподбор при необходимости расширит её.",
                        20, y, right - 20, 66);
                    y += 76;
                }
                else
                {
                    for (size_t part = 0; part < drafts_.size(); ++part)
                    {
                        const auto& p = drafts_[part];
                        int id = 1000 + static_cast<int>(part) * 32;
                        place(id, 20, y, right - 68, 30);
                        place(id + 11, right - 38, y, 38, 30);
                        y += 42;
                        place(id + 2, 20, y, right - 20, 180);
                        y += 42;
                        const bool graph = p.Definition == Core::Services::PartDefinition::Function;
                        place(id + 14, 20, y, right - 20, 160);
                        y += 42;
                        text(graph ? (p.DependentAxis == 0 ? L"x ="
                            : p.DependentAxis == 1 ? L"y ="
                            : L"z =")
                            : L"f =",
                            20, y, 35);
                        place(id + 1, 60, y, right - 108);
                        place(id + 10, right - 38, y, 38);
                        y += 40;
                        if (graph)
                        {
                            place(id + 15, 20, y, right - 20, 120);
                            y += 42;
                        }
                        if (!graph)
                        {
                            place(id + 3, 20, y, right - 20, 160);
                            y += 42;
                        }
                        text(L"Ось", 20, y, 40);
                        text(L"От", 65, y, (right - 80) / 2);
                        text(L"До", 75 + (right - 80) / 2, y, (right - 80) / 2);
                        y += 30;
                        const wchar_t* cart[] = { L"x", L"y", L"z" };
                        const wchar_t* radial[] = { L"r", L"phi", L"z" };
                        int dimensions = graph || p.Coordinates == Core::Services::CoordinateSystem::Polar ? 2 : 3;
                        auto order = graph ? p.FunctionOrder() : p.Order;
                        for (int d = 0; d < dimensions; ++d)
                        {
                            int k = order[d];
                            text(p.Coordinates == Core::Services::CoordinateSystem::Cartesian ? cart[k] : radial[k], 20, y, 40);
                            place(id + 4 + 2 * k, 65, y, (right - 80) / 2);
                            place(id + 5 + 2 * k, 75 + (right - 80) / 2, y, (right - 80) / 2);
                            y += 38;
                        }
                        if (graph)
                        {
                            int k = p.DependentAxis;
                            text(p.Coordinates == Core::Services::CoordinateSystem::Cartesian ? cart[k] : radial[k], 20, y, 40);
                            place(id + 4 + 2 * k, 65, y, (right - 80) / 2);
                            place(id + 5 + 2 * k, 75 + (right - 80) / 2, y, (right - 80) / 2);
                            y += 38;
                        }
                        place(id + 12, 20, y, right - 20, 72);
                        y += 90;
                    }
                    place(901, 20, y, right - 20, 32);
                    y += 46;
                }
            }
            if (activePage_ == 1 && header(1, L"Сечение"))
            {
                for (int i = 0; i < 3; ++i)
                {
                    place(241 + i, 20 + i * (right - 20) / 3, y, (right - 28) / 3, 30);
                }
                y += 44;
                const wchar_t* names[] = { L"Положение X",  L"Положение Y",  L"Положение Z",
                                          L"Поворот X, °", L"Поворот Y, °", L"Поворот Z, °" };
                for (int i = 0; i < 6; ++i)
                {
                    row(names[i], 200 + i);
                    place(220 + i, 20, y - 6, right - 20, 26);
                    y += 32;
                }
                place(240, 20, y, right - 20, 32);
                y += 42;
                place(244, 20, y, right - 20, 32);
                y += 42;
                place(245, 20, y, right - 20, 32);
                y += 50;
            }
            if (activePage_ == 2 && header(2, L"Свет"))
            {
                place(300, 20, y, right - 20, 150);
                y += 44;
                const wchar_t* names[] = { L"Фоновая интенсивность", L"Собственное свечение", L"Сила источника",
                                          L"Положение X",           L"Положение Y",          L"Положение Z",
                                          L"Направление X",         L"Направление Y",        L"Направление Z",
                                          L"Угол прожектора, °",    L"Концентрация пучка" };
                int type = static_cast<int>(SendMessageW(Control(300), CB_GETCURSEL, 0, 0));
                for (int i = 0; i < 11; ++i)
                    if (i < 3 || (i < 6 && type != 0) || (i >= 6 && i < 9 && type != 1) || (i >= 9 && type == 2))
                        row(names[i], 310 + i);
                place(330, 20, y, right - 20, 32);
                y += 48;
            }
        }
        else if (header(3, L"Вид и цвета"))
        {
            place(400, 20, y, right - 20, 120);
            y += 44;
            place(401, 20, y, right - 20);
            y += 42;
            wchar_t b[80];
            swprintf_s(b, L"Размер подсказок: %d px", model_->Appearance().HintSize);
            text(b, 20, y, right - 20);
            y += 28;
            place(402, 20, y, right - 20, 28);
            y += 44;
            swprintf_s(b, L"Размер координат: %d px", model_->Appearance().CoordinateSize);
            text(b, 20, y, right - 20);
            y += 28;
            place(403, 20, y, right - 20, 28);
            y += 48;
            const wchar_t* names[] = { L"Фон 3D",          L"Фон сечения",    L"Фон панели",     L"Текст",
                                      L"Сетка",           L"Ползунки",       L"Поверхность",    L"Рожок / часть 1",
                                      L"Шапка / часть 2", L"Контур сечения", L"Границы частей", L"Ось X",
                                      L"Ось Y",           L"Ось Z" };
            for (int i = 0; i < 14; ++i)
            {
                if (i >= 6 && i <= 8)
                    continue;
                text(names[i], 20, y, right - 80);
                place(500 + i, right - 64, y, 64, 28);
                y += 40;
            }
        }
        if (model_->Appearance().ShowHints)
        {
            SetText(800, appearanceTab_ ? L"Нажмите на образец цвета, чтобы открыть палитру. Размер координат "
                L"изменяет буквы осей и числа делений."
                : L"Enter применяет ввод, Esc возвращает принятые значения. Tab переключает "
                L"поля. Колесо прокручивает панель. Функции задаются с явным умножением, "
                L"например sin(x)*cos(y). Сечение задано в мировых координатах.");
            place(800, 20, y, right - 20, model_->Appearance().HintSize * 9);
            y += model_->Appearance().HintSize * 9 + 16;
        }
        if (activePage_ != 0)
        {
            place(801, 20, y, right - 20, 70);
            y += 82;
        }
        contentHeight_ = y;
        int maxScroll = std::max(0, y - (height_ - 52));
        if (scroll_ > maxScroll)
        {
            scroll_ = maxScroll;
            Layout();
            return;
        }
        SCROLLINFO info{ sizeof(info),
                        SIF_RANGE | SIF_PAGE | SIF_POS,
                        0,
                        std::max(0, y - 1),
                        static_cast<UINT>(std::max(1, height_ - 52)),
                        scroll_,
                        0 };
        SetScrollInfo(content_, SB_VERT, &info, TRUE);
        struct Move
        {
            HWND Window;
            RECT Bounds;
            UINT Flags;
        };
        std::vector<Move> moves;
        for (HWND child : controls_)
        {
            const bool visible = (GetWindowLongPtrW(child, GWL_STYLE) & WS_VISIBLE) != 0;
            auto target = desired.find(child);
            if (target == desired.end())
            {
                if (visible)
                    moves.push_back({ child, {}, SWP_HIDEWINDOW | SWP_NOMOVE | SWP_NOSIZE });
                continue;
            }
            auto previous = positions_.find(child);
            if (!visible || previous == positions_.end() || !EqualRect(&previous->second, &target->second))
                moves.push_back({ child, target->second, visible ? 0u : SWP_SHOWWINDOW });
        }
        HDWP batch = BeginDeferWindowPos(static_cast<int>(moves.size()));
        for (const auto& move : moves)
        {
            if (!batch)
                break;
            batch = DeferWindowPos(batch, move.Window, nullptr, move.Bounds.left, move.Bounds.top,
                move.Bounds.right - move.Bounds.left, move.Bounds.bottom - move.Bounds.top,
                SWP_NOACTIVATE | SWP_NOZORDER | move.Flags);
        }
        if (batch)
            EndDeferWindowPos(batch);
        else
            for (const auto& move : moves)
                SetWindowPos(move.Window, nullptr, move.Bounds.left, move.Bounds.top, move.Bounds.right - move.Bounds.left,
                    move.Bounds.bottom - move.Bounds.top, SWP_NOACTIVATE | SWP_NOZORDER | move.Flags);
        positions_ = std::move(desired);
        if (focus && GetFocus() != focus && IsWindowVisible(focus))
            SetFocus(focus);
        if (!moves.empty())
            InvalidateRect(content_, nullptr, FALSE);
    }
    void ParametersPanel::ApplyShape()
    {
        struct WaitCursor
        {
            HCURSOR Previous = SetCursor(LoadCursor(nullptr, IDC_WAIT));
            ~WaitCursor() { SetCursor(Previous); }
        } cursor;
        try
        {
            StoreDrafts();
            const auto read = [&](int id) {
                int length = GetWindowTextLengthW(Control(id));
                std::wstring wide(length + 1, L'\0');
                GetWindowTextW(Control(id), wide.data(), length + 1);
                int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
                std::string text(size, ' ');
                WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, text.data(), size, nullptr, nullptr);
                return text;
                };
            if (geometryMode_ != 0)
            {
                double low[3], high[3];
                int cells[3];
                for (int axis = 0; axis < 3; ++axis)
                {
                    low[axis] = Number(905 + 3 * axis);
                    high[axis] = Number(906 + 3 * axis);
                    if (low[axis] >= high[axis]) throw std::invalid_argument("Граница «От» должна быть меньше «До» по каждой оси.");
                    double count = Number(907 + 3 * axis);
                    if (count < 2 || count > 128 || count != std::floor(count))
                        throw std::invalid_argument("Ячейки: целые числа от 2 до 128 по каждой оси.");
                    cells[axis] = static_cast<int>(count);
                }
                if (geometryMode_ == 1)
                {
                    Core::Geometry::Point3D fittedMin{ low[0], low[1], low[2] };
                    Core::Geometry::Point3D fittedMax{ high[0], high[1], high[2] };
                    bool autoFit = SendMessageW(Control(919), BM_GETCHECK, 0, 0) == BST_CHECKED;
                    std::string formula = read(904);
                    if (autoFit) model_->EstimateImplicitBounds(formula, fittedMin, fittedMax);
                    model_->ApplyImplicitSurface(formula, fittedMin, fittedMax, cells[0], cells[1], cells[2], read(903));
                    if (autoFit)
                    {
                        double a[] = { fittedMin.X, fittedMin.Y, fittedMin.Z };
                        double b[] = { fittedMax.X, fittedMax.Y, fittedMax.Z };
                        for (int axis = 0; axis < 3; ++axis)
                        {
                            SetNumber(905 + 3 * axis, a[axis]);
                            SetNumber(906 + 3 * axis, b[axis]);
                        }
                    }
                }
                else
                {
                    Core::Geometry::Point3D fittedMin{ low[0], low[1], low[2] };
                    Core::Geometry::Point3D fittedMax{ high[0], high[1], high[2] };
                    bool autoFit = SendMessageW(Control(919), BM_GETCHECK, 0, 0) == BST_CHECKED;
                    if (autoFit) model_->EstimateInequalityBounds(read(918), fittedMin, fittedMax);
                    model_->ApplyInequalityBody(read(918), fittedMin, fittedMax, cells[0], cells[1], cells[2]);
                    if (autoFit)
                    {
                        double a[] = { fittedMin.X, fittedMin.Y, fittedMin.Z };
                        double b[] = { fittedMax.X, fittedMax.Y, fittedMax.Z };
                        for (int axis = 0; axis < 3; ++axis)
                        {
                            SetNumber(905 + 3 * axis, a[axis]);
                            SetNumber(906 + 3 * axis, b[axis]);
                        }
                    }
                }
            }
            else
            {
                model_->ApplyIntegralParts(drafts_, read(903));
                LoadDrafts();
            }
            revision_ = model_->GeometryRevision();
            SetError(L"");
            Notify();
        }
        catch (const std::exception& e)
        {
            std::string message = e.what();
            int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
            std::wstring wide(size, L' ');
            MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide.data(), size);
            SetError(wide.c_str());
            Scroll(0);
        }
    }
    void ParametersPanel::EstimateBounds()
    {
        try
        {
            int formulaId = geometryMode_ == 1 ? 904 : 918;
            int length = GetWindowTextLengthW(Control(formulaId));
            std::wstring wide(length + 1, L'\0');
            GetWindowTextW(Control(formulaId), wide.data(), length + 1);
            int lengthUtf8 = WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
            std::string expression(lengthUtf8, ' ');
            WideCharToMultiByte(CP_UTF8, 0, wide.data(), length, expression.data(), lengthUtf8, nullptr, nullptr);

            Core::Geometry::Point3D min{ Number(905), Number(908), Number(911) };
            Core::Geometry::Point3D max{ Number(906), Number(909), Number(912) };
            if (geometryMode_ == 1) model_->EstimateImplicitBounds(expression, min, max);
            else model_->EstimateInequalityBounds(expression, min, max);

            double a[] = { min.X, min.Y, min.Z };
            double b[] = { max.X, max.Y, max.Z };
            for (int axis = 0; axis < 3; ++axis)
            {
                SetNumber(905 + 3 * axis, a[axis]);
                SetNumber(906 + 3 * axis, b[axis]);
            }
            SetError(L"");
        }
        catch (const std::exception& e)
        {
            std::string message = e.what();
            int size = MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, nullptr, 0);
            std::wstring wide(size, L' ');
            MultiByteToWideChar(CP_UTF8, 0, message.c_str(), -1, wide.data(), size);
            SetError(wide.c_str());
        }
    }

    void ParametersPanel::ApplySection()
    {
        try
        {
            double values[6];
            for (int i = 0; i < 6; ++i)
            {
                values[i] = Number(200 + i);
                double limit = i < 3 ? ViewModels::MainViewModel::SectionPositionLimit : 180;
                if (std::abs(values[i]) > limit)
                    throw std::out_of_range("Section range");
            }
            model_->SetSectionPose({ values[0], values[1], values[2] }, { values[3], values[4], values[5] });
            RefreshSectionRanges();
            SetError(L"");
            Notify();
        }
        catch (const std::exception&)
        {
            SetError(L"Положение: −1000…1000; поворот: −180…180 градусов.");
        }
    }
    void ParametersPanel::Command(int id, int notification)
    {
        if (id == 920 && notification == BN_CLICKED)
        {
            EstimateBounds();
            return;
        }
        if (id == 245)
        {
            if (resetSectionView_) resetSectionView_();
            Notify();
            return;
        }
        if (id == 917)
        {
            auto appearance = model_->Appearance();
            appearance.ShowConstraintSurfaces = SendMessageW(Control(917), BM_GETCHECK, 0, 0) == BST_CHECKED;
            model_->SetAppearance(appearance);
            Notify();
            return;
        }
        if (id == 902 && notification == CBN_SELCHANGE)
        {
            StoreDrafts();
            geometryMode_ = static_cast<int>(SendMessageW(Control(902), CB_GETCURSEL, 0, 0));
            scroll_ = 0;
            SetError(L"");
            Layout();
            return;
        }
        if (id == 914)
        {
            EditColor(static_cast<int>(ViewModels::ColorRole::Body));
            InvalidateRect(Control(914), nullptr, FALSE);
            return;
        }
        if (id == 916)
        {
            constraintsOpen_ = !constraintsOpen_;
            Layout();
            return;
        }
        if (id >= 10 && id <= 13)
        {
            StoreDrafts();
            bool same = activePage_ == id - 10;
            panelOpen_ = same ? !panelOpen_ : true;
            activePage_ = id - 10;
            appearanceTab_ = activePage_ == 3;
            expanded_ = { {false, false, false, true} };
            if (activePage_ < 3)
                expanded_[activePage_] = true;
            scroll_ = 0;
            Layout();
            Notify();
            return;
        }
        if (id >= 20 && id < 24)
        {
            panelOpen_ = false;
            Notify();
            return;
        }
        if (id == 900 || id == 901 || id >= 1000)
        {
            IntegralCommand(id, notification);
            return;
        }
        if (id == 240)
        {
            ApplySection();
            return;
        }
        if (id >= 241 && id <= 243)
        {
            model_->SetSectionPreset(id == 241 ? 2 : id == 242 ? 1 : 0);
            const auto& r = model_->SectionRotation();
            double v[] = { r.X, r.Y, r.Z };
            for (int i = 0; i < 3; ++i)
            {
                SetNumber(203 + i, v[i]);
                SendMessageW(Control(223 + i), TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(v[i] * 10)));
            }
            Notify();
            return;
        }
        if (id == 244)
        {
            model_->ToggleTransparentSide();
            SetWindowTextW(Control(244),
                model_->TransparentPositive() ? L"Прозрачная сторона: +" : L"Прозрачная сторона: −");
            Notify();
            return;
        }
        if (id == 300 && notification == CBN_SELCHANGE)
        {
            Layout();
            return;
        }
        if (id == 330)
        {
            ApplyLighting();
            return;
        }
        if (id == 400 && notification == CBN_SELCHANGE)
        {
            auto a = model_->Appearance();
            a.SetTheme(SendMessageW(Control(400), CB_GETCURSEL, 0, 0) == 1);
            model_->SetAppearance(a);
            Theme();
            Notify();
            return;
        }
        if (id == 401)
        {
            auto a = model_->Appearance();
            a.ShowHints = SendMessageW(Control(401), BM_GETCHECK, 0, 0) == BST_CHECKED;
            model_->SetAppearance(a);
            Layout();
            Notify();
            return;
        }
        if (id >= 500 && id < 514)
        {
            EditColor(id - 500);
            return;
        }
    }
    LRESULT CALLBACK ParametersPanel::EditProcedure(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR, DWORD_PTR data)
    {
        auto* self = reinterpret_cast<ParametersPanel*>(data);
        if (msg == WM_KEYDOWN)
        {
            if (w == 'A' && GetKeyState(VK_CONTROL) < 0)
            {
                SendMessageW(window, EM_SETSEL, 0, -1);
                return 0;
            }
            if (w == VK_RETURN)
            {
                int id = GetDlgCtrlID(window);
                if ((id == 903 || id == 918) && GetKeyState(VK_CONTROL) >= 0)
                    return DefSubclassProc(window, msg, w, l);
                if (id >= 900 || id < 200)
                    self->ApplyShape();
                else if (id < 300)
                    self->ApplySection();
                else
                    self->ApplyLighting();
                return 0;
            }
            if (w == VK_ESCAPE)
            {
                self->Sync();
                self->SetError(L"");
                return 0;
            }
            if (w == VK_TAB)
            {
                HWND next = GetNextDlgTabItem(self->content_, window, GetKeyState(VK_SHIFT) < 0);
                if (next)
                    SetFocus(next);
                return 0;
            }
        }
        if (msg == WM_CHAR && w == VK_RETURN && (GetDlgCtrlID(window) == 903 || GetDlgCtrlID(window) == 918) && GetKeyState(VK_CONTROL) >= 0)
            return DefSubclassProc(window, msg, w, l);
        if (msg == WM_CHAR && (w == VK_RETURN || w == VK_ESCAPE || w == VK_TAB))
            return 0;
        return DefSubclassProc(window, msg, w, l);
    }
} // namespace IceCream::App::Views
