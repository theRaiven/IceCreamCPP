#include "ParametersPanel.h"
#include "../WindowLayout.h"
#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwctype>
#include <glm/gtc/type_ptr.hpp>

namespace IceCream::App::Views
{
    using ViewModels::ColorRole;
    namespace
    {
        HFONT CreatePanelFont(int size)
        {
            return CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                CLEARTYPE_QUALITY, 0, L"Segoe UI");
        }

        COLORREF Color(const glm::vec4& c)
        {
            return RGB(int(c.r * 255), int(c.g * 255), int(c.b * 255));
        }
        [[maybe_unused]] const wchar_t* colorNames[] = {
            L"Фон 3D",          L"Фон сечения",     L"Фон панели",     L"Текст",          L"Сетка", L"Ползунки", L"Поверхность",
            L"Рожок / часть 1", L"Шапка / часть 2", L"Контур сечения", L"Границы частей", L"Ось X", L"Ось Y",    L"Ось Z" };
    } // namespace
    HWND ParametersPanel::Make(const wchar_t* cls, const wchar_t* text, int id, DWORD style)
    {
        if (wcscmp(cls, L"BUTTON") == 0 && (style & BS_TYPEMASK) != BS_OWNERDRAW)
            style |= BS_MULTILINE;
        HWND w = CreateWindowExW(wcscmp(cls, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0, cls, text, WS_CHILD | style, 0, 0, 1, 1,
            content_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr),
            nullptr);
        if (!w)
            throw std::runtime_error("Cannot create panel control");
        SendMessageW(w, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        if (wcscmp(cls, L"EDIT") == 0)
        {
            int field = id >= 1000 ? (id - 1000) % 32 : -1;
            bool bound = field >= 4 && field <= 9;
            SendMessageW(w, EM_SETLIMITTEXT, id == 903 || id == 918 ? 4096 : bound ? 2048 : id >= 1000 || id == 904 ? 512 : 64, 0);
            SetWindowSubclass(w, EditProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
        }
        else if (wcscmp(cls, TRACKBAR_CLASSW) == 0)
            SetWindowSubclass(w, SliderProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
        else if (wcscmp(cls, L"COMBOBOX") == 0)
        {
            comboBoxes_.push_back(w);
            SetWindowSubclass(w, ComboProcedure, 1, 0);
        }
        controls_.push_back(w);
        return w;
    }
    HWND ParametersPanel::Control(int id) const
    {
        return GetDlgItem(content_, id);
    }
    void ParametersPanel::Initialize(ViewModels::MainViewModel& model, std::function<void()> changed, std::function<void()> resetSectionView)
    {
        model_ = &model;
        changed_ = std::move(changed);
        resetSectionView_ = std::move(resetSectionView);
        parent_ = WindowFromDC(wglGetCurrentDC());
        dpi_ = GetDpiForWindow(parent_);
        SetWindowLongPtrW(parent_, GWL_STYLE, GetWindowLongPtrW(parent_, GWL_STYLE) | WS_CLIPCHILDREN);
        INITCOMMONCONTROLSEX common{ sizeof(common), ICC_BAR_CLASSES };
        InitCommonControlsEx(&common);
        fontSize_ = Pixels(model_->Appearance().TextSize);
        font_ = CreatePanelFont(fontSize_);
        WNDCLASSW cls{};
        cls.lpfnWndProc = Procedure;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.lpszClassName = L"IceCream.Parameters";
        cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&cls);
        panel_ =
            CreateWindowExW(WS_EX_CONTROLPARENT | WS_EX_COMPOSITED, cls.lpszClassName, L"Параметры",
                WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 1, 1, parent_, nullptr, cls.hInstance, this);
        content_ = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"Блоки параметров",
            WS_CHILD | WS_VSCROLL | WS_CLIPCHILDREN, 0, 0, 1, 1, panel_, nullptr,
            cls.hInstance, this);
        constexpr std::array<const wchar_t*, PageCount> captions{L"Функции", L"Сечение", L"Свет", L"Вид", L"Помощь"};
        for (int i = 0; i < PageCount; ++i)
        {
            HWND b = CreateWindowW(L"BUTTON", captions[i],
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 1, 1, panel_,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(FirstTabId + i)), cls.hInstance, nullptr);
            SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
        }
        for (int i = 0; i < 6; ++i)
        {
            Make(L"EDIT", L"0", 200 + i, WS_TABSTOP | ES_AUTOHSCROLL);
            Make(TRACKBAR_CLASSW, L"", 220 + i, WS_TABSTOP | TBS_HORZ | TBS_NOTICKS);
            SendMessageW(Control(220 + i), TBM_SETRANGE, TRUE, MAKELPARAM(i < 3 ? -200 : -1800, i < 3 ? 200 : 1800));
        }
        Make(L"BUTTON", L"Применить плоскость", 240, WS_TABSTOP);
        for (int i = 0; i < 3; ++i)
            Make(L"BUTTON", i == 0 ? L"XY" : i == 1 ? L"XZ" : L"YZ", 241 + i, WS_TABSTOP);
        Make(L"BUTTON", L"Прозрачная сторона: +", 244, WS_TABSTOP);
        Make(L"BUTTON", L"Вписать сечение в окно", 245, WS_TABSTOP);
        Make(L"COMBOBOX", L"", 300, WS_TABSTOP | CBS_DROPDOWNLIST);
        for (auto s : { L"Направленный", L"Точечный", L"Прожектор" })
            SendMessageW(Control(300), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        for (int i = 0; i < 11; ++i)
            Make(L"EDIT", L"", 310 + i, WS_TABSTOP | ES_AUTOHSCROLL);
        Make(L"BUTTON", L"Применить свет", 330, WS_TABSTOP);
        Make(L"COMBOBOX", L"", 400, WS_TABSTOP | CBS_DROPDOWNLIST);
        for (auto s : { L"Тёмная тема", L"Светлая тема" })
            SendMessageW(Control(400), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
        SendMessageW(Control(400), CB_SETCURSEL, 0, 0);
        Make(L"BUTTON", L"Показывать подсказки", 401, WS_TABSTOP | BS_AUTOCHECKBOX);
        for (int i = 0; i < 3; ++i)
            Make(TRACKBAR_CLASSW, L"", 402 + i, WS_TABSTOP | TBS_HORZ | TBS_NOTICKS);
        SendMessageW(Control(402), TBM_SETRANGE, TRUE, MAKELPARAM(10, 24));
        SendMessageW(Control(402), TBM_SETPOS, TRUE, 14);
        SendMessageW(Control(403), TBM_SETRANGE, TRUE, MAKELPARAM(12, 36));
        SendMessageW(Control(403), TBM_SETPOS, TRUE, 20);
        SendMessageW(Control(404), TBM_SETRANGE, TRUE,
            MAKELPARAM(ViewModels::AppearanceSettings::MinTextSize, ViewModels::AppearanceSettings::MaxTextSize));
        SendMessageW(Control(404), TBM_SETPOS, TRUE, model_->Appearance().TextSize);
        for (int i = 0; i < 14; ++i)
            Make(L"BUTTON", colorNames[i], 500 + i, WS_TABSTOP | BS_OWNERDRAW);
        for (int i = 0; i < 256; ++i)
            Make(L"STATIC", L"", 20000 + i, SS_LEFT);
        Make(L"BUTTON", L"Построить", 900, WS_TABSTOP);
        Make(L"BUTTON", L"Сохранить фигуру", 921, WS_TABSTOP);
        Make(L"BUTTON", L"Открыть фигуру", 922, WS_TABSTOP);
        Make(L"BUTTON", L"+ Добавить часть", 901, WS_TABSTOP);
        Make(L"COMBOBOX", L"", 902, WS_TABSTOP | CBS_DROPDOWNLIST);
        for (auto name : { L"Функции и интегралы", L"Неявная поверхность", L"Тело по неравенствам" })
            SendMessageW(Control(902), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        SendMessageW(Control(902), CB_SETCURSEL, 0, 0);
        Make(L"EDIT", L"", 903, WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL);
        Make(L"EDIT", L"y^2<=2*x+2\r\n0<=z<=4-x", 918, WS_TABSTOP | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL);
        Make(L"BUTTON", L"Автоподбор границ", 919, WS_TABSTOP | BS_AUTOCHECKBOX);
        Make(L"BUTTON", L"Оценить границы", 920, WS_TABSTOP);
        Make(L"EDIT", L"x^2+y^2+z^2=9", 904, WS_TABSTOP | ES_AUTOHSCROLL);
        for (int axis = 0; axis < 3; ++axis)
        {
            Make(L"EDIT", L"-4", 905 + 3 * axis, WS_TABSTOP | ES_AUTOHSCROLL);
            Make(L"EDIT", axis == 1 ? L"4" : L"5", 906 + 3 * axis, WS_TABSTOP | ES_AUTOHSCROLL);
            Make(L"EDIT", L"40", 907 + 3 * axis, WS_TABSTOP | ES_AUTOHSCROLL);
        }
        Make(L"BUTTON", L"Цвет поверхности", 914, WS_TABSTOP | BS_OWNERDRAW);
        Make(L"BUTTON", L"+ Ограничения поверхности", 916, WS_TABSTOP);
        Make(L"BUTTON", L"Поверхности ограничений", 917, WS_TABSTOP | BS_AUTOCHECKBOX);
        SendMessageW(Control(917), BM_SETCHECK, BST_CHECKED, 0);
        for (int part = 0; part < 8; ++part)
        {
            int id = 1000 + part * 32;
            for (int k : {0, 1, 4, 5, 6, 7, 8, 9})
                Make(L"EDIT", L"", id + k, WS_TABSTOP | ES_AUTOHSCROLL);
            Make(L"COMBOBOX", L"", id + 2, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
            for (auto name : { L"Декартовы (x, y, z)", L"Цилиндрические", L"Полярные: z=f(r, phi)" })
                SendMessageW(Control(id + 2), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
            Make(L"COMBOBOX", L"", id + 3, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
            Make(L"BUTTON", L"Цвет функции", id + 10, WS_TABSTOP | BS_OWNERDRAW);
            Make(L"BUTTON", L"×", id + 11, WS_TABSTOP);
            Make(L"STATIC", L"", id + 12, SS_LEFT);
            Make(L"COMBOBOX", L"", id + 14, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
            Make(L"COMBOBOX", L"", id + 15, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
        }
        drafts_ = Core::Services::IntegralBodyBuilder::DefaultParts();
        model_->ApplyIntegralParts(drafts_);
        LoadDrafts();
        Make(L"STATIC", L"", 800, SS_LEFT);
        Make(L"STATIC", L"", 801, SS_LEFT);
        Theme();
        Sync();
    }
    void ParametersPanel::Destroy()
    {
        if (panel_)
        {
            KillTimer(panel_, 1);
            DestroyWindow(panel_);
        }
        panel_ = content_ = nullptr;
        if (font_)
            DeleteObject(font_);
        if (hintFont_)
            DeleteObject(hintFont_);
        if (tabFont_)
            DeleteObject(tabFont_);
        if (background_)
            DeleteObject(background_);
        if (fieldBrush_)
            DeleteObject(fieldBrush_);
        controls_.clear();
        comboBoxes_.clear();
        positions_.clear();
    }
    void ParametersPanel::Theme()
    {
        const auto& a = model_->Appearance();
        if (background_)
            DeleteObject(background_);
        if (fieldBrush_)
            DeleteObject(fieldBrush_);
        background_ = CreateSolidBrush(Color(a.Color(ColorRole::Panel)));
        fieldBrush_ = CreateSolidBrush(a.LightTheme ? RGB(255, 255, 255) : RGB(42, 49, 62));
        RefreshTextSizes();
        RedrawWindow(panel_, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    }
    void ParametersPanel::RefreshFonts()
    {
        const int textSize = model_->Appearance().TextSize;
        const int mainPixels = Pixels(textSize);
        if (fontSize_ != mainPixels || !font_)
        {
            if (HFONT next = CreatePanelFont(mainPixels))
            {
                const HWND hint = Control(800);
                for (HWND child : controls_)
                    if (child != hint)
                        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(next), FALSE);
                HDC dc = GetDC(content_);
                const auto previous = SelectObject(dc, next);
                TEXTMETRICW metric{};
                GetTextMetricsW(dc, &metric);
                SelectObject(dc, previous);
                ReleaseDC(content_, dc);
                for (HWND combo : comboBoxes_)
                {
                    SendMessageW(combo, CB_SETITEMHEIGHT, 0, metric.tmHeight + Pixels(4));
                    SendMessageW(combo, CB_SETITEMHEIGHT, static_cast<WPARAM>(-1), metric.tmHeight + Pixels(4));
                }
                DeleteObject(font_);
                font_ = next;
                fontSize_ = mainPixels;
            }
        }
        const int tabPixels = Pixels(std::clamp(Logical(viewportTabsWidth_) / 21, ViewModels::AppearanceSettings::MinTextSize, textSize));
        if (tabFontSize_ != tabPixels || !tabFont_)
        {
            if (HFONT next = CreatePanelFont(tabPixels))
            {
                for (int i = 0; i < PageCount; ++i)
                    SendMessageW(GetDlgItem(panel_, FirstTabId + i), WM_SETFONT, reinterpret_cast<WPARAM>(next), FALSE);
                DeleteObject(tabFont_);
                tabFont_ = next;
                tabFontSize_ = tabPixels;
            }
        }
        RefreshTextSizes();
    }
    void ParametersPanel::SetViewport(int width, int height, int tabsWidth, int dpi)
    {
        if (viewportWidth_ == width && viewportHeight_ == height && viewportTabsWidth_ == tabsWidth &&
            viewportOpen_ == panelOpen_ && dpi_ == dpi)
            return;
        viewportWidth_ = width;
        viewportHeight_ = height;
        viewportTabsWidth_ = tabsWidth;
        viewportOpen_ = panelOpen_;
        dpi_ = dpi;
        RefreshFonts();
        const int toolbar = std::min(height, Pixels(WindowLayout::ToolbarHeight));
        if (!panelOpen_)
        {
            if (IsChild(content_, GetFocus()))
                SetFocus(GetDlgItem(panel_, FirstTabId + activePage_));
            ShowWindow(content_, SW_HIDE);
        }
        MoveWindow(panel_, 0, 0, width, height, TRUE);
        const int margin = Pixels(8), gap = Pixels(4);
        const int space = std::max(0, tabsWidth - 2 * margin + gap);
        // Reserve more space for the longer captions, including on the narrow drawer.
        constexpr std::array<int, PageCount + 1> tabEdges{0, 23, 45, 61, 76, 100};
        for (int i = 0; i < PageCount; ++i)
        {
            const int left = margin + space * tabEdges[i] / tabEdges.back();
            const int right = margin + space * tabEdges[i + 1] / tabEdges.back() - gap;
            MoveWindow(GetDlgItem(panel_, FirstTabId + i), left, margin, std::max(0, right - left),
                std::max(0, toolbar - 2 * margin), TRUE);
        }
        MoveWindow(content_, 0, toolbar, width, height - toolbar, panelOpen_);
        RECT client{};
        GetClientRect(content_, &client);
        width_ = Logical(client.right);
        height_ = Logical(client.bottom);
        if (panelOpen_)
        {
            Layout();
            ShowWindow(content_, SW_SHOWNA);
        }
    }
    void ParametersPanel::Render(const ViewModels::MainViewModel&)
    {
        if (revision_ != model_->GeometryRevision())
        {
            Sync();
            geometryMode_ = model_->IsInequalityBody() ? 2 : model_->IsImplicitSurface() ? 1 : 0;
            SendMessageW(Control(902), CB_SETCURSEL, geometryMode_, 0);
            const auto& constraints = model_->Constraints();
            int size = MultiByteToWideChar(CP_UTF8, 0, constraints.c_str(), -1, nullptr, 0);
            std::wstring wide(size, L'\0');
            MultiByteToWideChar(CP_UTF8, 0, constraints.c_str(), -1, wide.data(), size);
            SetText(geometryMode_ == 2 ? 918 : 903, wide.c_str());
            if (model_->IsIntegralBody())
            {
                drafts_ = model_->IntegralParts();
                LoadDrafts();
            }
            Layout();
        }
    }
    void ParametersPanel::Notify()
    {
        changed_();
    }
    void ParametersPanel::SetError(const wchar_t* message)
    {
        if (status_ == message)
            return;
        status_ = message;
        SetText(801, message);
        Layout();
    }
    double ParametersPanel::Number(int id) const
    {
        wchar_t b[520]{};
        GetWindowTextW(Control(id), b, 520);
        for (auto& c : b)
            if (c == L',')
                c = L'.';
        wchar_t* end = nullptr;
        double n = wcstod(b, &end);
        while (end && iswspace(*end))
            ++end;
        if (end == b || !end || *end || !std::isfinite(n))
            throw std::invalid_argument("Invalid number");
        return n;
    }
    void ParametersPanel::SetNumber(int id, double n)
    {
        wchar_t b[64];
        swprintf_s(b, L"%.6g", n);
        SetText(id, b);
    }
    void ParametersPanel::Sync()
    {
        syncing_ = true;
        RefreshSectionRanges();
        SendMessageW(Control(917), BM_SETCHECK, model_->Appearance().ShowConstraintSurfaces ? BST_CHECKED : BST_UNCHECKED, 0);
        const auto& o = model_->SectionPosition();
        const auto& r = model_->SectionRotation();
        const double v[] = { o.X, o.Y, o.Z, r.X, r.Y, r.Z };
        for (int i = 0; i < 6; ++i)
        {
            SetNumber(200 + i, v[i]);
            SendMessageW(Control(220 + i), TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(v[i] * 10)));
        }
        const auto& l = model_->Lighting();
        SendMessageW(Control(300), CB_SETCURSEL, static_cast<int>(l.Type), 0);
        const float lv[] = { l.AmbientIntensity, l.EmissionIntensity, l.LightIntensity, l.Position.x,
                            l.Position.y,       l.Position.z,        l.Direction.x,    l.Direction.y,
                            l.Direction.z,      l.SpotAngle,         l.SpotExponent };
        for (int i = 0; i < 11; ++i)
            SetNumber(310 + i, lv[i]);
        revision_ = model_->GeometryRevision();
        syncing_ = false;
    }
    void ParametersPanel::RefreshSectionRanges()
    {
        std::array<double, 3> extent{ {0,0,0} };
        auto rotation = model_->SurfaceRotation();
        for (const auto& p : model_->Body().Mesh().Vertices())
        {
            auto q = rotation * glm::dvec3(p.X, p.Y, p.Z);
            for (int axis = 0; axis < 3; ++axis) extent[axis] = std::max(extent[axis], std::abs(q[axis]));
        }
        const auto& position = model_->SectionPosition();
        const double values[] = { position.X,position.Y,position.Z };
        for (int axis = 0; axis < 3; ++axis)
        {
            double range = std::clamp(std::ceil(std::max(extent[axis], std::abs(values[axis])) * 1.2 / 10) * 10,
                20.0, ViewModels::MainViewModel::SectionPositionLimit);
            int ticks = static_cast<int>(std::round(range * 10));
            if (ticks != sectionSliderRanges_[axis])
            {
                SendMessageW(Control(220 + axis), TBM_SETRANGEMIN, FALSE, -ticks);
                SendMessageW(Control(220 + axis), TBM_SETRANGEMAX, TRUE, ticks);
                sectionSliderRanges_[axis] = ticks;
            }
            SendMessageW(Control(220 + axis), TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(values[axis] * 10)));
        }
    }
    void ParametersPanel::SetText(int id, const wchar_t* value)
    {
        HWND window = Control(id);
        const int length = GetWindowTextLengthW(window);
        if (length == static_cast<int>(wcslen(value)))
        {
            std::wstring current(length + 1, L'\0');
            GetWindowTextW(window, current.data(), length + 1);
            if (wcscmp(current.c_str(), value) == 0)
                return;
        }
        SetWindowTextW(window, value);
    }
    void ParametersPanel::RefreshTextSizes()
    {
        const auto& a = model_->Appearance();
        const int size = Pixels(a.HintSize);
        if (hintFontSize_ != size || !hintFont_)
        {
            HFONT next = CreatePanelFont(size);
            SendMessageW(Control(800), WM_SETFONT, reinterpret_cast<WPARAM>(next), TRUE);
            if (hintFont_)
                DeleteObject(hintFont_);
            hintFont_ = next;
            hintFontSize_ = size;
        }
        if (activePage_ == AppearancePage)
        {
            wchar_t text[80];
            swprintf_s(text, L"Размер подсказок: %d px", a.HintSize);
            SetText(20000, text);
            swprintf_s(text, L"Размер координат: %d px", a.CoordinateSize);
            SetText(20001, text);
            swprintf_s(text, L"Основной текст: %d px", a.TextSize);
            SetText(20002, text);
        }
    }
    void ParametersPanel::QueueSlider(int id, bool finish)
    {
        if (syncing_)
            return;
        int value = static_cast<int>(SendMessageW(Control(id), TBM_GETPOS, 0, 0));
        if (id == 404)
        {
            wchar_t label[64];
            swprintf_s(label, L"Основной текст: %d px", value);
            SetText(20002, label);
            if (!finish && GetCapture() == Control(id))
                return;
            auto appearance = model_->Appearance();
            if (appearance.TextSize != value)
            {
                // Keep the size slider in view when changing it in a scrolled panel.
                if (scroll_ > 0)
                {
                    const auto bounds = positions_.find(Control(id));
                    if (bounds != positions_.end())
                    {
                        constexpr int standard = ViewModels::AppearanceSettings::DefaultTextSize;
                        const int top = Logical(bounds->second.top);
                        scroll_ = std::max(0, MulDiv(scroll_ + top, std::max(standard, value),
                            std::max(standard, appearance.TextSize)) - top);
                    }
                }
                appearance.TextSize = value;
                model_->SetAppearance(appearance);
                RefreshFonts();
                Layout();
                RedrawWindow(panel_, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
                Notify();
            }
            return;
        }
        if (id >= 220 && id < 226)
        {
            pendingSection_[id - 220] = value / 10.0;
            SetNumber(200 + id - 220, value / 10.0);
        }
        else if (id == 402)
            pendingHintSize_ = value;
        else if (id == 403)
            pendingCoordinateSize_ = value;
        else
            return;
        if (finish)
        {
            FlushSliders();
            if (id == 402 && model_->Appearance().ShowHints)
                Layout();
            return;
        }
        if (!sliderTimer_)
            sliderTimer_ = SetTimer(panel_, 1, 16, nullptr) != 0;
        if (!sliderTimer_)
            FlushSliders();
    }
    void ParametersPanel::FlushSliders()
    {
        KillTimer(panel_, 1);
        sliderTimer_ = false;
        auto pending = pendingSection_;
        pendingSection_ = {};
        bool changed = false;
        auto origin = model_->SectionPosition();
        auto rotation = model_->SectionRotation();
        double values[] = { origin.X, origin.Y, origin.Z, rotation.X, rotation.Y, rotation.Z };
        bool sectionChanged = false;
        for (int i = 0; i < 6; ++i)
            if (pending[i] && values[i] != *pending[i])
            {
                values[i] = *pending[i];
                sectionChanged = true;
            }
        if (sectionChanged)
        {
            try
            {
                model_->SetSectionPose({ values[0], values[1], values[2] }, { values[3], values[4], values[5] });
                SetError(L"");
                changed = true;
            }
            catch (const std::exception&)
            {
                SetError(L"Не удалось вычислить сечение на границе. Немного сдвиньте плоскость.");
            }
        }
        if (pendingHintSize_ || pendingCoordinateSize_)
        {
            auto appearance = model_->Appearance();
            if (pendingHintSize_)
            {
                changed = changed || appearance.HintSize != *pendingHintSize_;
                appearance.HintSize = *pendingHintSize_;
            }
            if (pendingCoordinateSize_)
            {
                changed = changed || appearance.CoordinateSize != *pendingCoordinateSize_;
                appearance.CoordinateSize = *pendingCoordinateSize_;
            }
            pendingHintSize_.reset();
            pendingCoordinateSize_.reset();
            model_->SetAppearance(appearance);
            RefreshTextSizes();
        }
        if (changed)
            Notify();
    }
    void ParametersPanel::Scroll(int position)
    {
        int next = std::clamp(position, 0, std::max(0, contentHeight_ - height_));
        if (next == scroll_)
            return;
        scroll_ = next;
        Layout();
    }
    void ParametersPanel::FitDropDown(HWND combo)
    {
        HDC dc = GetDC(combo);
        const auto previous = SelectObject(dc, font_);
        LONG longest = 0;
        const int count = static_cast<int>(SendMessageW(combo, CB_GETCOUNT, 0, 0));
        for (int i = 0; i < count; ++i)
        {
            const int length = static_cast<int>(SendMessageW(combo, CB_GETLBTEXTLEN, i, 0));
            if (length <= 0)
                continue;
            std::wstring item(length + 1, L'\0');
            SendMessageW(combo, CB_GETLBTEXT, i, reinterpret_cast<LPARAM>(item.data()));
            SIZE size{};
            GetTextExtentPoint32W(dc, item.c_str(), length, &size);
            longest = std::max(longest, size.cx);
        }
        SelectObject(dc, previous);
        ReleaseDC(combo, dc);
        MONITORINFO info{};
        info.cbSize = sizeof(info);
        GetMonitorInfoW(MonitorFromWindow(combo, MONITOR_DEFAULTTONEAREST), &info);
        const LONG available = info.rcWork.right - info.rcWork.left;
        SendMessageW(combo, CB_SETDROPPEDWIDTH, std::min(longest + Pixels(30), available), 0);
    }
    LRESULT CALLBACK ParametersPanel::ComboProcedure(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR)
    {
        if (msg == WM_MOUSEWHEEL && !SendMessageW(window, CB_GETDROPPEDSTATE, 0, 0))
            return DefWindowProcW(window, msg, w, l);
        if (msg == WM_NCDESTROY)
            RemoveWindowSubclass(window, ComboProcedure, id);
        return DefSubclassProc(window, msg, w, l);
    }
    LRESULT CALLBACK ParametersPanel::SliderProcedure(HWND window, UINT msg, WPARAM w, LPARAM l, UINT_PTR id, DWORD_PTR data)
    {
        auto& self = *reinterpret_cast<ParametersPanel*>(data);
        auto paint = [&](HDC dc)
        {
            RECT bounds{}, channel{}, thumb{};
            GetClientRect(window, &bounds);
            SendMessageW(window, TBM_GETCHANNELRECT, 0, reinterpret_cast<LPARAM>(&channel));
            SendMessageW(window, TBM_GETTHUMBRECT, 0, reinterpret_cast<LPARAM>(&thumb));
            FillRect(dc, &bounds, self.background_);
            const auto& appearance = self.model_->Appearance();
            const auto brush = static_cast<HBRUSH>(GetStockObject(DC_BRUSH));
            SetDCBrushColor(dc, Color(appearance.Color(ColorRole::Grid)));
            FillRect(dc, &channel, brush);
            SetDCBrushColor(dc, Color(appearance.Color(ColorRole::Slider)));
            FillRect(dc, &thumb, brush);
            if (GetFocus() == window && !(SendMessageW(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
                DrawFocusRect(dc, &bounds);
        };
        if (msg == WM_ERASEBKGND)
            return 1;
        if (msg == WM_PRINTCLIENT)
        {
            paint(reinterpret_cast<HDC>(w));
            return 0;
        }
        if (msg == WM_PAINT)
        {
            PAINTSTRUCT ps{};
            paint(BeginPaint(window, &ps));
            EndPaint(window, &ps);
            return 0;
        }
        if (msg == WM_SETFOCUS || msg == WM_KILLFOCUS)
            InvalidateRect(window, nullptr, FALSE);
        if (msg == WM_NCDESTROY)
            RemoveWindowSubclass(window, SliderProcedure, id);
        return DefSubclassProc(window, msg, w, l);
    }
    LRESULT CALLBACK ParametersPanel::Procedure(HWND window, UINT msg, WPARAM w, LPARAM l)
    {
        auto* self = reinterpret_cast<ParametersPanel*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        if (msg == WM_NCCREATE)
        {
            self = static_cast<ParametersPanel*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (!self || !self->model_)
            return DefWindowProcW(window, msg, w, l);
        try
        {
            if (msg == WM_COMMAND)
            {
                if (!self->syncing_)
                {
                    const HWND source = reinterpret_cast<HWND>(l);
                    if (HIWORD(w) == CBN_DROPDOWN &&
                        std::find(self->comboBoxes_.begin(), self->comboBoxes_.end(), source) != self->comboBoxes_.end())
                        self->FitDropDown(source);
                    else
                        self->Command(LOWORD(w), HIWORD(w));
                }
                return 0;
            }
            if (msg == WM_HSCROLL && l)
            {
                self->QueueSlider(GetDlgCtrlID(reinterpret_cast<HWND>(l)),
                    LOWORD(w) == TB_ENDTRACK || LOWORD(w) == TB_THUMBPOSITION);
                return 0;
            }
            if (msg == WM_TIMER && w == 1)
            {
                self->FlushSliders();
                return 0;
            }
            if (msg == WM_VSCROLL)
            {
                SCROLLINFO s{};
                s.cbSize = sizeof(s);
                s.fMask = SIF_ALL;
                GetScrollInfo(self->content_, SB_VERT, &s);
                int p = self->scroll_;
                switch (LOWORD(w))
                {
                case SB_LINEUP:
                    p -= 32;
                    break;
                case SB_LINEDOWN:
                    p += 32;
                    break;
                case SB_PAGEUP:
                    p -= std::max(32, self->height_ - 32);
                    break;
                case SB_PAGEDOWN:
                    p += std::max(32, self->height_ - 32);
                    break;
                case SB_THUMBTRACK:
                    p = s.nTrackPos;
                    break;
                }
                self->Scroll(p);
                return 0;
            }
            if (msg == WM_MOUSEWHEEL)
            {
                const POINT point{static_cast<short>(LOWORD(l)), static_cast<short>(HIWORD(l))};
                const HWND hovered = WindowFromPoint(point);
                if (hovered == self->parent_)
                    return DefWindowProcW(window, msg, w, l);
                if (hovered != self->content_ && !IsChild(self->content_, hovered))
                    return 0;
                for (HWND combo : self->comboBoxes_)
                    if (combo == hovered || IsChild(combo, hovered))
                        return 0;
                self->Scroll(self->scroll_ - MulDiv(GET_WHEEL_DELTA_WPARAM(w), 80, WHEEL_DELTA));
                return 0;
            }
            if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN)
            {
                HDC dc = reinterpret_cast<HDC>(w);
                const auto& a = self->model_->Appearance();
                SetTextColor(dc, Color(a.Color(ColorRole::Text)));
                COLORREF bg = msg == WM_CTLCOLOREDIT ? (a.LightTheme ? RGB(255, 255, 255) : RGB(42, 49, 62))
                    : Color(a.Color(ColorRole::Panel));
                SetBkColor(dc, bg);
                return reinterpret_cast<LRESULT>(msg == WM_CTLCOLOREDIT ? self->fieldBrush_ : self->background_);
            }
            if (msg == WM_DRAWITEM)
            {
                auto* d = reinterpret_cast<DRAWITEMSTRUCT*>(l);
                int role = d->CtlID == 914 ? static_cast<int>(ColorRole::Body) :
                    d->CtlID >= 500 ? int(d->CtlID) - 500 : d->CtlID == 122 ? 7 : d->CtlID == 123 ? 8 : 6;
                glm::vec4 c;
                if (d->CtlID >= 1000)
                {
                    size_t part = (d->CtlID - 1000) / 32;
                    auto color =
                        part < self->drafts_.size() ? self->drafts_[part].Color : std::array<float, 4>{ {1, 1, 1, 1} };
                    c = { color[0], color[1], color[2], color[3] };
                }
                else
                    c = self->model_->Appearance().Colors[role];
                HBRUSH b = CreateSolidBrush(Color(c));
                FillRect(d->hDC, &d->rcItem, b);
                DeleteObject(b);
                FrameRect(d->hDC, &d->rcItem, reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH)));
                if (d->itemState & ODS_FOCUS)
                    DrawFocusRect(d->hDC, &d->rcItem);
                return TRUE;
            }
            if (msg == WM_ERASEBKGND)
                return 1;
            if (msg == WM_PAINT)
            {
                PAINTSTRUCT p;
                HDC dc = BeginPaint(window, &p);
                RECT r;
                GetClientRect(window, &r);
                FillRect(dc, &r, self->background_);
                EndPaint(window, &p);
                return 0;
            }
        }
        catch (const std::exception&)
        {
            self->SetError(L"Не удалось применить изменение. Попробуйте немного сдвинуть плоскость от границы фигуры.");
            self->Notify();
            return 0;
        }
        return DefWindowProcW(window, msg, w, l);
    }
} // namespace IceCream::App::Views
