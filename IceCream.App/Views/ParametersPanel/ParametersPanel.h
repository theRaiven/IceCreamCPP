#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Windows types must be declared before the common-controls headers.
#include <windows.h>

#include "../../ViewModels/MainViewModel.h"
#include <array>
#include <commctrl.h>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace IceCream::App::Views
{
    class ParametersPanel
    {
        ViewModels::MainViewModel* model_ = nullptr;
        std::function<void()> changed_;
        std::function<void()> resetSectionView_;
        std::array<int, 3> sectionSliderRanges_{ {200,200,200} };
        void RefreshSectionRanges();
        HWND parent_ = nullptr, panel_ = nullptr, content_ = nullptr;
        HFONT font_ = nullptr, hintFont_ = nullptr;
        HBRUSH background_ = nullptr, fieldBrush_ = nullptr;
        std::vector<HWND> controls_;
        std::array<bool, 4> expanded_{ {true, false, false, true} };
        bool appearanceTab_ = false, syncing_ = false, panelOpen_ = false;
        int geometryMode_ = 0; // 0: интегралы, 1: неявная поверхность, 2: тело по неравенствам
        bool constraintsOpen_ = false;
        int activePage_ = 0;
        std::vector<Core::Services::IntegralPart> drafts_;
        void LoadDrafts();
        void StoreDrafts();
        void EstimateBounds();
        void UpdateIntegralPreview(size_t part);
        void IntegralCommand(int id, int notification);
        int scroll_ = 0, contentHeight_ = 0, height_ = 800, width_ = 400;
        std::uint64_t revision_ = ~std::uint64_t(0);
        HWND Make(const wchar_t* cls, const wchar_t* text, int id, DWORD style = 0);
        HWND Control(int id) const;
        std::map<HWND, RECT> positions_;
        int viewportX_ = -1, viewportY_ = -1, viewportWidth_ = -1, viewportHeight_ = -1;
        bool viewportOpen_ = false;
        int hintFontSize_ = 0;
        std::array<std::optional<double>, 6> pendingSection_{};
        std::optional<int> pendingHintSize_, pendingCoordinateSize_;
        bool sliderTimer_ = false;
        void QueueSlider(int id, bool finish);
        void FlushSliders();
        void RefreshTextSizes();
        void SetText(int id, const wchar_t* value);
        void Layout();
        void Sync();
        void Theme();
        void Notify();
        void ApplyShape();
        void ApplySection();
        void ApplyLighting();
        void EditColor(int role);
        void Scroll(int position);
        double Number(int id) const;
        void SetNumber(int id, double value);
        void SetError(const wchar_t* message);
        void Command(int id, int notification);
        static LRESULT CALLBACK Procedure(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK EditProcedure(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

    public:
        int OccupiedWidth() const
        {
            return panelOpen_ ? 440 : 0;
        }
        void Initialize(ViewModels::MainViewModel&, std::function<void()>, std::function<void()> resetSectionView);
        void Destroy();
        void SetViewport(int, int, int, int);
        void Render(const ViewModels::MainViewModel&);
        bool Contains(int, int, int) const
        {
            return false;
        }
        bool Mouse(bool, int, int, int, ViewModels::MainViewModel&)
        {
            return false;
        }
        bool Motion(int, ViewModels::MainViewModel&)
        {
            return false;
        }
        bool Keyboard(unsigned char, ViewModels::MainViewModel&)
        {
            return false;
        }
        bool Editing() const
        {
            return panel_ && (GetFocus() == panel_ || IsChild(panel_, GetFocus()));
        }
    };
} // namespace IceCream::App::Views
