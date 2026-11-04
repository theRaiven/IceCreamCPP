#include "ParametersPanel.h"
#include <cmath>
namespace IceCream::App::Views
{
void ParametersPanel::ApplyLighting()
{
    try
    {
        auto light = model_->Lighting();
        light.Type = static_cast<Graphics::LightType>(SendMessageW(Control(300), CB_GETCURSEL, 0, 0));
        light.AmbientIntensity = static_cast<float>(Number(310));
        light.EmissionIntensity = static_cast<float>(Number(311));
        light.LightIntensity = static_cast<float>(Number(312));
        if (light.AmbientIntensity < 0 || light.AmbientIntensity > 1 || light.EmissionIntensity < 0 ||
            light.EmissionIntensity > 1 || light.LightIntensity < 0 || light.LightIntensity > 5)
            throw std::out_of_range("Intensity");
        auto vector = [&](int id) {
            glm::vec3 v;
            for (int i = 0; i < 3; ++i)
            {
                double n = Number(id + i);
                if (std::abs(n) > 100)
                    throw std::out_of_range("Light position");
                v[i] = static_cast<float>(n);
            }
            return v;
        };
        if (light.Type != Graphics::LightType::Directional)
            light.Position = vector(313);
        if (light.Type != Graphics::LightType::Point)
        {
            light.Direction = vector(316);
            if (glm::length(light.Direction) < 1e-6f)
                throw std::invalid_argument("Zero direction");
        }
        if (light.Type == Graphics::LightType::Spot)
        {
            light.SpotAngle = static_cast<float>(Number(319));
            light.SpotExponent = static_cast<float>(Number(320));
            if (light.SpotAngle < .1f || light.SpotAngle > 89.9f || light.SpotExponent < 0 || light.SpotExponent > 128)
                throw std::out_of_range("Spot");
        }
        model_->SetLighting(light);
        SetError(L"");
        Notify();
    }
    catch (const std::exception &)
    {
        SetError(L"Фон и свечение: 0…1; сила: 0…5; координаты: ±100; угол: 0.1…89.9; концентрация: 0…128. Направление "
                 L"не нулевое.");
    }
}
} // namespace IceCream::App::Views
