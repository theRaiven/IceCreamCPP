#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <windows.h>

namespace IceCream::App::Views::Graphics
{
class OverlayText
{
    struct FontCache
    {
        HGLRC Context;
        std::map<int, GLuint> Sizes;
        explicit FontCache(HGLRC context) : Context(context)
        {
        }
        ~FontCache()
        {
            if (wglGetCurrentContext() == Context)
                for (const auto &font : Sizes)
                    glDeleteLists(font.second, 1280);
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
    mutable GLuint font_ = 0;
    mutable int size_ = 0;

    void Emit(const char *text) const
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        std::wstring wide(count, L' ');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), count);
        glListBase(font_);
        glCallLists(count - 1, GL_UNSIGNED_SHORT, wide.data());
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
            font_ = existing->second;
            size_ = size;
            return;
        }
        GLuint lists = glGenLists(1280);
        HFONT font = CreateFontW(-size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                 CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        HDC dc = wglGetCurrentDC();
        auto previous = SelectObject(dc, font);
        BOOL ok = lists && font && wglUseFontBitmapsW(dc, 0, 1280, lists);
        SelectObject(dc, previous);
        if (font)
            DeleteObject(font);
        if (!ok)
        {
            if (lists)
                glDeleteLists(lists, 1280);
            throw std::runtime_error("Cannot initialize coordinate font");
        }
        fonts_->Sizes.emplace(size, lists);
        font_ = lists;
        size_ = size;
    }
    void Destroy()
    {
        font_ = 0;
        size_ = 0;
        fonts_.reset();
    }

    void World(const glm::vec3 &point, const glm::mat4 &mvp, const glm::vec3 &color, const char *text) const
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
        Emit(text);
        glPopMatrix();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(mode);
        glPopAttrib();
    }

    void Screen(float x, float y, int width, int height, const glm::vec3 &color, const char *text) const
    {
        glm::mat4 projection(1);
        projection[0][0] = 2.0f / width;
        projection[1][1] = 2.0f / height;
        projection[3][0] = -1;
        projection[3][1] = -1;
        World({x, y, 0}, projection, color, text);
    }
};
} // namespace IceCream::App::Views::Graphics
