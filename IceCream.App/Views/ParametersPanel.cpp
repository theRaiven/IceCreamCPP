#include "ParametersPanel.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <cctype>
#include <iostream>

namespace IceCream::App::Views
{
    namespace { GLuint textFont = 0; }
    void ParametersPanel::Initialize(ViewModels::MainViewModel& model, std::function<void()> onChanged)
    {
        font_ = glGenLists(1280);
        HFONT font = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH, L"Segoe UI");
        HDC dc = wglGetCurrentDC();
        auto previous = SelectObject(dc, font);
        const BOOL ok = wglUseFontBitmapsW(dc, 0, 1280, font_);
        SelectObject(dc, previous);
        DeleteObject(font);
        if (!ok) throw std::runtime_error("Cannot initialize panel font.");
        textFont = font_;
        model_ = &model;
        onChanged_ = std::move(onChanged);
        parent_ = WindowFromDC(dc);
        nativeInstance_ = this;
        SetWindowLongPtrW(parent_, GWL_STYLE, GetWindowLongPtrW(parent_, GWL_STYLE) | WS_CLIPCHILDREN);
        parentProcedure_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(parent_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ParentProcedure)));
        nativeFont_ = CreateFontW(-15,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
        for(int i=0;i<5;++i) {
            DWORD style=WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_BORDER|ES_AUTOVSCROLL;
            style |= i==4 ? ES_MULTILINE|WS_VSCROLL : ES_AUTOHSCROLL;
            editors_[i]=CreateWindowExW(0,L"EDIT",L"",style,0,0,1,1,parent_,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(1000+i)),GetModuleHandleW(nullptr),nullptr);
            if(!editors_[i]) throw std::runtime_error("Cannot create text editor");
            SendMessageW(editors_[i],WM_SETFONT,reinterpret_cast<WPARAM>(nativeFont_),TRUE);
            SendMessageW(editors_[i],EM_SETLIMITTEXT,i==4?512:32,0);
            auto previousEdit = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(editors_[i],GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(EditProcedure)));
            if(!editProcedure_) editProcedure_=previousEdit;
        }
        applyButton_=CreateWindowW(L"BUTTON",L"Построить / Enter",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
            0,0,1,1,parent_,reinterpret_cast<HMENU>(1006),GetModuleHandleW(nullptr),nullptr);
        sideButton_=CreateWindowW(L"BUTTON",L"Прозрачная сторона: +",WS_CHILD|WS_VISIBLE|WS_TABSTOP,
            0,0,1,1,parent_,reinterpret_cast<HMENU>(1007),GetModuleHandleW(nullptr),nullptr);
        for(auto button : {applyButton_,sideButton_}) SendMessageW(button,WM_SETFONT,reinterpret_cast<WPARAM>(nativeFont_),TRUE);
        shapePicker_=CreateWindowW(L"COMBOBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|CBS_DROPDOWNLIST|WS_VSCROLL,
            0,0,1,1,parent_,reinterpret_cast<HMENU>(1008),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(shapePicker_,WM_SETFONT,reinterpret_cast<WPARAM>(nativeFont_),TRUE);
        for(const wchar_t* item : {L"Мороженое",L"Цилиндр",L"Конус",L"Шар",L"Своя поверхность z=f(x,y)"})
            SendMessageW(shapePicker_,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(item));
        SyncEditors();
    }
    void ParametersPanel::Destroy()
    {
        if(parent_ && parentProcedure_) SetWindowLongPtrW(parent_,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(parentProcedure_));
        for(auto edit:editors_) if(edit) DestroyWindow(edit);
        if(shapePicker_) DestroyWindow(shapePicker_);
        if(applyButton_) DestroyWindow(applyButton_);
        if(sideButton_) DestroyWindow(sideButton_);
        if(nativeFont_) DeleteObject(nativeFont_);
        nativeInstance_=nullptr;
        if(font_) glDeleteLists(font_,1280);
        font_=textFont=0;
    }
    bool ParametersPanel::Editing() const
    {
        HWND focus=GetFocus();
        return focus==shapePicker_ || activeField_>=0 || std::find(editors_.begin(),editors_.end(),focus)!=editors_.end();
    }
    const char* ParametersPanel::ShapeFormula(int mode)
    {
        switch(mode) {
        case 0: return Core::Profiles::FormulaProfile::DefaultFormula();
        case 1: return "radius";
        case 2: return "radius*z/height";
        case 3: return "sqrt(max(0,radius^2-(z-radius)^2))";
        default: return "x^2 + y^2";
        }
    }
    int ParametersPanel::DetectShape(const std::string& formula)
    {
        for(int mode=0;mode<4;++mode) if(formula==ShapeFormula(mode)) return mode;
        return 4;
    }
    void ParametersPanel::SelectShape(int mode)
    {
        if(mode<0 || mode>4) return;
        if(shapeMode_==4) {
            char text[513]{}; GetWindowTextA(editors_[4],text,sizeof(text)); customDraft_=text;
        }
        shapeMode_=mode;
        error_.clear();
        ConfigureShapeEditors();
    }
    void ParametersPanel::ConfigureShapeEditors()
    {
        SendMessageW(shapePicker_,CB_SETCURSEL,shapeMode_,0);
        const auto& p=model_->Parameters();
        const auto& domain=model_->SurfaceBounds();
        double values[]={shapeMode_==0?p.BaseHeight:shapeMode_==3?p.JoinRadius:p.BaseHeight+p.CapHeight,
            p.JoinRadius,p.BaseExponent,p.CapHeight};
        if(shapeMode_==4) { values[0]=domain.MinX; values[1]=domain.MaxX; values[2]=domain.MinY; values[3]=domain.MaxY; }
        for(int i=0;i<4;++i) {
            char buffer[64]; std::snprintf(buffer,sizeof(buffer),"%.6g",values[i]);
            SetWindowTextA(editors_[i],buffer);
            bool visible=shapeMode_==4 || shapeMode_==0 || i==0 || (i==1 && shapeMode_!=3);
            ShowWindow(editors_[i],visible?SW_SHOW:SW_HIDE);
        }
        ShowWindow(editors_[4],shapeMode_==4?SW_SHOW:SW_HIDE);
        if(shapeMode_==4) SetWindowTextA(editors_[4],customDraft_.c_str());
    }
    void ParametersPanel::SyncEditors()
    {
        if(!model_ || nativeRevision_==model_->GeometryRevision()) return;
        if(!applyingShape_ && model_->Formula()!=lastFormula_) shapeMode_=model_->IsHeightField()?4:DetectShape(model_->Formula());
        if(shapeMode_==4 && model_->IsHeightField()) customDraft_=model_->Formula();
        lastFormula_=model_->Formula();
        ConfigureShapeEditors();
        nativeRevision_=model_->GeometryRevision();
    }
    void ParametersPanel::ApplyEditors()
    {
        try {
            auto parameters=model_->Parameters();
            auto read=[this](int index) {
                char buffer[64]{}; GetWindowTextA(editors_[index],buffer,sizeof(buffer));
                std::string text=buffer; std::replace(text.begin(),text.end(),',','.');
                size_t end=0; double value=std::stod(text,&end);
                while(end<text.size() && std::isspace(static_cast<unsigned char>(text[end]))) ++end;
                if(end!=text.size() || !std::isfinite(value)) throw std::invalid_argument("Invalid number");
                return value;
            };
            if(shapeMode_==4) {
                char text[513]{}; GetWindowTextA(editors_[4],text,sizeof(text));
                model_->ApplyHeightField({read(0),read(1),read(2),read(3)},text);
                applyingShape_=true; SyncEditors(); applyingShape_=false;
                error_.clear(); activeField_=-1; SetFocus(parent_); onChanged_(); return;
            }
            if(shapeMode_==0) parameters={read(0),read(1),read(2),read(3)};
            else {
                double height=read(0);
                if(shapeMode_==3) { parameters.JoinRadius=height; height*=2; }
                else parameters.JoinRadius=read(1);
                if(height<0.7 || height>12) throw std::out_of_range("Height must be between 0.7 and 12");
                // Map one intuitive total-height field onto the existing H+h domain.
                parameters.BaseHeight=std::clamp(height-.2,.5,8.0);
                parameters.CapHeight=std::max(.2,height-parameters.BaseHeight);
            }
            std::string formula=ShapeFormula(shapeMode_);
            if(shapeMode_==4) {
                char text[513]{}; GetWindowTextA(editors_[4],text,sizeof(text)); formula=text;
            }
            model_->ApplyShape(parameters,formula);
            applyingShape_=true;
            SyncEditors();
            applyingShape_=false;
            error_.clear(); activeField_=-1; SetFocus(parent_);
        } catch(const std::exception& e) {
            applyingShape_=false;
            error_=shapeMode_==4?"Не удалось построить: проверьте формулу и размеры":"Проверьте размеры: значение вне диапазона";
            std::cerr << e.what() << '\n';
        }
        onChanged_();
    }
    LRESULT CALLBACK ParametersPanel::ParentProcedure(HWND hwnd,UINT message,WPARAM w,LPARAM l)
    {
        auto* self=nativeInstance_;
        if(!self) return DefWindowProcW(hwnd,message,w,l);
        if(message==WM_COMMAND && LOWORD(w)==1008 && HIWORD(w)==CBN_SELCHANGE) {
            self->SelectShape(static_cast<int>(SendMessageW(self->shapePicker_,CB_GETCURSEL,0,0)));
            self->onChanged_(); return 0;
        }
        if(message==WM_COMMAND && HIWORD(w)==EN_SETFOCUS && LOWORD(w)>=1000 && LOWORD(w)<=1004)
            self->activeField_=-1;
        if(message==WM_COMMAND && HIWORD(w)==BN_CLICKED) {
            if(LOWORD(w)==1006) { self->ApplyEditors(); return 0; }
            if(LOWORD(w)==1007) {
                self->model_->ToggleTransparentSide();
                SetWindowTextW(self->sideButton_,self->model_->TransparentPositive()?L"Прозрачная сторона: +":L"Прозрачная сторона: -");
                SetFocus(self->parent_); self->onChanged_(); return 0;
            }
        }
        return CallWindowProcW(self->parentProcedure_,hwnd,message,w,l);
    }
    LRESULT CALLBACK ParametersPanel::EditProcedure(HWND hwnd,UINT message,WPARAM w,LPARAM l)
    {
        auto* self=nativeInstance_;
        if(!self) return DefWindowProcW(hwnd,message,w,l);
        if(message==WM_KEYDOWN) {
            if(w==VK_RETURN) { self->ApplyEditors(); return 0; }
            if(w==VK_ESCAPE) { self->shapeMode_=self->model_->IsHeightField()?4:DetectShape(self->model_->Formula()); self->nativeRevision_=static_cast<std::uint64_t>(-1); self->SyncEditors(); self->error_.clear(); SetFocus(self->parent_); self->onChanged_(); return 0; }
            if(w=='A' && (GetKeyState(VK_CONTROL)&0x8000)) { SendMessageW(hwnd,EM_SETSEL,0,-1); return 0; }
            if(w==VK_TAB) {
                auto found=std::find(self->editors_.begin(),self->editors_.end(),hwnd);
                int index=static_cast<int>(found-self->editors_.begin());
                do { index=(index+((GetKeyState(VK_SHIFT)&0x8000)?4:1))%5; }
                while(!IsWindowVisible(self->editors_[index]));
                SetFocus(self->editors_[index]); SendMessageW(self->editors_[index],EM_SETSEL,0,-1); return 0;
            }
        }
        if(message==WM_CHAR && (w==VK_RETURN || w==VK_TAB || w==VK_ESCAPE)) return 0;
        return CallWindowProcW(self->editProcedure_,hwnd,message,w,l);
    }
    void ParametersPanel::DrawString(int x, int y, const char* text)
    {
        const int count = MultiByteToWideChar(CP_UTF8, 0, text, -1, nullptr, 0);
        std::wstring wide(count, L' ');
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wide.data(), count);
        glRasterPos2i(x, y);
        glListBase(textFont);
        glCallLists(count - 1, GL_UNSIGNED_SHORT, wide.data());
    }
    void ParametersPanel::Box(int x, int y, int width, int height)
    {
        glBegin(GL_QUADS);
        glVertex2i(x,y); glVertex2i(x+width,y); glVertex2i(x+width,y+height); glVertex2i(x,y+height);
        glEnd();
    }
    int ParametersPanel::FieldTop(int field) { return field < 4 ? 82 + field * 30 : 360 + (field - 4) * 40; }
    double ParametersPanel::Value(const ViewModels::MainViewModel& m, int field)
    {
        const auto& p = m.Parameters(); const auto& o = m.SectionPosition(); const auto& r = m.SectionRotation();
        const double values[] = {p.BaseHeight,p.JoinRadius,p.BaseExponent,p.CapHeight,o.X,o.Y,o.Z,r.X,r.Y,r.Z};
        return values[field];
    }
    void ParametersPanel::SetValue(ViewModels::MainViewModel& m, int field, double value)
    {
        if (!std::isfinite(value)) throw std::invalid_argument("Invalid number");
        const double low[] = {0.5,0.2,0.2,0.2}; const double high[] = {8,4,3,4};
        if (field < 4 && (value < low[field] || value > high[field])) throw std::out_of_range("Range");
        switch(field) {
        case 0: m.SetBaseHeight(value); break; case 1: m.SetJoinRadius(value); break;
        case 2: m.SetBaseExponent(value); break; case 3: m.SetCapHeight(value); break;
        default:
            if (field < 7) m.SetSectionPosition(field-4,value);
            else { if (value < -180 || value > 180) throw std::out_of_range("Angle"); m.SetSectionRotation(field-7,value); }
        }
    }
    bool ParametersPanel::Commit(ViewModels::MainViewModel& model)
    {
        if (activeField_ < 0) return true;
        try {
            std::string number = input_; std::replace(number.begin(),number.end(),',','.');
            size_t end = 0; double value = std::stod(number,&end);
            if (end != number.size()) throw std::invalid_argument("Number");
            SetValue(model,activeField_,value);
            activeField_ = -1; error_.clear(); return true;
        } catch (const std::exception&) { error_ = "Введите число в указанном диапазоне"; return false; }
    }
    void ParametersPanel::SetViewport(int x,int y,int width,int height)
    {
        x_=x; y_=y; width_=width; height_=height;
        for(int i=0;i<4;++i) MoveWindow(editors_[i],x_+244,82+i*30,140,25,TRUE);
        MoveWindow(editors_[4],x_+72,216,312,32,TRUE);
        MoveWindow(shapePicker_,x_+16,42,368,180,TRUE);
        MoveWindow(applyButton_,x_+16,264,172,27,TRUE);
        MoveWindow(sideButton_,x_+195,264,189,27,TRUE);
    }
    bool ParametersPanel::Contains(int x,int y,int windowHeight) const
    { int localY=windowHeight-y-y_; return x>=x_ && x<x_+width_ && localY>=0 && localY<height_; }
    void ParametersPanel::Render(const ViewModels::MainViewModel& model)
    {
        SyncEditors();
        SetWindowTextW(sideButton_,model.TransparentPositive()?L"Прозрачная сторона: +":L"Прозрачная сторона: -");
        glViewport(x_,y_,width_,height_); glEnable(GL_SCISSOR_TEST); glScissor(x_,y_,width_,height_);
        glClearColor(0.12f,0.14f,0.18f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST); glDisable(GL_DEPTH_TEST); glUseProgram(0);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,width_,height_,0,-1,1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        auto text=[](int x,int y,const char* s) { glColor3f(.88f,.91f,.96f); DrawString(x,y,s); };
        text(16,26,"1. ВЫБЕРИТЕ ФИГУРУ");
        const char* labels[]={"Высота рожка","Радиус стыка","Форма рожка (степень)","Высота шапки",
            "Положение X","Положение Y","Положение Z","Поворот X, градусы","Поворот Y, градусы","Поворот Z, градусы"};
        if(shapeMode_==4) { labels[0]="X: от"; labels[1]="X: до"; labels[2]="Y: от"; labels[3]="Y: до"; }
        else if(shapeMode_!=0) { labels[0]=shapeMode_==3?"Радиус шара":"Полная высота фигуры"; labels[1]="Радиус фигуры"; }
        char buffer[160];
        for(int i=0;i<10;++i) {
            if(i<4 && shapeMode_!=0 && shapeMode_!=4 && (i>1 || (i==1 && shapeMode_==3))) continue;
            int top=FieldTop(i); text(16,top+18,labels[i]);
            if(i<4) continue;
            glColor3f(i==activeField_?.22f:.18f,.24f,.31f); Box(294,top,90,27);
            std::snprintf(buffer,sizeof(buffer),"%.3f",Value(model,i));
            text(300,top+18,i==activeField_?input_.c_str():buffer);
            if(i>=4) {
                const double low=i<7?-20:-180, high=i<7?20:180;
                float t=static_cast<float>((Value(model,i)-low)/(high-low));
                glColor3f(.27f,.32f,.40f); Box(20,top+32,364,3);
                glColor3f(.50f,.78f,.96f); Box(17+static_cast<int>(364*t),top+27,7,11);
            }
        }
        if(shapeMode_==0) {
            text(16,219,"Степень 1 - конус; 0.5 - округлый рожок.");
            text(16,238,"Измените размеры и нажмите Построить.");
        } else if(shapeMode_==4) {
            text(16,233,"z =");
            text(16,259,"X и Y - координаты основания; Z - высота.");
        } else {
            const char* descriptions[]={"", "Постоянный радиус по всей высоте.",
                "Радиус растёт от вершины к основанию.","Радиус - расстояние от центра до поверхности."};
            text(16,165,descriptions[shapeMode_]);
            if(shapeMode_==3) text(16,187,"Диаметр и высота шара равны двум радиусам.");
            else text(16,187,"Формула выбирается автоматически.");
            text(16,219,"Задайте размеры и нажмите Построить.");
            text(16,238,"Или задайте свою поверхность z=f(x,y).");
        }
        text(16,313,"2. НАСТРОЙТЕ СРЕЗ");
        const char* presets[]={"XY","XZ","YZ"};
        for(int i=0;i<3;++i) { glColor3f(.23f,.33f,.43f); Box(16+i*125,323,118,26); text(61+i*125,341,presets[i]); }
        text(16,617,"Положение: -20..20; углы: -180..180 градусов");
        std::snprintf(buffer,sizeof(buffer),"Объём: %.4f",model.Volume()); text(16,640,buffer);
        std::snprintf(buffer,sizeof(buffer),"Площадь среза: %.4f",model.SectionArea()); text(16,662,buffer);
        if(model.Section().IsEmpty()) text(16,684,"Плоскость не пересекает фигуру");
        if(!error_.empty()) { glColor3f(1,.55f,.4f); DrawString(16,706,error_.c_str()); }
        else text(16,706,"Размеры применяются кнопкой Построить или Enter.");
        if(shapeMode_==4) {
            text(16,729,"Примеры: x^2+y^2; sin(x)*cos(y); 2-x*y.");
            if(model.IsHeightField()) {
                std::snprintf(buffer,sizeof(buffer),"Основание объёма: Z = %.3f",model.SurfaceBaseZ());
                text(16,750,buffer);
            } else text(16,750,"Объём замкнут основанием ниже поверхности.");
        } else {
            text(16,729,"Готовые формы не требуют ввода формулы.");
            text(16,750,"Своя формула: поверхность z = f(x, y).");
        }
        if(shapeMode_==0) text(16,771,"Рожок: 0.5-8; радиус: 0.2-4; степень: 0.2-3.");
        else if(shapeMode_==3) text(16,771,"Радиус шара: от 0.35 до 4.");
        else if(shapeMode_==4) text(16,771,"Диапазоны X/Y: -10..10; от < до.");
        else text(16,771,"Высота: от 0.7 до 12; радиус: от 0.2 до 4.");
        text(16,792,"ЛКМ: вращать; ПКМ: двигать; колесо: масштаб");
        glEnable(GL_DEPTH_TEST);
    }
    void ParametersPanel::Drag(int x,ViewModels::MainViewModel& model)
    {
        const double t=std::clamp((x-x_-20)/364.0,0.0,1.0);
        const double low=dragging_<7?-20:-180, high=dragging_<7?20:180;
        const double value=std::round((low+t*(high-low))*10)/10;
        SetValue(model,dragging_,value);
    }
    bool ParametersPanel::Mouse(bool down,int x,int y,int windowHeight,ViewModels::MainViewModel& model)
    {
        if(down) SetFocus(parent_);
        if(!down) { bool handled=dragging_>=0; dragging_=-1; return handled; }
        if(!Contains(x,y,windowHeight)) { return activeField_>=0 && !Commit(model); }
        if(!Commit(model)) return true;
        int localX=x-x_, localY=height_-(windowHeight-y-y_);
        if(localY>=323 && localY<=349 && localX>=16 && localX<384) {
            int preset=(localX-16)/125; model.SetSectionPreset(preset==0?2:preset==1?1:0); return true;
        }
        for(int i=4;i<10;++i) {
            int top=FieldTop(i);
            if(localX>=294 && localX<=384 && localY>=top && localY<=top+27) {
                activeField_=i; char value[64]; std::snprintf(value,sizeof(value),"%.3f",Value(model,i)); input_=value; replaceInput_=true; return true;
            }
            if(i>=4 && localX>=12 && localX<=390 && localY>=top+27 && localY<=top+38) {
                dragging_=i; Drag(x,model); return true;
            }
        }
        return true;
    }
    bool ParametersPanel::Motion(int x,ViewModels::MainViewModel& model)
    { if(dragging_<0) return false; Drag(x,model); return true; }
    bool ParametersPanel::Keyboard(unsigned char key,ViewModels::MainViewModel& model)
    {
        if(activeField_<0) return false;
        if(key==27) { activeField_=-1; error_.clear(); }
        else if(key==13 || key==9) {
            int next=4+(activeField_-4+1)%6;
            if(Commit(model) && key==9) { activeField_=next; char value[64]; std::snprintf(value,sizeof(value),"%.3f",Value(model,next)); input_=value; replaceInput_=true; }
        }
        else if(key==8 || key==127) { if(replaceInput_) input_.clear(); else if(!input_.empty()) input_.pop_back(); replaceInput_=false; }
        else if(key==1) input_.clear();
        else if(input_.size()<10 && ((key>='0' && key<='9') || key=='.' || key==',' || key=='-' || key=='+')) { if(replaceInput_) input_.clear(); replaceInput_=false; input_+=static_cast<char>(key); }
        return true;
    }
}
