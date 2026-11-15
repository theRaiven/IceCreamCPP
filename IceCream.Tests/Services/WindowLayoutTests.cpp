#include "../pch.h"
#include "../../IceCream.App/Views/WindowLayout.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using IceCream::App::Views::ViewBounds;
using IceCream::App::Views::WindowLayout;

namespace IceCream::Tests::Views
{
    TEST_CLASS(WindowLayoutTests)
    {
        static bool Overlap(const ViewBounds& a, const ViewBounds& b)
        {
            return std::max(a.X, b.X) < std::min(a.X + a.Width, b.X + b.Width) &&
                std::max(a.Y, b.Y) < std::min(a.Y + a.Height, b.Y + b.Height);
        }

    public:
        TEST_METHOD(PanelAndPlotsStayInsideTheWindowAtDifferentScales)
        {
            for (int dpi : {96, 120, 144, 192, 240})
            {
                for (int width : {320, 600, 679, 680, 1024, 1440, 2560})
                {
                    for (int height : {250, 540, 820, 1440})
                    {
                        const int w = (width * dpi + 48) / 96;
                        const int h = (height * dpi + 48) / 96;
                        const auto closed = WindowLayout::Calculate(w, h, dpi, false);
                        const auto open = WindowLayout::Calculate(w, h, dpi, true);
                        Assert::AreEqual(closed.TabsWidth, open.TabsWidth);
                        for (const auto& layout : {closed, open})
                        {
                            for (const auto& bounds : {layout.Panel, layout.Scene, layout.Section, layout.Readouts})
                            {
                                Assert::IsTrue(bounds.X >= 0 && bounds.Y >= 0 &&
                                    bounds.Width >= 0 && bounds.Height >= 0 &&
                                    bounds.X + bounds.Width <= w && bounds.Y + bounds.Height <= h);
                            }
                            Assert::IsFalse(Overlap(layout.Scene, layout.Section));
                            Assert::IsFalse(Overlap(layout.Panel, layout.Scene));
                            Assert::IsFalse(Overlap(layout.Panel, layout.Section));
                            Assert::IsFalse(Overlap(layout.Readouts, layout.Panel));
                            Assert::IsFalse(Overlap(layout.Readouts, layout.Scene));
                            Assert::IsFalse(Overlap(layout.Readouts, layout.Section));
                        }
                    }
                }
            }
        }

        TEST_METHOD(NarrowWindowsStackPlotsAndGiveAnOpenPanelTheFullWidth)
        {
            const auto closed = WindowLayout::Calculate(520, 600, 96, false);
            Assert::AreEqual(520, closed.Scene.Width);
            Assert::AreEqual(520, closed.Section.Width);
            Assert::AreEqual(closed.Scene.Y + closed.Scene.Height, closed.Section.Y);
            Assert::AreEqual(520, closed.TabsWidth);
            Assert::AreEqual(0, closed.Readouts.Width);
            const auto open = WindowLayout::Calculate(520, 600, 96, true);
            Assert::AreEqual(520, open.Panel.Width);
            Assert::AreEqual(600, open.Panel.Height);
            Assert::AreEqual(0, open.Scene.Width);
            Assert::AreEqual(0, open.Section.Width);
        }

        TEST_METHOD(WideWindowsKeepBothPlotsVisibleBesideTheOpenPanel)
        {
            const auto layout = WindowLayout::Calculate(2880, 1640, 192, true);
            Assert::AreEqual(layout.Panel.Width, layout.Scene.X);
            Assert::AreEqual(layout.Scene.X + layout.Scene.Width, layout.Section.X);
            Assert::AreEqual(layout.TabsWidth, layout.Readouts.X);
            Assert::AreEqual(2880, layout.Readouts.X + layout.Readouts.Width);
            Assert::AreEqual(96, layout.Readouts.Height);
            Assert::AreEqual(layout.Readouts.Height, layout.Scene.Y);
            Assert::AreEqual(layout.Readouts.Height, layout.Section.Y);
            Assert::AreEqual(1640, layout.Scene.Y + layout.Scene.Height);
            Assert::AreEqual(1640, layout.Section.Y + layout.Section.Height);
        }
    };
}
