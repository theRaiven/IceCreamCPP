#pragma once
#include "../ViewModels/MainViewModel.h"
#include <string>
#include <functional>
#include <array>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace IceCream::App::Views
{
    class ParametersPanel
    {
        int x_ = 0, y_ = 0, width_ = 340, height_ = 720;
        int activeField_ = -1, dragging_ = -1;
        std::string input_, error_;
        bool replaceInput_ = false;
        unsigned int font_ = 0;
        inline static ParametersPanel* nativeInstance_ = nullptr;
        HWND parent_ = nullptr;
        std::array<HWND, 5> editors_{};
        HWND applyButton_ = nullptr, sideButton_ = nullptr, shapePicker_ = nullptr;
        int shapeMode_ = 0;
        bool applyingShape_ = false;
        std::string lastFormula_, customDraft_ = "x^2 + y^2";
        void ConfigureShapeEditors();
        void SelectShape(int mode);
        static const char* ShapeFormula(int mode);
        static int DetectShape(const std::string& formula);
        HFONT nativeFont_ = nullptr;
        WNDPROC parentProcedure_ = nullptr, editProcedure_ = nullptr;
        ViewModels::MainViewModel* model_ = nullptr;
        std::function<void()> onChanged_;
        std::uint64_t nativeRevision_ = static_cast<std::uint64_t>(-1);
        static LRESULT CALLBACK ParentProcedure(HWND, UINT, WPARAM, LPARAM);
        static LRESULT CALLBACK EditProcedure(HWND, UINT, WPARAM, LPARAM);
        void ApplyEditors();
        void SyncEditors();
        static void DrawString(int x, int y, const char* text);
        static void Box(int x, int y, int width, int height);
        static double Value(const ViewModels::MainViewModel& model, int field);
        static void SetValue(ViewModels::MainViewModel& model, int field, double value);
        static int FieldTop(int field);
        bool Commit(ViewModels::MainViewModel& model);
        void Drag(int localX, ViewModels::MainViewModel& model);
    public:
        void Initialize(ViewModels::MainViewModel& model, std::function<void()> onChanged);
        void Destroy();
        void SetViewport(int x, int y, int width, int height);
        void Render(const ViewModels::MainViewModel& model);
        bool Contains(int windowX, int windowY, int windowHeight) const;
        bool Mouse(bool down, int x, int y, int windowHeight, ViewModels::MainViewModel& model);
        bool Motion(int x, ViewModels::MainViewModel& model);
        bool Keyboard(unsigned char key, ViewModels::MainViewModel& model);
        bool Editing() const;
    };
}
