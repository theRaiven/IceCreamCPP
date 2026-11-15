#include "ParametersPanel.h"
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <vector>

namespace IceCream::App::Views
{
    namespace
    {
        void SetPaletteText(HWND window, const wchar_t* text)
        {
            int length = GetWindowTextLengthW(window);
            if (length == static_cast<int>(wcslen(text)))
            {
                std::wstring current(length + 1, L'\0');
                GetWindowTextW(window, current.data(), length + 1);
                if (wcscmp(current.c_str(), text) == 0)
                    return;
            }
            SetWindowTextW(window, text);
        }
        struct Palette
        {
            static constexpr int DesignWidth = 666, DesignHeight = 452;
            static constexpr DWORD Style = WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;
            static constexpr DWORD ExStyle = WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT | WS_EX_COMPOSITED;
            struct ControlLayout
            {
                HWND Window;
                RECT Bounds;
            };
            HWND window = nullptr;
            glm::vec4 color{ 1 };
            bool accepted = false, valid = true, updating = false, alternate = false;
            HFONT font = nullptr;
            std::vector<ControlLayout> controls;
            RECT spectrum{}, sample{};
            double scale = 1;
            std::vector<DWORD> spectrum_;
            bool spectrumAlternate_ = false;
            ~Palette() { if (font) DeleteObject(font); }

            int Pixel(LONG value) const { return static_cast<int>(std::lround(value * scale)); }

            RECT Scale(RECT rect) const
            {
                return {Pixel(rect.left), Pixel(rect.top), Pixel(rect.right), Pixel(rect.bottom)};
            }

            void Layout(UINT dpi, const RECT& anchor)
            {
                MONITORINFO info{};
                info.cbSize = sizeof(info);
                GetMonitorInfoW(MonitorFromRect(&anchor, MONITOR_DEFAULTTONEAREST), &info);
                const auto& work = info.rcWork;
                RECT frame{};
                AdjustWindowRectExForDpi(&frame, Style, FALSE, ExStyle, dpi);
                const int borderWidth = frame.right - frame.left;
                const int borderHeight = frame.bottom - frame.top;
                scale = std::min({dpi / 96.0, std::max(1L, work.right - work.left - borderWidth) / double(DesignWidth),
                    std::max(1L, work.bottom - work.top - borderHeight) / double(DesignHeight)});
                const int width = Pixel(DesignWidth) + borderWidth;
                const int height = Pixel(DesignHeight) + borderHeight;
                const int x = std::clamp<int>((anchor.left + anchor.right - width) / 2, work.left, work.right - width);
                const int y = std::clamp<int>((anchor.top + anchor.bottom - height) / 2, work.top, work.bottom - height);
                SetWindowPos(window, nullptr, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
                spectrum = Scale({16, 16, 376, 266});
                sample = Scale({396, 58, 650, 100});
                HFONT next = CreateFontW(-std::max(1, Pixel(16)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
                for (const auto& control : controls)
                {
                    const auto bounds = Scale(control.Bounds);
                    if (next)
                        SendMessageW(control.Window, WM_SETFONT, reinterpret_cast<WPARAM>(next), FALSE);
                    MoveWindow(control.Window, bounds.left, bounds.top, bounds.right - bounds.left,
                        bounds.bottom - bounds.top, TRUE);
                }
                if (next)
                {
                    if (font) DeleteObject(font);
                    font = next;
                }
                InvalidateRect(window, nullptr, FALSE);
            }
            void RepaintSample()
            {
                InvalidateRect(window, &sample, FALSE);
            }
            static glm::vec3 Spectrum(float x, float y, bool alternate)
            {
                float hue = x * 6;
                float c = 1 - std::abs(std::fmod(hue, 2.0f) - 1);
                glm::vec3 rgb = hue < 1 ? glm::vec3(1, c, 0)
                    : hue < 2 ? glm::vec3(c, 1, 0)
                    : hue < 3 ? glm::vec3(0, 1, c)
                    : hue < 4 ? glm::vec3(0, c, 1)
                    : hue < 5 ? glm::vec3(c, 0, 1)
                    : glm::vec3(1, 0, c);
                if (alternate)
                    return glm::mix(glm::vec3(1), rgb, 1 - y) * (1 - y * .8f);
                return y < .5f ? glm::mix(glm::vec3(1), rgb, y * 2) : rgb * (2 - y * 2);
            }
            void Sync()
            {
                valid = true;
                updating = true;
                wchar_t b[128];
                int n[4];
                for (int i = 0; i < 4; ++i)
                {
                    n[i] = int(std::round(color[i] * 255));
                    swprintf_s(b, L"%d", n[i]);
                    SetPaletteText(GetDlgItem(window, 10 + i), b);
                    if (SendMessageW(GetDlgItem(window, 30 + i), TBM_GETPOS, 0, 0) != n[i])
                        SendMessageW(GetDlgItem(window, 30 + i), TBM_SETPOS, TRUE, n[i]);
                }
                swprintf_s(b, L"%d, %d, %d", n[0], n[1], n[2]);
                SetPaletteText(GetDlgItem(window, 20), b);
                swprintf_s(b, L"%d, %d, %d, %d", n[0], n[1], n[2], n[3]);
                SetPaletteText(GetDlgItem(window, 21), b);
                swprintf_s(b, L"#%02X%02X%02X%02X", n[0], n[1], n[2], n[3]);
                SetPaletteText(GetDlgItem(window, 22), b);
                updating = false;
                RepaintSample();
            }
            bool Read(bool hex)
            {
                wchar_t b[128];
                if (hex)
                {
                    GetWindowTextW(GetDlgItem(window, 22), b, 128);
                    std::wstring s = b;
                    if (!s.empty() && s[0] == '#')
                        s.erase(0, 1);
                    if (s.size() != 6 && s.size() != 8)
                        return false;
                    for (wchar_t c : s)
                        if (!iswxdigit(c))
                            return false;
                    unsigned long long v = wcstoull(s.c_str(), nullptr, 16);
                    if (s.size() == 6)
                        v = (v << 8) | 255;
                    for (int i = 0; i < 4; ++i)
                        color[i] = float((v >> (24 - i * 8)) & 255) / 255;
                }
                else
                {
                    glm::vec4 next;
                    for (int i = 0; i < 4; ++i)
                    {
                        GetWindowTextW(GetDlgItem(window, 10 + i), b, 128);
                        wchar_t* end;
                        long n = wcstol(b, &end, 10);
                        if (end == b || *end || n < 0 || n > 255)
                            return false;
                        next[i] = float(n) / 255;
                    }
                    color = next;
                }
                return true;
            }
            static LRESULT CALLBACK Proc(HWND hwnd, UINT msg, WPARAM w, LPARAM l)
            {
                auto* p = reinterpret_cast<Palette*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
                if (msg == WM_NCCREATE)
                {
                    p = static_cast<Palette*>(reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams);
                    p->window = hwnd;
                    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(p));
                }
                if (!p)
                    return DefWindowProcW(hwnd, msg, w, l);
                if (msg == WM_DPICHANGED)
                {
                    p->Layout(HIWORD(w), *reinterpret_cast<const RECT*>(l));
                    return 0;
                }
                if (msg == WM_PAINT)
                {
                    PAINTSTRUCT ps;
                    HDC dc = BeginPaint(hwnd, &ps);
                    RECT all;
                    GetClientRect(hwnd, &all);
                    FillRect(dc, &all, reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
                    if (p->spectrum_.empty() || p->spectrumAlternate_ != p->alternate)
                    {
                        p->spectrum_.resize(360 * 250);
                        p->spectrumAlternate_ = p->alternate;
                        for (int y = 0; y < 250; ++y)
                            for (int x = 0; x < 360; ++x)
                            {
                                auto c = Spectrum(x / 359.0f, y / 249.0f, p->alternate);
                                p->spectrum_[y * 360 + x] =
                                    (DWORD(c.r * 255) << 16) | (DWORD(c.g * 255) << 8) | DWORD(c.b * 255);
                            }
                    }
                    BITMAPINFO info{};
                    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                    info.bmiHeader.biWidth = 360;
                    info.bmiHeader.biHeight = -250;
                    info.bmiHeader.biPlanes = 1;
                    info.bmiHeader.biBitCount = 32;
                    info.bmiHeader.biCompression = BI_RGB;
                    const auto& spectrum = p->spectrum;
                    StretchDIBits(dc, spectrum.left, spectrum.top, spectrum.right - spectrum.left, spectrum.bottom - spectrum.top,
                        0, 0, 360, 250, p->spectrum_.data(), &info, DIB_RGB_COLORS, SRCCOPY);
                    const auto& sample = p->sample;
                    const int square = std::max(1, p->Pixel(10));
                    for (int y = sample.top; y < sample.bottom; y += square)
                        for (int x = sample.left; x < sample.right; x += square)
                        {
                            float gray = ((x / square + y / square) % 2) ? .8f : 1;
                            auto c = glm::vec3(p->color) * p->color.a + glm::vec3(gray) * (1 - p->color.a);
                            HBRUSH b = CreateSolidBrush(RGB(int(c.r * 255), int(c.g * 255), int(c.b * 255)));
                            RECT r{ x, y, std::min<LONG>(x + square, sample.right), std::min<LONG>(y + square, sample.bottom) };
                            FillRect(dc, &r, b);
                            DeleteObject(b);
                        }
                    EndPaint(hwnd, &ps);
                    return 0;
                }
                if (msg == WM_LBUTTONDOWN || (msg == WM_MOUSEMOVE && (w & MK_LBUTTON)))
                {
                    const POINT point{static_cast<short>(LOWORD(l)), static_cast<short>(HIWORD(l))};
                    const auto& bounds = p->spectrum;
                    if (PtInRect(&bounds, point))
                    {
                        if (msg == WM_LBUTTONDOWN)
                            SetCapture(hwnd);
                        const float x = float(point.x - bounds.left) / std::max(1L, bounds.right - bounds.left - 1);
                        const float y = float(point.y - bounds.top) / std::max(1L, bounds.bottom - bounds.top - 1);
                        auto c = Spectrum(x, y, p->alternate);
                        p->color = glm::vec4(c, p->color.a);
                        p->Sync();
                    }
                    return 0;
                }
                if (msg == WM_LBUTTONUP)
                {
                    if (GetCapture() == hwnd)
                        ReleaseCapture();
                    return 0;
                }
                if (msg == WM_HSCROLL)
                {
                    int id = GetDlgCtrlID(reinterpret_cast<HWND>(l));
                    if (id >= 30 && id < 34)
                    {
                        p->color[id - 30] = float(SendMessageW(GetDlgItem(hwnd, id), TBM_GETPOS, 0, 0)) / 255;
                        p->Sync();
                    }
                    return 0;
                }
                if (msg == WM_COMMAND)
                {
                    int id = LOWORD(w), note = HIWORD(w);
                    if (id == 1)
                    {
                        if (!p->valid || !p->Read(false))
                        {
                            MessageBoxW(hwnd, L"Введите каналы от 0 до 255 или HEX #RRGGBB / #RRGGBBAA.", L"Цвет", MB_OK);
                            return 0;
                        }
                        p->accepted = true;
                        DestroyWindow(hwnd);
                        return 0;
                    }
                    if (id == 2)
                    {
                        DestroyWindow(hwnd);
                        return 0;
                    }
                    if (id == 3)
                    {
                        p->alternate = !p->alternate;
                        InvalidateRect(hwnd, &p->spectrum, FALSE);
                        return 0;
                    }
                    if (!p->updating && note == EN_CHANGE && ((id >= 10 && id < 14) || id == 22))
                    {
                        if ((p->valid = p->Read(id == 22)))
                        {
                            if (id == 22)
                                p->Sync();
                            else
                            {
                                p->updating = true;
                                for (int i = 0; i < 4; ++i)
                                    SendMessageW(GetDlgItem(hwnd, 30 + i), TBM_SETPOS, TRUE,
                                        int(std::round(p->color[i] * 255)));
                                wchar_t b[80];
                                swprintf_s(b, L"#%02X%02X%02X%02X", int(std::round(p->color.r * 255)),
                                    int(std::round(p->color.g * 255)), int(std::round(p->color.b * 255)),
                                    int(std::round(p->color.a * 255)));
                                SetPaletteText(GetDlgItem(hwnd, 22), b);
                                swprintf_s(b, L"%d, %d, %d", int(p->color.r * 255), int(p->color.g * 255),
                                    int(p->color.b * 255));
                                SetPaletteText(GetDlgItem(hwnd, 20), b);
                                swprintf_s(b, L"%d, %d, %d, %d", int(p->color.r * 255), int(p->color.g * 255),
                                    int(p->color.b * 255), int(p->color.a * 255));
                                SetPaletteText(GetDlgItem(hwnd, 21), b);
                                p->updating = false;
                                p->RepaintSample();
                            }
                        }
                    }
                    return 0;
                }
                if (msg == WM_CLOSE)
                {
                    DestroyWindow(hwnd);
                    return 0;
                }
                return DefWindowProcW(hwnd, msg, w, l);
            }
        };
    } // namespace
    void ParametersPanel::EditColor(int role)
    {
        Palette p;
        if (role >= 100)
        {
            const auto& c = drafts_.at(role - 100).Color;
            p.color = { c[0], c[1], c[2], c[3] };
        }
        else
            p.color = model_->Appearance().Colors.at(role);
        WNDCLASSW cls{};
        cls.lpfnWndProc = Palette::Proc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_CROSS);
        cls.lpszClassName = L"IceCream.Palette";
        RegisterClassW(&cls);
        RECT parent;
        GetWindowRect(parent_, &parent);
        HWND win = CreateWindowExW(Palette::ExStyle, cls.lpszClassName, L"Палитра цвета · RGB / RGBA / HEX",
            Palette::Style, (parent.left + parent.right) / 2, (parent.top + parent.bottom) / 2, 1, 1,
            parent_, nullptr, cls.hInstance, &p);
        if (!win)
            return;
        auto make = [&](const wchar_t* type, const wchar_t* title, int id, int x, int y, int width, int height,
            DWORD style) {
                HWND c = CreateWindowExW(wcscmp(type, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0, type, title,
                    WS_CHILD | WS_VISIBLE | style, 0, 0, 1, 1, win,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), cls.hInstance, nullptr);
                p.controls.push_back({c, {x, y, x + width, y + height}});
                return c;
            };
        make(L"BUTTON", L"Сменить палитру", 3, 396, 16, 254, 30, WS_TABSTOP);
        const wchar_t* names[] = { L"R", L"G", L"B", L"A" };
        for (int i = 0; i < 4; ++i)
        {
            make(L"STATIC", names[i], 0, 396, 112 + i * 38, 20, 26, 0);
            make(L"EDIT", L"", 10 + i, 422, 108 + i * 38, 55, 28, WS_TABSTOP | ES_AUTOHSCROLL);
            HWND slider = make(TRACKBAR_CLASSW, L"", 30 + i, 485, 108 + i * 38, 165, 28, WS_TABSTOP | TBS_NOTICKS);
            SendMessageW(slider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
        }
        const wchar_t* formats[] = { L"RGB", L"RGBA", L"HEX" };
        for (int i = 0; i < 3; ++i)
        {
            make(L"STATIC", formats[i], 0, 16, 286 + i * 38, 60, 26, 0);
            make(L"EDIT", L"", 20 + i, 80, 282 + i * 38, 570, 28, i < 2 ? ES_READONLY : WS_TABSTOP | ES_AUTOHSCROLL);
        }
        make(L"BUTTON", L"Применить", 1, 398, 405, 122, 32, WS_TABSTOP | BS_DEFPUSHBUTTON);
        make(L"BUTTON", L"Отмена", 2, 530, 405, 120, 32, WS_TABSTOP);
        p.Layout(GetDpiForWindow(win), parent);
        p.Sync();
        EnableWindow(parent_, FALSE);
        ShowWindow(win, SW_SHOW);
        SetFocus(GetDlgItem(win, 22));
        MSG msg;
        while (IsWindow(win) && GetMessageW(&msg, nullptr, 0, 0) > 0)
        {
            if (!IsDialogMessageW(win, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
        EnableWindow(parent_, TRUE);
        SetActiveWindow(parent_);
        SetFocus(Control(role >= 100 ? 1000 + (role - 100) * 32 + 10 :
            geometryMode_ != 0 && role == static_cast<int>(ViewModels::ColorRole::Body) ? 914 : 500 + role));
        if (p.accepted)
        {
            if (role >= 100)
            {
                drafts_.at(role - 100).Color = { {p.color.r, p.color.g, p.color.b, p.color.a} };
                InvalidateRect(Control(1000 + (role - 100) * 32 + 10), nullptr, TRUE);
                return;
            }
            auto a = model_->Appearance();
            a.SetColor(static_cast<ViewModels::ColorRole>(role), p.color);
            model_->SetAppearance(a);
            Theme();
            Notify();
        }
    }
} // namespace IceCream::App::Views
