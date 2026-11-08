#pragma once
#include <array>
#include <cstddef>
#include <cmath>
#include <glm/glm.hpp>
#include <stdexcept>

namespace IceCream::App::ViewModels
{
enum class ColorRole
{
    Scene,
    Section,
    Panel,
    Text,
    Grid,
    Slider,
    Body,
    Cone,
    Cap,
    Contour,
    Seam,
    AxisX,
    AxisY,
    AxisZ,
    Count
};
struct AppearanceSettings
{
    bool LightTheme = false;
    bool ShowHints = false;
    bool ShowConstraintSurfaces = true;
    int HintSize = 14;
    int CoordinateSize = 20;
    std::array<glm::vec4, static_cast<size_t>(ColorRole::Count)> Colors{};
    AppearanceSettings()
    {
        SetTheme(false);
    }
    const glm::vec4 &Color(ColorRole role) const
    {
        return Colors.at(static_cast<size_t>(role));
    }
    void SetColor(ColorRole role, glm::vec4 color)
    {
        for (int i = 0; i < 4; ++i)
            if (!std::isfinite(color[i]) || color[i] < 0 || color[i] > 1)
                throw std::invalid_argument("Color channel outside 0..1");
        Colors.at(static_cast<size_t>(role)) = color;
    }
    void SetTheme(bool light)
    {
        const auto previous = Colors;
        bool initialized = Colors[static_cast<size_t>(ColorRole::Body)].a > 0;
        LightTheme = light;
        Colors = {{light ? glm::vec4(.94f, .96f, .98f, 1) : glm::vec4(.12f, .15f, .20f, 1),
                   light ? glm::vec4(1, 1, 1, 1) : glm::vec4(.08f, .09f, .12f, 1),
                   light ? glm::vec4(.97f, .97f, .98f, 1) : glm::vec4(.12f, .14f, .18f, 1),
                   light ? glm::vec4(.12f, .16f, .22f, 1) : glm::vec4(.9f, .93f, .97f, 1),
                   light ? glm::vec4(.76f, .80f, .85f, 1) : glm::vec4(.23f, .28f, .34f, 1),
                   glm::vec4(.25f, .60f, .90f, 1), glm::vec4(.32f, .70f, .88f, 1), glm::vec4(.76f, .50f, .30f, 1),
                   glm::vec4(.60f, .85f, .35f, 1), light ? glm::vec4(.72f, .40f, .02f, 1) : glm::vec4(1, .82f, .30f, 1),
                   light ? glm::vec4(.70f, .12f, .55f, 1) : glm::vec4(.98f, .38f, .84f, 1),
                   glm::vec4(.90f, .24f, .22f, 1), glm::vec4(.15f, .65f, .30f, 1), glm::vec4(.20f, .48f, .95f, 1)}};
        if (initialized)
            for (size_t i = 5; i < Colors.size(); ++i)
                Colors[i] = previous[i];
    }
};
} // namespace IceCream::App::ViewModels
