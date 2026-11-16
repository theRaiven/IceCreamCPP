#include "MainWindow.h"
#include "../Resource.h"
#include <Windows.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <GL/freeglut.h>
#include <GL/glew.h>

#include <algorithm>
#include <iostream>

namespace IceCream::App::Views
{
    namespace
    {
        RECT WorkArea(HMONITOR monitor)
        {
            MONITORINFO info{};
            info.cbSize = sizeof(info);
            GetMonitorInfoW(monitor, &info);
            return info.rcWork;
        }

        void PlaceWindow(HWND window, const RECT& bounds, const RECT& work)
        {
            const int width = std::min(bounds.right - bounds.left, work.right - work.left);
            const int height = std::min(bounds.bottom - bounds.top, work.bottom - work.top);
            const int x = std::clamp<int>(bounds.left, work.left, work.right - width);
            const int y = std::clamp<int>(bounds.top, work.top, work.bottom - height);
            SetWindowPos(window, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }

    int MainWindow::Run(int argc, char** argv)
    {
        instance_ = this;

        glutInit(&argc, argv);

        // Совместимый профиль: шейдеры 3.3 для геометрии и GLUT-текст на панели.
        glutInitContextVersion(3, 3);
        glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);
        glutSetOption(GLUT_MULTISAMPLE, 4); // Запрашиваем 4-кратное сглаживание.
        const RECT work = WorkArea(MonitorFromPoint({0, 0}, MONITOR_DEFAULTTOPRIMARY));
        const UINT initialDpi = GetDpiForSystem();
        RECT frame{};
        AdjustWindowRectExForDpi(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0, initialDpi);
        windowWidth_ = std::min(MulDiv(windowWidth_, initialDpi, 96),
            std::max<int>(1, (work.right - work.left) - (frame.right - frame.left)));
        windowHeight_ = std::min(MulDiv(windowHeight_, initialDpi, 96),
            std::max<int>(1, (work.bottom - work.top) - (frame.bottom - frame.top)));
        glutInitWindowPosition(work.left + (work.right - work.left - windowWidth_) / 2,
            work.top + (work.bottom - work.top - windowHeight_) / 2);
        glutInitWindowSize(windowWidth_, windowHeight_);
        glutCreateWindow("IceCream");

        HWND window = nullptr;

        EnumThreadWindows(GetCurrentThreadId(), [](HWND hwnd, LPARAM param) -> BOOL
            {
                wchar_t title[128]{};
                GetWindowTextW(hwnd, title, 128);

                if (lstrcmpW(title, L"IceCream") != 0) return TRUE;

                *reinterpret_cast<HWND*>(param) = hwnd;
                return FALSE;

            }, reinterpret_cast<LPARAM>(&window));

        if (window)
        {
            HINSTANCE instance = GetModuleHandleW(nullptr);

            HICON largeIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_ICECREAM), IMAGE_ICON,
                GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_SHARED));

            HICON smallIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_ICECREAM), IMAGE_ICON,
                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));

            if (largeIcon && smallIcon)
            {
                SendMessageW(window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(largeIcon));
                SendMessageW(window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));

                SetClassLongPtrW(window, GCLP_HICON, reinterpret_cast<LONG_PTR>(largeIcon));
                SetClassLongPtrW(window, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(smallIcon));
            }
        }


        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

        glewExperimental = GL_TRUE;
        const GLenum glewResult = glewInit();

        if (glewResult != GLEW_OK)
        {
            std::cerr << "GLEW error: " << glewGetErrorString(glewResult) << '\n';
            glutDestroyWindow(glutGetWindow());

            return 1;
        }

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        window = WindowFromDC(wglGetCurrentDC());
        RECT bounds{};
        GetWindowRect(window, &bounds);
        PlaceWindow(window, bounds, work);
        dpi_ = GetDpiForWindow(window);
        RECT client{};
        GetClientRect(window, &client);
        windowWidth_ = client.right;
        windowHeight_ = client.bottom;
        SetWindowSubclass(window, WindowProcedure, 1, reinterpret_cast<DWORD_PTR>(this));

        GLint samples = 0;
        glGetIntegerv(GL_SAMPLES, &samples);
        std::cout << "MSAA: " << samples << " образцов на пиксель" << '\n';

        parametersPanel_.Initialize(viewModel_, [this] {
            UpdateLayout();
            ReloadMeshIfChanged();
            glutPostRedisplay();
            }, [this] { sectionView_.ResetView(); });
        sceneView_.Initialize();
        sectionView_.Initialize();
        ReloadMesh();
        UpdateLayout();

        glutDisplayFunc(Display);
        glutReshapeFunc(Reshape);
        glutMouseFunc(Mouse);
        glutMotionFunc(MouseMotion);
        glutMouseWheelFunc(MouseWheel);
        glutKeyboardFunc(Keyboard);
        glutSpecialFunc(Special);
        glutCloseFunc(Close);

        glutMainLoop();

        return 0;
    }

    void MainWindow::UpdateLayout()
    {
        const bool panelOpen = parametersPanel_.IsOpen();
        if (!layoutDirty_ && layoutPanelOpen_ == panelOpen)
            return;
        layoutDirty_ = false;
        layoutPanelOpen_ = panelOpen;
        const auto layout = WindowLayout::Calculate(windowWidth_, windowHeight_, dpi_, panelOpen);
        readouts_ = layout.Readouts;
        const auto& scene = layout.Scene;
        const auto& section = layout.Section;
        sceneView_.SetViewport(scene.X, windowHeight_ - scene.Y - scene.Height, scene.Width, scene.Height, dpi_);
        sectionView_.SetViewport(section.X, windowHeight_ - section.Y - section.Height, section.Width, section.Height, dpi_);
        const auto& panel = layout.Panel;
        parametersPanel_.SetViewport(panel.Width, panel.Height, layout.TabsWidth, dpi_);
    }

    LRESULT CALLBACK MainWindow::WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
        UINT_PTR subclass, DWORD_PTR data)
    {
        auto& self = *reinterpret_cast<MainWindow*>(data);
        if (message == WM_GETMINMAXINFO)
        {
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            const UINT dpi = GetDpiForWindow(window);
            RECT minimum{0, 0, MulDiv(WindowLayout::MinimumWidth, dpi, 96), MulDiv(WindowLayout::MinimumHeight, dpi, 96)};
            AdjustWindowRectExForDpi(&minimum, static_cast<DWORD>(GetWindowLongPtrW(window, GWL_STYLE)), FALSE,
                static_cast<DWORD>(GetWindowLongPtrW(window, GWL_EXSTYLE)), dpi);
            const RECT work = WorkArea(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST));
            auto& info = *reinterpret_cast<MINMAXINFO*>(lParam);
            info.ptMinTrackSize.x = std::min(minimum.right - minimum.left, work.right - work.left);
            info.ptMinTrackSize.y = std::min(minimum.bottom - minimum.top, work.bottom - work.top);
            return result;
        }
        if (message == WM_DPICHANGED)
        {
            self.dpi_ = HIWORD(wParam);
            const RECT bounds = *reinterpret_cast<const RECT*>(lParam);
            if (IsZoomed(window))
                SetWindowPos(window, nullptr, bounds.left, bounds.top, bounds.right - bounds.left,
                    bounds.bottom - bounds.top, SWP_NOZORDER | SWP_NOACTIVATE);
            else
                PlaceWindow(window, bounds, WorkArea(MonitorFromRect(&bounds, MONITOR_DEFAULTTONEAREST)));
            RECT client{};
            GetClientRect(window, &client);
            self.layoutDirty_ = true;
            Reshape(client.right, client.bottom);
            return 0;
        }
        if (message == WM_NCDESTROY)
            RemoveWindowSubclass(window, WindowProcedure, subclass);
        return DefSubclassProc(window, message, wParam, lParam);
    }

    void MainWindow::ReloadMesh()
    {
        sceneView_.UploadMesh(viewModel_.Body().Mesh(), viewModel_.TriangleColors());
        uploadedRevision_ = viewModel_.GeometryRevision();
        std::cout << "Vertices: " << viewModel_.Body().Mesh().Vertices().size()
            << "  Triangles: " << viewModel_.Body().Mesh().TriangleCount() << "  Volume: " << viewModel_.Volume()
            << '\n';
    }

    void MainWindow::ReloadMeshIfChanged()
    {
        if (viewModel_.GeometryRevision() != uploadedRevision_)
        {
            ReloadMesh();
        }
    }

    void MainWindow::Display()
    {
        const auto& background = instance_->viewModel_.Appearance().Color(ViewModels::ColorRole::Panel);
        glClearColor(background.r, background.g, background.b, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const bool showMeasurement = instance_->readouts_.Width == 0;
        instance_->sceneView_.Render(instance_->viewModel_, showMeasurement);
        instance_->sectionView_.Render(instance_->viewModel_, showMeasurement);
        instance_->DrawReadouts();
        instance_->parametersPanel_.Render(instance_->viewModel_);

        glutSwapBuffers();
    }

    void MainWindow::DrawReadouts() const
    {
        if (readouts_.Width == 0 || readouts_.Height == 0)
            return;
        const auto& appearance = viewModel_.Appearance();
        const auto textColor = glm::vec3(appearance.Color(ViewModels::ColorRole::Text));
        const int fontSize = MulDiv(appearance.TextSize + 2, dpi_, USER_DEFAULT_SCREEN_DPI);
        const int inset = MulDiv(16, dpi_, USER_DEFAULT_SCREEN_DPI);
        const int firstWidth = readouts_.Width / 2;
        auto draw = [&](int x, int width, const std::string& label)
        {
            toolbarText_.SetSize(fontSize);
            const int available = width - 2 * inset;
            const int measured = toolbarText_.Measure(label.c_str());
            if (measured > available)
                toolbarText_.SetSize(std::max(MulDiv(12, dpi_, USER_DEFAULT_SCREEN_DPI), MulDiv(fontSize, available, measured)));
            const int baseline = (readouts_.Height - toolbarText_.Height()) / 2 + toolbarText_.Height() - toolbarText_.Ascent();
            const auto line = toolbarText_.Wrap(label.c_str(), available, 1);
            if (!line.empty())
            {
                glViewport(x, windowHeight_ - readouts_.Height, width, readouts_.Height);
                toolbarText_.Screen(static_cast<float>(inset), static_cast<float>(baseline), width, readouts_.Height,
                    textColor, line.front());
            }
        };
        glDisable(GL_DEPTH_TEST);
        draw(readouts_.X, firstWidth, SceneView::MeasurementText(viewModel_, true));
        draw(readouts_.X + firstWidth, readouts_.Width - firstWidth, SectionView::MeasurementText(viewModel_, true));
        glEnable(GL_DEPTH_TEST);
    }

    void MainWindow::Reshape(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;
        if (instance_->windowWidth_ != width || instance_->windowHeight_ != height)
        {
            instance_->windowWidth_ = width;
            instance_->windowHeight_ = height;
            instance_->layoutDirty_ = true;
        }
        instance_->UpdateLayout();
        glutPostRedisplay();
    }

    void MainWindow::Mouse(int button, int state, int x, int y)
    {
        if (state == GLUT_UP)
        {
            instance_->sectionView_.OnMouseUp();
            if (button == GLUT_LEFT_BUTTON) instance_->sceneView_.OnMouseUp();
            if (button == GLUT_RIGHT_BUTTON) instance_->sceneView_.OnPanUp();
        }
        if (instance_->sectionView_.Contains(x, y, instance_->windowHeight_))
        {
            if (state == GLUT_DOWN && (button == GLUT_LEFT_BUTTON || button == GLUT_RIGHT_BUTTON))
            {
                instance_->sceneView_.OnMouseUp();
                instance_->sceneView_.OnPanUp();
                instance_->sectionView_.OnMouseDown(x, y);
            }
            if (state == GLUT_DOWN && button == GLUT_MIDDLE_BUTTON) instance_->sectionView_.ResetView();
            glutPostRedisplay();
            return;
        }
        if (button == GLUT_RIGHT_BUTTON)
        {
            if (state == GLUT_DOWN && instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
            {
                instance_->sceneView_.OnPanDown(x, y);
            }
            else if (state == GLUT_UP)
            {
                instance_->sceneView_.OnPanUp();
            }

            glutPostRedisplay();

            return;
        }

        if (button != GLUT_LEFT_BUTTON)
        {
            return;
        }

        glutPostRedisplay();

        if (state == GLUT_DOWN && instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
        {
            instance_->sceneView_.OnMouseDown(x, y);
        }
        else
        {
            instance_->sceneView_.OnMouseUp();
        }
    }

    void MainWindow::MouseMotion(int x, int y)
    {
        instance_->sceneView_.OnMouseMove(x, y);
        instance_->sectionView_.OnMouseMove(x, y);

        glutPostRedisplay();
    }

    void MainWindow::MouseWheel(int, int direction, int x, int y)
    {
        if (instance_->sectionView_.Contains(x, y, instance_->windowHeight_))
        {
            instance_->sectionView_.OnWheel(direction, x, y, instance_->windowHeight_);
            glutPostRedisplay();
            return;
        }
        if (instance_->sceneView_.Contains(x, y, instance_->windowHeight_))
        {
            instance_->sceneView_.OnWheel(direction);
            glutPostRedisplay();
        }
    }

    void MainWindow::Keyboard(unsigned char key, int x, int y)
    {
        auto& model = instance_->viewModel_;

        if (instance_->parametersPanel_.Editing())
            return;

        const auto& parameters = model.Parameters();

        if ((model.IsHeightField() || model.IsIntegralBody() || model.IsImplicitSurface() || model.IsInequalityBody()) && key != ' ' && key != 'c' && key != 'C')
        {
            return;
        }

        constexpr double Step = 0.1;

        try
        {
            switch (key)
            {
            case 'q':
            case 'Q':
                model.SetBaseHeight(parameters.BaseHeight + Step);
                instance_->ReloadMesh();
                break;
            case 'a':
            case 'A':
                model.SetBaseHeight(parameters.BaseHeight - Step);
                instance_->ReloadMesh();
                break;
            case 'w':
            case 'W':
                model.SetJoinRadius(parameters.JoinRadius + Step);
                instance_->ReloadMesh();
                break;
            case 's':
            case 'S':
                model.SetJoinRadius(parameters.JoinRadius - Step);
                instance_->ReloadMesh();
                break;
            case 'e':
            case 'E':
                model.SetBaseExponent(parameters.BaseExponent + Step);
                instance_->ReloadMesh();
                break;
            case 'd':
            case 'D':
                model.SetBaseExponent(parameters.BaseExponent - Step);
                instance_->ReloadMesh();
                break;
            case 'r':
            case 'R':
                model.SetCapHeight(parameters.CapHeight + Step);
                instance_->ReloadMesh();
                break;
            case 'f':
            case 'F':
                model.SetCapHeight(parameters.CapHeight - Step);
                instance_->ReloadMesh();
                break;
            case 'c':
            case 'C':
                if (instance_->sectionView_.Contains(x, y, instance_->windowHeight_)) instance_->sectionView_.ResetView();
                else instance_->sceneView_.ResetCamera();
                break;
            case ' ':
                model.ResetDefaults();
                instance_->sectionView_.ResetView();
                instance_->sceneView_.ResetCamera();
                instance_->ReloadMesh();
                break;
            default:
                return;
            }
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << '\n';
        }

        glutPostRedisplay();
    }

    void MainWindow::Special(int key, int, int)
    {
        auto& model = instance_->viewModel_;

        if (instance_->parametersPanel_.Editing())
        {
            return;
        }

        constexpr double OffsetStep = 0.05;
        constexpr double AngleStep = 5.0;

        switch (key)
        {
        case GLUT_KEY_LEFT:
            model.SetSectionPosition(2, std::max(-ViewModels::MainViewModel::SectionPositionLimit, model.SectionPosition().Z - OffsetStep));
            break;
        case GLUT_KEY_RIGHT:
            model.SetSectionPosition(2, std::min(ViewModels::MainViewModel::SectionPositionLimit, model.SectionPosition().Z + OffsetStep));
            break;
        case GLUT_KEY_UP:
            model.SetSectionRotation(1, model.SectionRotation().Y + AngleStep);
            break;
        case GLUT_KEY_DOWN:
            model.SetSectionRotation(1, model.SectionRotation().Y - AngleStep);
            break;
        default:
            return;
        }

        glutPostRedisplay();
    }

    void MainWindow::Close()
    {
        instance_->toolbarText_.Destroy();
        instance_->parametersPanel_.Destroy();
        instance_->sceneView_.Destroy();
        instance_->sectionView_.Destroy();
    }
} // namespace IceCream::App::Views
