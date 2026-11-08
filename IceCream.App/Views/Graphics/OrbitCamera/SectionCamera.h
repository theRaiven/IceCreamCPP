#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace IceCream::App::Views::Graphics
{
struct SectionBounds
{
    double MinX, MaxX, MinY, MaxY;
};

// Orthographic navigation, independent of OpenGL and the section geometry.
class SectionCamera
{
    int width_ = 1, height_ = 1;
    double centerX_ = 0, centerY_ = 0, spanY_ = 4;
    SectionBounds content_{-1.5, 1.5, -1.5, 1.5};
    bool autoFit_ = true;
    static double MinimumSpan(double x, double y)
    {
        return std::max(1e-6, 64*std::numeric_limits<double>::epsilon()*std::max({1.0,std::abs(x),std::abs(y)}));
    }

    void Fit()
    {
        centerX_ = (content_.MinX + content_.MaxX) * .5;
        centerY_ = (content_.MinY + content_.MaxY) * .5;
        double extent = std::max(content_.MaxY-content_.MinY,
            (content_.MaxX-content_.MinX) * height_ / width_);
        spanY_ = extent > 0 ? std::max(MinimumSpan(centerX_,centerY_), extent * 1.15) : 1;
    }

  public:
    void SetViewport(int width, int height)
    {
        width_ = std::max(1, width); height_ = std::max(1, height);
        if (autoFit_) Fit();
    }
    void SetContentBounds(SectionBounds bounds)
    {
        if (!std::isfinite(bounds.MinX) || !std::isfinite(bounds.MaxX) || !std::isfinite(bounds.MinY) ||
            !std::isfinite(bounds.MaxY) || bounds.MinX > bounds.MaxX || bounds.MinY > bounds.MaxY) return;
        content_ = bounds;
        if (autoFit_) Fit();
    }
    void ResetView() { autoFit_ = true; Fit(); }
    bool AutoFit() const { return autoFit_; }
    double CenterX() const { return centerX_; }
    double CenterY() const { return centerY_; }
    double SpanY() const { return spanY_; }
    SectionBounds Bounds() const
    {
        double halfX = spanY_ * width_ / height_ * .5, halfY = spanY_ * .5;
        return {centerX_-halfX, centerX_+halfX, centerY_-halfY, centerY_+halfY};
    }
    std::pair<double, double> WorldAt(double pixelX, double pixelY) const
    {
        double units = spanY_ / height_;
        return {centerX_ + (pixelX-width_*.5)*units, centerY_ + (height_*.5-pixelY)*units};
    }
    void PanPixels(double dx, double dy)
    {
        if (!std::isfinite(dx) || !std::isfinite(dy)) return;
        autoFit_ = false;
        centerX_ = std::clamp(centerX_-dx*spanY_/height_, -1e12, 1e12);
        centerY_ = std::clamp(centerY_+dy*spanY_/height_, -1e12, 1e12);
        spanY_ = std::max(spanY_, MinimumSpan(centerX_,centerY_));
    }
    void ZoomAt(int direction, double pixelX, double pixelY)
    {
        if (!direction || !std::isfinite(pixelX) || !std::isfinite(pixelY)) return;
        auto [x,y] = WorldAt(pixelX, pixelY);
        double factor = std::pow(1.15, -std::clamp(direction, -100, 100));
        spanY_ = std::clamp(spanY_*factor, MinimumSpan(x,y), 1e9);
        centerX_ = std::clamp(x-(pixelX-width_*.5)*spanY_/height_, -1e12, 1e12);
        centerY_ = std::clamp(y-(height_*.5-pixelY)*spanY_/height_, -1e12, 1e12);
        autoFit_ = false;
    }
};
}
