#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <windows.h>

namespace IceCream::App::Views::Graphics
{
class OverlayText
{
    static constexpr int GlyphCount = 1280;
    struct BitmapFont
    {
        GLuint List = 0;
        std::array<int, GlyphCount> Advances{};
        int Ascent = 0, Height = 0;
    };
    struct FontCache
    {
        HGLRC Context;
        std::map<int, BitmapFont> Sizes;
        explicit FontCache(HGLRC context) : Context(context)
        {
        }
        ~FontCache()
        {
            if (wglGetCurrentContext() == Context)
                for (const auto &font : Sizes)
                    glDeleteLists(font.second.List, GlyphCount);
        }
    };
    static std::shared_ptr<FontCache> ContextFonts()
    {
        static std::map<HGLRC, std::weak_ptr<FontCache>> caches;
        auto context = wglGetCurrentContext();
        if (!context)
            throw std::runtime_error("No OpenGL context for coordinate font");
        auto cache = caches[context].lock();
        if (!cache)
        {
            cache = std::make_shared<FontCache>(context);
            caches[context] = cache;
        }
        return cache;
    }
    mutable std::shared_ptr<FontCache> fonts_;
    mutable const BitmapFont* font_ = nullptr;
    mutable int size_ = 0;

    static std::wstring Widen(const char* text)
    {
        const int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        if (count <= 1) return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), count);
        wide.pop_back();
        std::wstring result;
        for (wchar_t ch : wide)
        {
            if (ch == L'≤') result += L"<=";
            else if (ch == L'≥') result += L">=";
            else if (ch == L'–' || ch == L'—' || ch == L'−') result += L'-';
            else if (ch == L'…') result += L"...";
            else result += ch < GlyphCount ? ch : L'?';
        }
        return result;
    }

    int Width(std::wstring_view text) const
    {
        int width = 0;
        for (wchar_t ch : text)
            width += font_->Advances[ch];
        return width;
    }

  public:
    void Initialize()
    {
        SetSize(20);
    }

    void SetSize(int size) const
    {
        if (size_ == size && font_)
            return;
        if (!fonts_ || fonts_->Context != wglGetCurrentContext())
            fonts_ = ContextFonts();
        auto existing = fonts_->Sizes.find(size);
        if (existing != fonts_->Sizes.end())
        {
            font_ = &existing->second;
            size_ = size;
            return;
        }
        BitmapFont bitmap;
        bitmap.List = glGenLists(GlyphCount);
        HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HDC dc = wglGetCurrentDC();
        auto previous = SelectObject(dc, font);
        TEXTMETRICW metrics{};
        const BOOL ok = bitmap.List && font && wglUseFontBitmapsW(dc, 0, GlyphCount, bitmap.List) &&
            GetTextMetricsW(dc, &metrics);
        bitmap.Ascent = metrics.tmAscent;
        bitmap.Height = metrics.tmHeight + metrics.tmExternalLeading;
        bitmap.Advances.fill(std::max<LONG>(1, metrics.tmMaxCharWidth));
        GetCharWidth32W(dc, 0, GlyphCount - 1, bitmap.Advances.data());
        SelectObject(dc, previous);
        if (font)
            DeleteObject(font);
        if (!ok)
        {
            if (bitmap.List)
                glDeleteLists(bitmap.List, GlyphCount);
            throw std::runtime_error("Cannot initialize coordinate font");
        }
        font_ = &fonts_->Sizes.emplace(size, std::move(bitmap)).first->second;
        size_ = size;
    }
    void Destroy()
    {
        font_ = nullptr;
        size_ = 0;
        fonts_.reset();
    }

    int Ascent() const { return font_->Ascent; }
    int Height() const { return font_->Height; }
    int Measure(const char* text) const { return Width(Widen(text)); }

    std::vector<std::wstring> Wrap(const char* text, int width, int maxLines) const
    {
        const auto wide = Widen(text);
        std::wstring_view remaining(wide);
        std::vector<std::wstring> lines;
        while (!remaining.empty() && static_cast<int>(lines.size()) < maxLines)
        {
            size_t end = 0;
            int length = 0;
            while (end < remaining.size() && length + font_->Advances[remaining[end]] <= width)
                length += font_->Advances[remaining[end++]];
            if (end == remaining.size())
            {
                lines.emplace_back(remaining);
                break;
            }
            if (static_cast<int>(lines.size()) + 1 == maxLines)
            {
                const int ellipsis = Width(L"...");
                while (end > 0 && length + ellipsis > width)
                    length -= font_->Advances[remaining[--end]];
                if (ellipsis <= width)
                    lines.emplace_back(std::wstring(remaining.substr(0, end)) + L"...");
                break;
            }
            if (end == 0)
                break;
            const size_t space = remaining.rfind(L' ', end);
            if (space != std::wstring_view::npos && space != 0)
                end = space;
            lines.emplace_back(remaining.substr(0, end));
            remaining.remove_prefix(end);
            while (!remaining.empty() && remaining.front() == L' ')
                remaining.remove_prefix(1);
        }
        return lines;
    }

    void World(const glm::vec3 &point, const glm::mat4 &mvp, const glm::vec3 &color, std::wstring_view text) const
    {
        GLint mode;
        glGetIntegerv(GL_MATRIX_MODE, &mode);
        glPushAttrib(GL_CURRENT_BIT | GL_LIST_BIT);
        glUseProgram(0);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(glm::value_ptr(mvp));
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glColor3f(color.r, color.g, color.b);
        glRasterPos3f(point.x, point.y, point.z);
        glListBase(font_->List);
        glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_SHORT, text.data());
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(mode);
        glPopAttrib();
    }

    void World(const glm::vec3 &point, const glm::mat4 &mvp, const glm::vec3 &color, const char* text) const
    {
        World(point, mvp, color, Widen(text));
    }

    void Screen(float x, float y, int width, int height, const glm::vec3 &color, std::wstring_view text) const
    {
        glm::mat4 projection(1);
        projection[0][0] = 2.0f / width;
        projection[1][1] = 2.0f / height;
        projection[3][0] = -1;
        projection[3][1] = -1;
        World({x, y, 0}, projection, color, text);
    }

    void Screen(float x, float y, int width, int height, const glm::vec3 &color, const char* text) const
    {
        Screen(x, y, width, height, color, Widen(text));
    }
};

// Screen text sits above the plot, outside the camera's working area.
class OverlayHeader
{
    struct Row
    {
        const OverlayText& Font;
        glm::vec3 Color;
        std::wstring Text;
    };
    std::vector<Row> rows_;
    int width_, padding_, gap_, limit_, used_;

    int RowHeight(const OverlayText& font) const { return font.Height() + gap_; }

  public:
    OverlayHeader(int width, int height, int dpi)
        : width_(width), padding_(MulDiv(height < MulDiv(120, dpi, 96) ? 5 : 8, dpi, 96)), gap_(MulDiv(2, dpi, 96)),
          limit_(std::min(height / 2, std::max(MulDiv(42, dpi, 96), height / 3))), used_(padding_ * 2)
    {
    }

    int AvailableRows(const OverlayText& font) const
    {
        return std::max(0, (limit_ - used_) / RowHeight(font));
    }

    void Add(const OverlayText& font, const glm::vec3& color, const char* text, int maxLines = 1)
    {
        auto lines = font.Wrap(text, width_ - 2 * padding_, std::min(maxLines, AvailableRows(font)));
        for (auto& line : lines)
        {
            rows_.push_back({font, color, std::move(line)});
            used_ += RowHeight(font);
        }
    }

    int Height() const { return rows_.empty() ? 0 : used_; }

    void Draw(int x, int y, int height) const
    {
        if (rows_.empty()) return;
        glViewport(x, y + height - used_, width_, used_);
        int top = used_ - padding_;
        for (const auto& row : rows_)
        {
            row.Font.Screen(static_cast<float>(padding_), static_cast<float>(top - row.Font.Ascent()),
                width_, used_, row.Color, row.Text);
            top -= RowHeight(row.Font);
        }
    }
};
} // namespace IceCream::App::Views::Graphics
