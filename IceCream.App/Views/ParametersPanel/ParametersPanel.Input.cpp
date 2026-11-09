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
            HWND window = nullptr;
            glm::vec4 color{ 1 };
            bool accepted = false, valid = true, updating = false, alternate = false;
            HFONT font = nullptr;
            std::vector<DWORD> spectrum_;
            bool spectrumAlternate_ = false;
            void RepaintSample()
            {
                RECT sample{ 396, 58, 650, 100 };
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
                    StretchDIBits(dc, 16, 16, 360, 250, 0, 0, 360, 250, p->spectrum_.data(), &info, DIB_RGB_COLORS, SRCCOPY);
                    for (int y = 58; y < 100; y += 10)
                        for (int x = 396; x < 650; x += 10)
                        {
                            float gray = ((x / 10 + y / 10) % 2) ? .8f : 1;
                            auto c = glm::vec3(p->color) * p->color.a + glm::vec3(gray) * (1 - p->color.a);
                            HBRUSH b = CreateSolidBrush(RGB(int(c.r * 255), int(c.g * 255), int(c.b * 255)));
                            RECT r{ x, y, std::min(x + 10, 650), std::min(y + 10, 100) };
                            FillRect(dc, &r, b);
                            DeleteObject(b);
                        }
                    EndPaint(hwnd, &ps);
                    return 0;
                }
                if (msg == WM_LBUTTONDOWN || (msg == WM_MOUSEMOVE && (w & MK_LBUTTON)))
                {
                    int x = static_cast<short>(LOWORD(l)), y = static_cast<short>(HIWORD(l));
                    if (x >= 16 && x < 376 && y >= 16 && y < 266)
                    {
                        if (msg == WM_LBUTTONDOWN)
                            SetCapture(hwnd);
                        auto c = Spectrum(std::clamp((x - 16) / 359.0f, 0.0f, 1.0f), std::clamp((y - 16) / 249.0f, 0.0f, 1.0f),
                            p->alternate);
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
                        RECT spectrum{ 16, 16, 376, 266 };
                        InvalidateRect(hwnd, &spectrum, FALSE);
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
        p.font = font_;
        WNDCLASSW cls{};
        cls.lpfnWndProc = Palette::Proc;
        cls.hInstance = GetModuleHandleW(nullptr);
        cls.hCursor = LoadCursor(nullptr, IDC_CROSS);
        cls.lpszClassName = L"IceCream.Palette";
        RegisterClassW(&cls);
        RECT parent;
        GetWindowRect(parent_, &parent);
        HWND win = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT | WS_EX_COMPOSITED, cls.lpszClassName,
            L"Палитра цвета · RGB / RGBA / HEX", WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN,
            parent.left + 80, parent.top + 80, 690, 490, parent_, nullptr, cls.hInstance, &p);
        if (!win)
            return;
        auto make = [&](const wchar_t* type, const wchar_t* title, int id, int x, int y, int width, int height,
            DWORD style) {
                HWND c = CreateWindowExW(wcscmp(type, L"EDIT") == 0 ? WS_EX_CLIENTEDGE : 0, type, title,
                    WS_CHILD | WS_VISIBLE | style, x, y, width, height, win,
                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), cls.hInstance, nullptr);
                SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
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
