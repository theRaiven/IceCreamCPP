#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <stdexcept>

namespace IceCream::App::Views::Graphics
{
    class OverlayText
    {
        GLuint font_ = 0;
        void Emit(const char* text) const
        {
            int count=MultiByteToWideChar(CP_UTF8,0,text,-1,nullptr,0);
            std::wstring wide(count,L' ');
            MultiByteToWideChar(CP_UTF8,0,text,-1,wide.data(),count);
            glListBase(font_); glCallLists(count-1,GL_UNSIGNED_SHORT,wide.data());
        }
    public:
        void Initialize()
        {
            font_=glGenLists(1280);
            HFONT font=CreateFontW(-13,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
            HDC dc=wglGetCurrentDC(); auto previous=SelectObject(dc,font);
            BOOL ok=wglUseFontBitmapsW(dc,0,1280,font_);
            SelectObject(dc,previous); DeleteObject(font);
            if(!ok) throw std::runtime_error("Cannot initialize coordinate font");
        }
        void Destroy() { if(font_) glDeleteLists(font_,1280); font_=0; }
        void World(const glm::vec3& point,const glm::mat4& mvp,const glm::vec3& color,const char* text) const
        {
            GLint mode; glGetIntegerv(GL_MATRIX_MODE,&mode);
            glPushAttrib(GL_CURRENT_BIT|GL_LIST_BIT);
            glUseProgram(0);
            glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadMatrixf(glm::value_ptr(mvp));
            glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
            glColor3f(color.r,color.g,color.b); glRasterPos3f(point.x,point.y,point.z); Emit(text);
            glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(mode);
            glPopAttrib();
        }
        void Screen(float x,float y,int width,int height,const glm::vec3& color,const char* text) const
        {
            glm::mat4 projection(1);
            projection[0][0]=2.0f/width; projection[1][1]=2.0f/height;
            projection[3][0]=-1; projection[3][1]=-1;
            World({x,y,0},projection,color,text);
        }
    };
}
