#include "ParametersPanel.h"
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
COLORREF Color(const glm::vec4 &c)
{
    return RGB(int(c.r * 255), int(c.g * 255), int(c.b * 255));
}
[[maybe_unused]] const wchar_t *colorNames[] = {
    L"Фон 3D",          L"Фон сечения",     L"Фон панели",     L"Текст",          L"Сетка", L"Ползунки", L"Поверхность",
    L"Рожок / часть 1", L"Шапка / часть 2", L"Контур сечения", L"Границы частей", L"Ось X", L"Ось Y",    L"Ось Z"};
} // namespace
HWND ParametersPanel::Make(const wchar_t *cls, const wchar_t *text, int id, DWORD style)
{
    HWND w = CreateWindowExW(wcscmp(cls, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0, cls, text, WS_CHILD | style, 0, 0, 1, 1,
                             content_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), GetModuleHandleW(nullptr),
                             nullptr);
    if (!w)
        throw std::runtime_error("Cannot create panel control");
    SendMessageW(w, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    if (wcscmp(cls, L"EDIT") == 0)
    {
        SendMessageW(w, EM_SETLIMITTEXT, id >= 1000 ? 512 : 64, 0);
        SetWindowSubclass(w, EditProcedure, 1, reinterpret_cast<DWORD_PTR>(this));
    }
    controls_.push_back(w);
    return w;
}
HWND ParametersPanel::Control(int id) const
{
    return GetDlgItem(content_, id);
}
void ParametersPanel::Initialize(ViewModels::MainViewModel &model, std::function<void()> changed)
{
    model_ = &model;
    changed_ = std::move(changed);
    parent_ = WindowFromDC(wglGetCurrentDC());
    SetWindowLongPtrW(parent_, GWL_STYLE, GetWindowLongPtrW(parent_, GWL_STYLE) | WS_CLIPCHILDREN);
    INITCOMMONCONTROLSEX common{sizeof(common), ICC_BAR_CLASSES};
    InitCommonControlsEx(&common);
    font_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0,
                        L"Segoe UI");
    WNDCLASSW cls{};
    cls.lpfnWndProc = Procedure;
    cls.hInstance = GetModuleHandleW(nullptr);
    cls.lpszClassName = L"IceCream.Parameters";
    cls.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&cls);
    panel_ =
        CreateWindowExW(WS_EX_CONTROLPARENT | WS_EX_COMPOSITED, cls.lpszClassName, L"Параметры",
                        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0, 400, 800, parent_, nullptr, cls.hInstance, this);
    content_ = CreateWindowExW(WS_EX_CONTROLPARENT, cls.lpszClassName, L"Блоки параметров",
                               WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN, 0, 48, 400, 752, panel_, nullptr,
                               cls.hInstance, this);
    for (int i = 0; i < 4; ++i)
    {
        HWND b = CreateWindowW(L"BUTTON",
                               i == 0   ? L"Функции"
                               : i == 1 ? L"Сечение"
                               : i == 2 ? L"Свет"
                                        : L"Вид",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON, 0, 0, 1, 1, panel_,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(10 + i)), cls.hInstance, nullptr);
        SendMessageW(b, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    }
    const wchar_t *groups[] = {L"Фигура", L"Сечение", L"Свет", L"Вид и цвета"};
    for (int i = 0; i < 4; ++i)
        Make(L"BUTTON", groups[i], 20 + i, WS_TABSTOP | BS_PUSHBUTTON);
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
    Make(L"COMBOBOX", L"", 300, WS_TABSTOP | CBS_DROPDOWNLIST);
    for (auto s : {L"Направленный", L"Точечный", L"Прожектор"})
        SendMessageW(Control(300), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
    for (int i = 0; i < 11; ++i)
        Make(L"EDIT", L"", 310 + i, WS_TABSTOP | ES_AUTOHSCROLL);
    Make(L"BUTTON", L"Применить свет", 330, WS_TABSTOP);
    Make(L"COMBOBOX", L"", 400, WS_TABSTOP | CBS_DROPDOWNLIST);
    for (auto s : {L"Тёмная тема", L"Светлая тема"})
        SendMessageW(Control(400), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(s));
    SendMessageW(Control(400), CB_SETCURSEL, 0, 0);
    Make(L"BUTTON", L"Показывать подсказки", 401, WS_TABSTOP | BS_AUTOCHECKBOX);
    for (int i = 0; i < 2; ++i)
        Make(TRACKBAR_CLASSW, L"", 402 + i, WS_TABSTOP | TBS_HORZ | TBS_NOTICKS);
    SendMessageW(Control(402), TBM_SETRANGE, TRUE, MAKELPARAM(10, 24));
    SendMessageW(Control(402), TBM_SETPOS, TRUE, 14);
    SendMessageW(Control(403), TBM_SETRANGE, TRUE, MAKELPARAM(12, 36));
    SendMessageW(Control(403), TBM_SETPOS, TRUE, 20);
    for (int i = 0; i < 14; ++i)
        Make(L"BUTTON", colorNames[i], 500 + i, WS_TABSTOP | BS_OWNERDRAW);
    for (int i = 0; i < 256; ++i)
        Make(L"STATIC", L"", 20000 + i, SS_LEFT);
    Make(L"BUTTON", L"Построить", 900, WS_TABSTOP);
    Make(L"BUTTON", L"+ Добавить часть", 901, WS_TABSTOP);
    for (int part = 0; part < 8; ++part)
    {
        int id = 1000 + part * 32;
        for (int k : {0, 1, 4, 5, 6, 7, 8, 9})
            Make(L"EDIT", L"", id + k, WS_TABSTOP | ES_AUTOHSCROLL);
        Make(L"COMBOBOX", L"", id + 2, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
        for (auto name : {L"Декартовы (x, y, z)", L"Цилиндрические (r, phi, z)", L"Полярные: z=f(r, phi)"})
            SendMessageW(Control(id + 2), CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        Make(L"COMBOBOX", L"", id + 3, WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL);
        Make(L"BUTTON", L"Цвет функции", id + 10, WS_TABSTOP | BS_OWNERDRAW);
        Make(L"BUTTON", L"×", id + 11, WS_TABSTOP);
        Make(L"STATIC", L"", id + 12, SS_LEFT);
        Make(L"COMBOBOX",L"",id+14,WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL);
    }
    drafts_ = Core::Services::IntegralBodyBuilder::DefaultParts();
    model_->ApplyIntegralParts(drafts_);
    LoadDrafts();
    Make(L"STATIC", L"", 800, SS_LEFT);
    Make(L"STATIC", L"", 801, SS_LEFT);
    Theme();
    Sync();
    Layout();
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
    if (background_)
        DeleteObject(background_);
    if (fieldBrush_)
        DeleteObject(fieldBrush_);
    controls_.clear();
    positions_.clear();
}
void ParametersPanel::Theme()
{
    const auto &a = model_->Appearance();
    if (background_)
        DeleteObject(background_);
    if (fieldBrush_)
        DeleteObject(fieldBrush_);
    background_ = CreateSolidBrush(Color(a.Color(ColorRole::Panel)));
    fieldBrush_ = CreateSolidBrush(a.LightTheme ? RGB(255, 255, 255) : RGB(42, 49, 62));
    RefreshTextSizes();
    InvalidateRect(panel_, nullptr, TRUE);
    InvalidateRect(content_, nullptr, TRUE);
    for (auto w : controls_)
        InvalidateRect(w, nullptr, TRUE);
}
void ParametersPanel::SetViewport(int x, int y, int width, int height)
{
    if (viewportX_ == x && viewportY_ == y && viewportWidth_ == width && viewportHeight_ == height &&
        viewportOpen_ == panelOpen_)
        return;
    viewportX_ = x;
    viewportY_ = y;
    viewportWidth_ = width;
    viewportHeight_ = height;
    viewportOpen_ = panelOpen_;
    width_ = width;
    height_ = height;
    MoveWindow(panel_, x, y, width, panelOpen_ ? height : 52, TRUE);
    for (int i = 0; i < 4; ++i)
        MoveWindow(GetDlgItem(panel_, 10 + i), 8 + i * (width - 16) / 4, 8, (width - 24) / 4, 32, TRUE);
    MoveWindow(content_, 0, 52, width, height - 52, TRUE);
    ShowWindow(content_, panelOpen_ ? SW_SHOW : SW_HIDE);
    Layout();
}
void ParametersPanel::Render(const ViewModels::MainViewModel &)
{
    if (revision_ != model_->GeometryRevision())
    {
        Sync();
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
void ParametersPanel::SetError(const wchar_t *message)
{
    SetText(801, message);
}
double ParametersPanel::Number(int id) const
{
    wchar_t b[520]{};
    GetWindowTextW(Control(id), b, 520);
    for (auto &c : b)
        if (c == L',')
            c = L'.';
    wchar_t *end = nullptr;
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
    const auto &o = model_->SectionPosition();
    const auto &r = model_->SectionRotation();
    const double v[] = {o.X, o.Y, o.Z, r.X, r.Y, r.Z};
    for (int i = 0; i < 6; ++i)
    {
        SetNumber(200 + i, v[i]);
        SendMessageW(Control(220 + i), TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(v[i] * 10)));
    }
    const auto &l = model_->Lighting();
    SendMessageW(Control(300), CB_SETCURSEL, static_cast<int>(l.Type), 0);
    const float lv[] = {l.AmbientIntensity, l.EmissionIntensity, l.LightIntensity, l.Position.x,
                        l.Position.y,       l.Position.z,        l.Direction.x,    l.Direction.y,
                        l.Direction.z,      l.SpotAngle,         l.SpotExponent};
    for (int i = 0; i < 11; ++i)
        SetNumber(310 + i, lv[i]);
    revision_ = model_->GeometryRevision();
    syncing_ = false;
}
void ParametersPanel::SetText(int id, const wchar_t *value)
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
    const auto &a = model_->Appearance();
    if (hintFontSize_ != a.HintSize || !hintFont_)
    {
        HFONT next = CreateFontW(-a.HintSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0,
                                 CLEARTYPE_QUALITY, 0, L"Segoe UI");
        SendMessageW(Control(800), WM_SETFONT, reinterpret_cast<WPARAM>(next), TRUE);
        if (hintFont_)
            DeleteObject(hintFont_);
        hintFont_ = next;
        hintFontSize_ = a.HintSize;
    }
    if (activePage_ == 3)
    {
        wchar_t text[80];
        swprintf_s(text, L"Размер подсказок: %d px", a.HintSize);
        SetText(20000, text);
        swprintf_s(text, L"Размер координат: %d px", a.CoordinateSize);
        SetText(20001, text);
    }
}
void ParametersPanel::QueueSlider(int id, bool finish)
{
    if (syncing_)
        return;
    int value = static_cast<int>(SendMessageW(Control(id), TBM_GETPOS, 0, 0));
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
    double values[] = {origin.X, origin.Y, origin.Z, rotation.X, rotation.Y, rotation.Z};
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
            model_->SetSectionPose({values[0], values[1], values[2]}, {values[3], values[4], values[5]});
            SetError(L"");
            changed = true;
        }
        catch (const std::exception &)
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
    int next = std::clamp(position, 0, std::max(0, contentHeight_ - (height_ - 52)));
    if (next == scroll_)
        return;
    scroll_ = next;
    Layout();
}
LRESULT CALLBACK ParametersPanel::Procedure(HWND window, UINT msg, WPARAM w, LPARAM l)
{
    auto *self = reinterpret_cast<ParametersPanel *>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (msg == WM_NCCREATE)
    {
        self = static_cast<ParametersPanel *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self || !self->model_)
        return DefWindowProcW(window, msg, w, l);
    try
    {
        if (msg == WM_COMMAND)
        {
            if (!self->syncing_)
                self->Command(LOWORD(w), HIWORD(w));
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
        if (msg == WM_NOTIFY)
        {
            auto *n = reinterpret_cast<NMHDR *>(l);
            if (n->code == NM_CUSTOMDRAW)
            {
                auto *draw = reinterpret_cast<NMCUSTOMDRAW *>(l);
                if (draw->dwDrawStage == CDDS_PREPAINT)
                    return CDRF_NOTIFYITEMDRAW;
                if (draw->dwDrawStage == CDDS_ITEMPREPAINT &&
                    (draw->dwItemSpec == TBCD_THUMB || draw->dwItemSpec == TBCD_CHANNEL))
                {
                    auto role = draw->dwItemSpec == TBCD_THUMB ? ColorRole::Slider : ColorRole::Grid;
                    HBRUSH brush = CreateSolidBrush(Color(self->model_->Appearance().Color(role)));
                    FillRect(draw->hdc, &draw->rc, brush);
                    DeleteObject(brush);
                    return CDRF_SKIPDEFAULT;
                }
            }
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
                p -= self->height_ - 100;
                break;
            case SB_PAGEDOWN:
                p += self->height_ - 100;
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
            self->Scroll(self->scroll_ - GET_WHEEL_DELTA_WPARAM(w) / WHEEL_DELTA * 80);
            return 0;
        }
        if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORBTN)
        {
            HDC dc = reinterpret_cast<HDC>(w);
            const auto &a = self->model_->Appearance();
            SetTextColor(dc, Color(a.Color(ColorRole::Text)));
            COLORREF bg = msg == WM_CTLCOLOREDIT ? (a.LightTheme ? RGB(255, 255, 255) : RGB(42, 49, 62))
                                                 : Color(a.Color(ColorRole::Panel));
            SetBkColor(dc, bg);
            return reinterpret_cast<LRESULT>(msg == WM_CTLCOLOREDIT ? self->fieldBrush_ : self->background_);
        }
        if (msg == WM_DRAWITEM)
        {
            auto *d = reinterpret_cast<DRAWITEMSTRUCT *>(l);
            int role = d->CtlID >= 500 ? int(d->CtlID) - 500 : d->CtlID == 122 ? 7 : d->CtlID == 123 ? 8 : 6;
            glm::vec4 c;
            if (d->CtlID >= 1000)
            {
                size_t part = (d->CtlID - 1000) / 32;
                auto color =
                    part < self->drafts_.size() ? self->drafts_[part].Color : std::array<float, 4>{{1, 1, 1, 1}};
                c = {color[0], color[1], color[2], color[3]};
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
    catch (const std::exception &)
    {
        self->SetError(L"Не удалось применить изменение. Попробуйте немного сдвинуть плоскость от границы фигуры.");
        self->Notify();
        return 0;
    }
    return DefWindowProcW(window, msg, w, l);
}
} // namespace IceCream::App::Views
