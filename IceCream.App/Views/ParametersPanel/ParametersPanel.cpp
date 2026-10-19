#include "ParametersPanel.h"

#include <GL/glew.h>
#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace IceCream::App::Views
{
	namespace
	{
		GLuint textFont = 0;
	}

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

		if (!ok)
		{
			throw std::runtime_error("Cannot initialize panel font.");
		}

		textFont = font_;
		model_ = &model;
		onChanged_ = std::move(onChanged);
		parent_ = WindowFromDC(dc);
		nativeInstance_ = this;
		SetWindowLongPtrW(parent_, GWL_STYLE, GetWindowLongPtrW(parent_, GWL_STYLE) | WS_CLIPCHILDREN);
		parentProcedure_ = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(parent_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(ParentProcedure)));
		nativeFont_ = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
		                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

		for (int i = 0; i < 5; ++i)
		{
			DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOVSCROLL;
			style |= i == 4 ? ES_MULTILINE | WS_VSCROLL : ES_AUTOHSCROLL;
			editors_[i] = CreateWindowExW(0, L"EDIT", L"", style, 0, 0, 1, 1, parent_,
			                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(1000 + i)), GetModuleHandleW(nullptr), nullptr);

			if (!editors_[i])
			{
				throw std::runtime_error("Cannot create text editor");
			}

			SendMessageW(editors_[i], WM_SETFONT, reinterpret_cast<WPARAM>(nativeFont_), TRUE);
			SendMessageW(editors_[i], EM_SETLIMITTEXT, i == 4 ? 512 : 32, 0);
			auto previousEdit = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(editors_[i], GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(EditProcedure)));

			if (!editProcedure_)
			{
				editProcedure_ = previousEdit;
			}
		}

		applyButton_ = CreateWindowW(L"BUTTON", L"Построить / Enter", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		                             0, 0, 1, 1, parent_, reinterpret_cast<HMENU>(1006), GetModuleHandleW(nullptr), nullptr);
		sideButton_ = CreateWindowW(L"BUTTON", L"Прозрачная сторона: +", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		                            0, 0, 1, 1, parent_, reinterpret_cast<HMENU>(1007), GetModuleHandleW(nullptr), nullptr);

		for (auto button : {applyButton_, sideButton_})
		{
			SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(nativeFont_), TRUE);
		}

		shapePicker_ = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
		                             0, 0, 1, 1, parent_, reinterpret_cast<HMENU>(1008), GetModuleHandleW(nullptr), nullptr);
		SendMessageW(shapePicker_, WM_SETFONT, reinterpret_cast<WPARAM>(nativeFont_), TRUE);

		for (const wchar_t* item : {L"Мороженое", L"Цилиндр", L"Конус", L"Шар", L"Своя поверхность z=f(x,y)"})
		{
			SendMessageW(shapePicker_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
		}

		SyncEditors();

		InitializeLighting();
		UpdateLightingVisibility();
	}

	void ParametersPanel::Destroy()
	{
		if (parent_ && parentProcedure_)
		{
			SetWindowLongPtrW(parent_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(parentProcedure_));
		}

		for (auto edit : editors_)
		{
			if (edit)
			{
				DestroyWindow(edit);
			}
		}

		if (shapePicker_)
		{
			DestroyWindow(shapePicker_);
		}

		if (applyButton_)
		{
			DestroyWindow(applyButton_);
		}

		if (sideButton_)
		{
			DestroyWindow(sideButton_);
		}

		for (HWND editor : lightEditors_)
		{
			if (editor)
			{
				DestroyWindow(editor);
			}
		}

		for (HWND control : {tabButton_, lightPicker_, applyLightButton_})
		{
			if (control)
			{
				DestroyWindow(control);
			}
		}

		if (nativeFont_)
		{
			DeleteObject(nativeFont_);
		}

		nativeInstance_ = nullptr;

		if (font_)
		{
			glDeleteLists(font_, 1280);
		}

		font_ = textFont = 0;
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
		glVertex2i(x, y);
		glVertex2i(x + width, y);
		glVertex2i(x + width, y + height);
		glVertex2i(x, y + height);
		glEnd();
	}

	int ParametersPanel::FieldTop(int field)
	{
		return field < 4 ? 82 + field * 30 : 360 + (field - 4) * 40;
	}

	void ParametersPanel::SetViewport(int x, int y, int width, int height)
	{
		x_ = x;
		y_ = y;
		width_ = width;
		height_ = height;

		for (int i = 0; i < 4; ++i)
		{
			MoveWindow(editors_[i], x_ + 244, 82 + i * 30, 140, 25, TRUE);
		}

		MoveWindow(editors_[4], x_ + 72, 216, 312, 32, TRUE);
		MoveWindow(shapePicker_, x_ + 16, 42, 368, 180, TRUE);
		MoveWindow(applyButton_, x_ + 16, 264, 172, 27, TRUE);
		MoveWindow(sideButton_, x_ + 195, 264, 189, 27, TRUE);

		SetLightingViewport();
	}

	bool ParametersPanel::Contains(int x, int y, int windowHeight) const
	{
		int localY = windowHeight - y - y_;

		return x >= x_ && x < x_ + width_ && localY >= 0 && localY < height_;
	}

	void ParametersPanel::Render(const ViewModels::MainViewModel& model)
	{
		if (!lightingTab_)
		{
			SyncEditors();
		}
		SetWindowTextW(sideButton_, model.TransparentPositive() ? L"Прозрачная сторона: +" : L"Прозрачная сторона: -");
		glViewport(x_, y_, width_, height_);
		glEnable(GL_SCISSOR_TEST);
		glScissor(x_, y_, width_, height_);
		glClearColor(0.12f, 0.14f, 0.18f, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_DEPTH_TEST);
		glUseProgram(0);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width_, height_, 0, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		if (lightingTab_)
		{
			RenderLighting();
			glEnable(GL_DEPTH_TEST);
			return;
		}

		auto text = [](int x, int y, const char* s)
		{
			glColor3f(.88f, .91f, .96f);
			DrawString(x, y, s);
		};
		text(16, 26, "1. ВЫБЕРИТЕ ФИГУРУ");
		const char* labels[] = {"Высота рожка", "Радиус стыка", "Форма рожка (степень)", "Высота шапки",
		                        "Положение X", "Положение Y", "Положение Z", "Поворот X, градусы", "Поворот Y, градусы", "Поворот Z, градусы"};

		if (shapeMode_ == 4)
		{
			labels[0] = "X: от";
			labels[1] = "X: до";
			labels[2] = "Y: от";
			labels[3] = "Y: до";
		}
		else if (shapeMode_ != 0)
		{
			labels[0] = shapeMode_ == 3 ? "Радиус шара" : "Полная высота фигуры";
			labels[1] = "Радиус фигуры";
		}

		char buffer[160];

		for (int i = 0; i < 10; ++i)
		{
			if (i < 4 && shapeMode_ != 0 && shapeMode_ != 4 && (i > 1 || (i == 1 && shapeMode_ == 3)))
			{
				continue;
			}

			int top = FieldTop(i);
			text(16, top + 18, labels[i]);

			if (i < 4)
			{
				continue;
			}

			glColor3f(i == activeField_ ? .22f : .18f, .24f, .31f);
			Box(294, top, 90, 27);
			std::snprintf(buffer, sizeof(buffer), "%.3f", Value(model, i));
			text(300, top + 18, i == activeField_ ? input_.c_str() : buffer);

			if (i >= 4)
			{
				const double low = i < 7 ? -20 : -180, high = i < 7 ? 20 : 180;
				float t = static_cast<float>((Value(model, i) - low) / (high - low));
				glColor3f(.27f, .32f, .40f);
				Box(20, top + 32, 364, 3);
				glColor3f(.50f, .78f, .96f);
				Box(17 + static_cast<int>(364 * t), top + 27, 7, 11);
			}
		}

		if (shapeMode_ == 0)
		{
			text(16, 219, "Степень 1 - конус; 0.5 - округлый рожок.");
			text(16, 238, "Измените размеры и нажмите Построить.");
		}
		else if (shapeMode_ == 4)
		{
			text(16, 233, "z =");
			text(16, 259, "X и Y - координаты основания; Z - высота.");
		}
		else
		{
			const char* descriptions[] = {"", "Постоянный радиус по всей высоте.",
			                              "Радиус растёт от вершины к основанию.", "Радиус - расстояние от центра до поверхности."};
			text(16, 165, descriptions[shapeMode_]);

			if (shapeMode_ == 3)
			{
				text(16, 187, "Диаметр и высота шара равны двум радиусам.");
			}
			else
			{
				text(16, 187, "Формула выбирается автоматически.");
			}

			text(16, 219, "Задайте размеры и нажмите Построить.");
			text(16, 238, "Или задайте свою поверхность z=f(x,y).");
		}

		text(16, 313, "2. НАСТРОЙТЕ СРЕЗ");
		const char* presets[] = {"XY", "XZ", "YZ"};

		for (int i = 0; i < 3; ++i)
		{
			glColor3f(.23f, .33f, .43f);
			Box(16 + i * 125, 323, 118, 26);
			text(61 + i * 125, 341, presets[i]);
		}

		text(16, 617, "Положение: -20..20; углы: -180..180 градусов");
		std::snprintf(buffer, sizeof(buffer), "Объём: %.4f", model.Volume());
		text(16, 640, buffer);
		std::snprintf(buffer, sizeof(buffer), "Площадь среза: %.4f", model.SectionArea());
		text(16, 662, buffer);

		if (model.Section().IsEmpty())
		{
			text(16, 684, "Плоскость не пересекает фигуру");
		}

		if (!error_.empty())
		{
			glColor3f(1, .55f, .4f);
			DrawString(16, 706, error_.c_str());
		}
		else
		{
			text(16, 706, "Размеры применяются кнопкой Построить или Enter.");
		}

		if (shapeMode_ == 4)
		{
			text(16, 729, "Примеры: x^2+y^2; sin(x)*cos(y); 2-x*y.");

			if (model.IsHeightField())
			{
				std::snprintf(buffer, sizeof(buffer), "Основание объёма: Z = %.3f", model.SurfaceBaseZ());
				text(16, 750, buffer);
			}
			else
			{
				text(16, 750, "Объём замкнут основанием ниже поверхности.");
			}
		}
		else
		{
			text(16, 729, "Готовые формы не требуют ввода формулы.");
			text(16, 750, "Своя формула: поверхность z = f(x, y).");
		}

		if (shapeMode_ == 0)
		{
			text(16, 771, "Рожок: 0.5-8; радиус: 0.2-4; степень: 0.2-3.");
		}
		else if (shapeMode_ == 3)
		{
			text(16, 771, "Радиус шара: от 0.35 до 4.");
		}
		else if (shapeMode_ == 4)
		{
			text(16, 771, "Диапазоны X/Y: -10..10; от < до.");
		}
		else
		{
			text(16, 771, "Высота: от 0.7 до 12; радиус: от 0.2 до 4.");
		}

		text(16, 792, "ЛКМ: вращать; ПКМ: двигать; колесо: масштаб");
		glEnable(GL_DEPTH_TEST);
	}
}
