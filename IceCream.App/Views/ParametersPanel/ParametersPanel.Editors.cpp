#include "ParametersPanel.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace IceCream::App::Views
{
	const char* ParametersPanel::ShapeFormula(int mode)
	{
		switch (mode)
		{
		case 0:
			return Core::Profiles::FormulaProfile::DefaultFormula();
		case 1:
			return "radius";
		case 2:
			return "radius*z/height";
		case 3:
			return "sqrt(max(0,radius^2-(z-radius)^2))";
		default:
			return "x^2 + y^2";
		}
	}

	int ParametersPanel::DetectShape(const std::string& formula)
	{
		for (int mode = 0; mode < 4; ++mode)
		{
			if (formula == ShapeFormula(mode))
			{
				return mode;
			}
		}

		return 4;
	}

	void ParametersPanel::SelectShape(int mode)
	{
		if (mode < 0 || mode > 4)
		{
			return;
		}

		if (shapeMode_ == 4)
		{
			char text[513]{};
			GetWindowTextA(editors_[4], text, sizeof(text));
			customDraft_ = text;
		}

		shapeMode_ = mode;
		error_.clear();
		ConfigureShapeEditors();
	}

	void ParametersPanel::ConfigureShapeEditors()
	{
		SendMessageW(shapePicker_, CB_SETCURSEL, shapeMode_, 0);
		const auto& p = model_->Parameters();
		const auto& domain = model_->SurfaceBounds();
		double values[] = {shapeMode_ == 0 ? p.BaseHeight : shapeMode_ == 3 ? p.JoinRadius
		                                                                    : p.BaseHeight + p.CapHeight,
		                   p.JoinRadius, p.BaseExponent, p.CapHeight};

		if (shapeMode_ == 4)
		{
			values[0] = domain.MinX;
			values[1] = domain.MaxX;
			values[2] = domain.MinY;
			values[3] = domain.MaxY;
		}

		for (int i = 0; i < 4; ++i)
		{
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.6g", values[i]);
			SetWindowTextA(editors_[i], buffer);
			bool visible = shapeMode_ == 4 || shapeMode_ == 0 || i == 0 || (i == 1 && shapeMode_ != 3);
			ShowWindow(editors_[i], visible ? SW_SHOW : SW_HIDE);
		}

		ShowWindow(editors_[4], shapeMode_ == 4 ? SW_SHOW : SW_HIDE);

		if (shapeMode_ == 4)
		{
			SetWindowTextA(editors_[4], customDraft_.c_str());
		}
	}

	void ParametersPanel::SyncEditors()
	{
		if (!model_ || nativeRevision_ == model_->GeometryRevision())
		{
			return;
		}

		if (!applyingShape_ && model_->Formula() != lastFormula_)
		{
			shapeMode_ = model_->IsHeightField() ? 4 : DetectShape(model_->Formula());
		}

		if (shapeMode_ == 4 && model_->IsHeightField())
		{
			customDraft_ = model_->Formula();
		}

		lastFormula_ = model_->Formula();
		ConfigureShapeEditors();
		nativeRevision_ = model_->GeometryRevision();
	}

	void ParametersPanel::ApplyEditors()
	{
		try
		{
			auto parameters = model_->Parameters();
			auto read = [this](int index)
			{
				char buffer[64]{};
				GetWindowTextA(editors_[index], buffer, sizeof(buffer));
				std::string text = buffer;
				std::replace(text.begin(), text.end(), ',', '.');
				size_t end = 0;
				double value = std::stod(text, &end);

				while (end < text.size() && std::isspace(static_cast<unsigned char>(text[end])))
				{
					++end;
				}

				if (end != text.size() || !std::isfinite(value))
				{
					throw std::invalid_argument("Invalid number");
				}

				return value;
			};

			if (shapeMode_ == 4)
			{
				char text[513]{};
				GetWindowTextA(editors_[4], text, sizeof(text));
				model_->ApplyHeightField({read(0), read(1), read(2), read(3)}, text);
				applyingShape_ = true;
				SyncEditors();
				applyingShape_ = false;
				error_.clear();
				activeField_ = -1;
				SetFocus(parent_);
				onChanged_();

				return;
			}

			if (shapeMode_ == 0)
			{
				parameters = {read(0), read(1), read(2), read(3)};
			}
			else
			{
				double height = read(0);

				if (shapeMode_ == 3)
				{
					parameters.JoinRadius = height;
					height *= 2;
				}
				else
				{
					parameters.JoinRadius = read(1);
				}

				if (height < 0.7 || height > 12)
				{
					throw std::out_of_range("Height must be between 0.7 and 12");
				}
				// Разделяем общую высоту на высоты основания и шапки.
				parameters.BaseHeight = std::clamp(height - .2, .5, 8.0);
				parameters.CapHeight = std::max(.2, height - parameters.BaseHeight);
			}

			std::string formula = ShapeFormula(shapeMode_);

			if (shapeMode_ == 4)
			{
				char text[513]{};
				GetWindowTextA(editors_[4], text, sizeof(text));
				formula = text;
			}

			model_->ApplyShape(parameters, formula);
			applyingShape_ = true;
			SyncEditors();
			applyingShape_ = false;
			error_.clear();
			activeField_ = -1;
			SetFocus(parent_);
		}
		catch (const std::exception& e)
		{
			applyingShape_ = false;
			error_ = shapeMode_ == 4 ? "Не удалось построить: проверьте формулу и размеры" : "Проверьте размеры: значение вне диапазона";
			std::cerr << e.what() << '\n';
		}

		onChanged_();
	}

	LRESULT CALLBACK ParametersPanel::ParentProcedure(HWND hwnd, UINT message, WPARAM w, LPARAM l)
	{
		auto* self = nativeInstance_;

		if (!self)
		{
			return DefWindowProcW(hwnd, message, w, l);
		}
		if (message == WM_COMMAND)
		{
			int id = LOWORD(w);
			int notification = HIWORD(w);

			if (id == 1100 && notification == BN_CLICKED)
			{
				self->SwitchTab();
				return 0;
			}

			if (id == 1101 && notification == CBN_SELCHANGE)
			{
				self->error_.clear();
				self->UpdateLightingVisibility();
				self->onChanged_();
				return 0;
			}

			if (id == 1102 && notification == BN_CLICKED)
			{
				self->ApplyLightingEditors();
				return 0;
			}
		}
		if (message == WM_COMMAND && LOWORD(w) == 1008 && HIWORD(w) == CBN_SELCHANGE)
		{
			self->SelectShape(static_cast<int>(SendMessageW(self->shapePicker_, CB_GETCURSEL, 0, 0)));
			self->onChanged_();

			return 0;
		}

		if (message == WM_COMMAND && HIWORD(w) == EN_SETFOCUS && LOWORD(w) >= 1000 && LOWORD(w) <= 1004)
		{
			self->activeField_ = -1;
		}

		if (message == WM_COMMAND && HIWORD(w) == BN_CLICKED)
		{
			if (LOWORD(w) == 1006)
			{
				self->ApplyEditors();

				return 0;
			}

			if (LOWORD(w) == 1007)
			{
				self->model_->ToggleTransparentSide();
				SetWindowTextW(self->sideButton_, self->model_->TransparentPositive() ? L"Прозрачная сторона: +" : L"Прозрачная сторона: -");
				SetFocus(self->parent_);
				self->onChanged_();

				return 0;
			}
		}

		return CallWindowProcW(self->parentProcedure_, hwnd, message, w, l);
	}

	LRESULT CALLBACK ParametersPanel::EditProcedure(HWND hwnd, UINT message, WPARAM w, LPARAM l)
	{
		auto* self = nativeInstance_;

		if (!self)
		{
			return DefWindowProcW(hwnd, message, w, l);
		}

		if (message == WM_KEYDOWN)
		{
			if (w == VK_RETURN)
			{
				self->ApplyEditors();

				return 0;
			}

			if (w == VK_ESCAPE)
			{
				self->shapeMode_ = self->model_->IsHeightField() ? 4 : DetectShape(self->model_->Formula());
				self->nativeRevision_ = static_cast<std::uint64_t>(-1);
				self->SyncEditors();
				self->error_.clear();
				SetFocus(self->parent_);
				self->onChanged_();

				return 0;
			}

			if (w == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				SendMessageW(hwnd, EM_SETSEL, 0, -1);

				return 0;
			}

			if (w == VK_TAB)
			{
				auto found = std::find(self->editors_.begin(), self->editors_.end(), hwnd);
				int index = static_cast<int>(found - self->editors_.begin());

				do
				{
					index = (index + ((GetKeyState(VK_SHIFT) & 0x8000) ? 4 : 1)) % 5;
				} while (!IsWindowVisible(self->editors_[index]));

				SetFocus(self->editors_[index]);
				SendMessageW(self->editors_[index], EM_SETSEL, 0, -1);

				return 0;
			}
		}

		if (message == WM_CHAR && (w == VK_RETURN || w == VK_TAB || w == VK_ESCAPE))
		{
			return 0;
		}

		return CallWindowProcW(self->editProcedure_, hwnd, message, w, l);
	}
}
