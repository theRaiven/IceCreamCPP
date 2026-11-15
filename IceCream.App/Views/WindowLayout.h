#pragma once

#include <algorithm>

namespace IceCream::App::Views
{
    // Window bounds here use pixels with the origin at the upper left.
    struct ViewBounds
    {
        int X = 0, Y = 0, Width = 0, Height = 0;
    };

    struct WindowLayout
    {
        static constexpr int ToolbarHeight = 48;
        static constexpr int MinimumWidth = 320;
        static constexpr int MinimumHeight = 250;

        ViewBounds Panel, Scene, Section, Readouts;
        int TabsWidth = 0;

        static WindowLayout Calculate(int width, int height, int dpi, bool panelOpen)
        {
            auto pixels = [dpi](int value) { return (value * dpi + 48) / 96; };
            const int toolbar = std::min(height, pixels(ToolbarHeight));
            WindowLayout layout;
            const int preferredPanel = std::clamp(width * 32 / 100, pixels(360), pixels(440));
            const bool drawer = width < preferredPanel + pixels(320);
            layout.TabsWidth = drawer ? width : preferredPanel;
            if (panelOpen && drawer)
            {
                layout.Panel = {0, 0, width, height};
                return layout;
            }

            const int left = panelOpen ? preferredPanel : 0;
            const bool readoutsFit = toolbar > 0 && width - layout.TabsWidth >= pixels(600);
            if (readoutsFit)
                layout.Readouts = {layout.TabsWidth, 0, width - layout.TabsWidth, toolbar};
            const int top = panelOpen && !readoutsFit ? 0 : toolbar;
            layout.Panel = {0, 0, panelOpen ? left : layout.TabsWidth, panelOpen ? height : toolbar};
            const int availableWidth = width - left;
            const int availableHeight = height - top;
            if (availableWidth >= pixels(650) && availableWidth >= availableHeight)
            {
                const int sceneWidth = availableWidth * 58 / 100;
                layout.Scene = {left, top, sceneWidth, availableHeight};
                layout.Section = {left + sceneWidth, top, availableWidth - sceneWidth, availableHeight};
            }
            else
            {
                const int sceneHeight = availableHeight * 57 / 100;
                layout.Scene = {left, top, availableWidth, sceneHeight};
                layout.Section = {left, top + sceneHeight, availableWidth, availableHeight - sceneHeight};
            }
            return layout;
        }
    };
}
