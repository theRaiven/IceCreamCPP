#include "ParametersPanel.h"
#include <GL/glew.h>
#include <cstdio>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace IceCream::App::Views
{
	void ParametersPanel::InitializeLighting()
	{
		HINSTANCE instance = GetModuleHandleW(nullptr);

		// Кнопка переключения вкладок видна постоянно.
		tabButton_ = CreateWindowW(
			L"BUTTON", L"Свет",
			WS_CHILD | WS_VISIBLE | WS_TABSTOP,
			0, 0, 1, 1,
			parent_, reinterpret_cast<HMENU>(1100), instance, nullptr);

		// Элементы вкладки света изначально скрыты.
		lightPicker_ = CreateWindowW(
			L"COMBOBOX", L"",
			WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
			0, 0, 1, 1,
			parent_, reinterpret_cast<HMENU>(1101), instance, nullptr);

		applyLightButton_ = CreateWindowW(
			L"BUTTON", L"Применить свет",
			WS_CHILD | WS_TABSTOP,
			0, 0, 1, 1,
			parent_, reinterpret_cast<HMENU>(1102), instance, nullptr);

		if (!tabButton_ || !lightPicker_ || !applyLightButton_)
		{
			throw std::runtime_error("Не удалось создать элементы управления светом.");
		}

		for (HWND control : {tabButton_, lightPicker_, applyLightButton_})
		{
			SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(nativeFont_), TRUE);
		}

		const wchar_t* types[] =
		{
			L"Направленный",
			L"Точечный",
			L"Прожектор"
		};

		for (const wchar_t* type : types)
		{
			SendMessageW(lightPicker_, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(type));
		}

		for (int i = 0; i < 11; i++)
		{
			lightEditors_[i] = CreateWindowExW(
				0, L"EDIT", L"",
				WS_CHILD | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
				0, 0, 1, 1,
				parent_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(1200 + i)),
				instance, nullptr);

			if (!lightEditors_[i])
			{
				throw std::runtime_error("Не удалось создать поле параметра света.");
			}

			SendMessageW(lightEditors_[i], WM_SETFONT, reinterpret_cast<WPARAM>(nativeFont_), TRUE);
			SendMessageW(lightEditors_[i], EM_SETLIMITTEXT, 32, 0);
		}

		SyncLightingEditors();
	}

	void ParametersPanel::SetLightingViewport()
	{
		MoveWindow(tabButton_, x_ + 280, 8, 104, 27, TRUE);
		MoveWindow(lightPicker_, x_ + 16, 42, 368, 160, TRUE);

		for (int i = 0; i < 11; i++)
		{
			int top = 90 + i * 38;

			MoveWindow(lightEditors_[i], x_ + 244, top, 140, 25, TRUE);
		}

		MoveWindow(applyLightButton_, x_ + 16, 530, 368, 30, TRUE);
	}

	void ParametersPanel::SyncLightingEditors()
	{
		const auto& lighting = model_->Lighting();

		SendMessageW(lightPicker_, CB_SETCURSEL, static_cast<WPARAM>(lighting.Type), 0);

		const float values[] =
		{
			lighting.AmbientIntensity,
			lighting.EmissionIntensity,
			lighting.LightIntensity,
			lighting.Position.x,
			lighting.Position.y,
			lighting.Position.z,
			lighting.Direction.x,
			lighting.Direction.y,
			lighting.Direction.z,
			lighting.SpotAngle,
			lighting.SpotExponent
		};

		for (int i = 0; i < 11; i++)
		{
			char buffer[64];
			std::snprintf(buffer, sizeof(buffer), "%.6g", values[i]);

			SetWindowTextA(lightEditors_[i], buffer);
		}
	}
	void ParametersPanel::SwitchTab()
	{
		// Перед уходом с вкладки фигуры сохраняем ввод параметра сечения.
		if (!lightingTab_ && !Commit(*model_))
		{
			onChanged_();
			return;
		}

		lightingTab_ = !lightingTab_;
		activeField_ = -1;
		dragging_ = -1;
		error_.clear();

		SetWindowTextW(tabButton_, lightingTab_ ? L"Фигура" : L"Свет");

		UpdateLightingVisibility();
		SetFocus(parent_);
		onChanged_();
	}

	void ParametersPanel::UpdateLightingVisibility()
	{
		// Элементы вкладки фигуры.
		ShowWindow(shapePicker_, lightingTab_ ? SW_HIDE : SW_SHOW);
		ShowWindow(applyButton_, lightingTab_ ? SW_HIDE : SW_SHOW);
		ShowWindow(sideButton_, lightingTab_ ? SW_HIDE : SW_SHOW);

		for (int i = 0; i < 5; i++)
		{
			bool visible = false;

			if (!lightingTab_)
			{
				if (i == 4)
				{
					visible = shapeMode_ == 4;
				}
				else
				{
					visible = shapeMode_ == 0 || shapeMode_ == 4 ||
						i == 0 || (i == 1 && shapeMode_ != 3);
				}
			}

			ShowWindow(editors_[i], visible ? SW_SHOW : SW_HIDE);
		}

		// Элементы вкладки света.
		ShowWindow(lightPicker_, lightingTab_ ? SW_SHOW : SW_HIDE);
		ShowWindow(applyLightButton_, lightingTab_ ? SW_SHOW : SW_HIDE);

		int type = static_cast<int>(SendMessageW(lightPicker_, CB_GETCURSEL, 0, 0));

		for (int i = 0; i < 11; i++)
		{
			bool available = true;

			if (i >= 3 && i <= 5)
			{
				// Положение нужно точечному источнику и прожектору.
				available = type == 1 || type == 2;
			}
			else if (i >= 6 && i <= 8)
			{
				// Направление нужно направленному источнику и прожектору.
				available = type == 0 || type == 2;
			}
			else if (i >= 9)
			{
				available = type == 2;
			}

			ShowWindow(lightEditors_[i], lightingTab_ && available ? SW_SHOW : SW_HIDE);
		}
	}

	void ParametersPanel::RenderLighting()
	{
		const char* labels[] =
		{
			"Фоновая интенсивность",
			"Собственная яркость",
			"Интенсивность источника",
			"Положение X",
			"Положение Y",
			"Положение Z",
			"Направление X",
			"Направление Y",
			"Направление Z",
			"Угол прожектора, градусы",
			"Концентрация пучка"
		};

		glColor3f(0.88f, 0.91f, 0.96f);
		DrawString(16, 26, "НАСТРОЙКИ СВЕТА");

		for (int i = 0; i < 11; i++)
		{
			if (IsWindowVisible(lightEditors_[i]))
			{
				int top = 90 + i * 38;

				DrawString(16, top + 18, labels[i]);
			}
		}

		DrawString(16, 590, "Направление: от источника к сцене.");
		DrawString(16, 612, "Положение: в координатах фигуры.");
		DrawString(16, 634, "Угол: от оси до границы конуса.");
		DrawString(16, 656, "Больше концентрация — уже яркая область.");

		if (!error_.empty())
		{
			glColor3f(1.0f, 0.55f, 0.4f);
			DrawString(16, 706, error_.c_str());
		}
		else
		{
			DrawString(16, 706, "Измените параметры и нажмите Применить.");
		}
	}
	void ParametersPanel::ApplyLightingEditors()
	{
		try
		{
			auto lighting = model_->Lighting();

			// Читаем число, разрешая точку или запятую.
			auto read = [this](int index)
				{
					char buffer[64]{};
					GetWindowTextA(lightEditors_[index], buffer, sizeof(buffer));

					std::string text = buffer;
					std::replace(text.begin(), text.end(), ',', '.');

					size_t end = 0;
					float value = std::stof(text, &end);

					while (end < text.size() && std::isspace(static_cast<unsigned char>(text[end])))
					{
						end++;
					}

					if (end != text.size() || !std::isfinite(value))
					{
						throw std::invalid_argument("Некорректное число.");
					}

					return value;
				};

			int type = static_cast<int>(SendMessageW(lightPicker_, CB_GETCURSEL, 0, 0));

			if (type < 0 || type > 2)
			{
				throw std::invalid_argument("Не выбран тип источника.");
			}

			lighting.Type = static_cast<Graphics::LightType>(type);

			lighting.AmbientIntensity = read(0);
			lighting.EmissionIntensity = read(1);
			lighting.LightIntensity = read(2);

			if (lighting.AmbientIntensity < 0.0f || lighting.AmbientIntensity > 1.0f ||
				lighting.EmissionIntensity < 0.0f || lighting.EmissionIntensity > 1.0f ||
				lighting.LightIntensity < 0.0f || lighting.LightIntensity > 5.0f)
			{
				error_ = "Фон и свечение: 0–1; источник: 0–5.";
				onChanged_();
				return;
			}

			// Читаем только параметры, доступные выбранному источнику.
			if (lighting.Type != Graphics::LightType::Directional)
			{
				lighting.Position = glm::vec3(read(3), read(4), read(5));

				if (std::abs(lighting.Position.x) > 100.0f ||
					std::abs(lighting.Position.y) > 100.0f ||
					std::abs(lighting.Position.z) > 100.0f)
				{
					error_ = "Координаты источника: от -100 до 100.";
					onChanged_();
					return;
				}
			}

			if (lighting.Type != Graphics::LightType::Point)
			{
				lighting.Direction = glm::vec3(read(6), read(7), read(8));

				if (std::abs(lighting.Direction.x) > 100.0f ||
					std::abs(lighting.Direction.y) > 100.0f ||
					std::abs(lighting.Direction.z) > 100.0f)
				{
					error_ = "Компоненты направления: от -100 до 100.";
					onChanged_();
					return;
				}

				if (glm::length(lighting.Direction) < 0.000001f)
				{
					error_ = "Направление не должно быть нулевым.";
					onChanged_();
					return;
				}
			}

			if (lighting.Type == Graphics::LightType::Spot)
			{
				lighting.SpotAngle = read(9);
				lighting.SpotExponent = read(10);

				if (lighting.SpotAngle < 0.1f || lighting.SpotAngle > 89.9f)
				{
					error_ = "Угол прожектора: от 0.1 до 89.9 градуса.";
					onChanged_();
					return;
				}

				if (lighting.SpotExponent < 0.0f || lighting.SpotExponent > 128.0f)
				{
					error_ = "Концентрация пучка: от 0 до 128.";
					onChanged_();
					return;
				}
			}

			// Применяем настройки только после всех проверок.
			model_->SetLighting(lighting);

			error_.clear();
			SetFocus(parent_);
		}
		catch (const std::exception&)
		{
			error_ = "Введите корректные числа в полях света.";
		}

		onChanged_();
	}
}