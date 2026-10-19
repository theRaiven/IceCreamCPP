#include "ParametersPanel.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace IceCream::App::Views
{
	bool ParametersPanel::Editing() const
	{
		if (lightingTab_)
		{
			return true;
		}

		HWND focus = GetFocus();

		return focus == shapePicker_ || activeField_ >= 0 || std::find(editors_.begin(), editors_.end(), focus) != editors_.end();
	}

	double ParametersPanel::Value(const ViewModels::MainViewModel& m, int field)
	{
		const auto& p = m.Parameters();
		const auto& o = m.SectionPosition();
		const auto& r = m.SectionRotation();
		const double values[] = {p.BaseHeight, p.JoinRadius, p.BaseExponent, p.CapHeight, o.X, o.Y, o.Z, r.X, r.Y, r.Z};

		return values[field];
	}

	void ParametersPanel::SetValue(ViewModels::MainViewModel& m, int field, double value)
	{
		if (!std::isfinite(value))
		{
			throw std::invalid_argument("Invalid number");
		}

		const double low[] = {0.5, 0.2, 0.2, 0.2};
		const double high[] = {8, 4, 3, 4};

		if (field < 4 && (value < low[field] || value > high[field]))
		{
			throw std::out_of_range("Range");
		}

		switch (field)
		{
		case 0:
			m.SetBaseHeight(value);
			break;
		case 1:
			m.SetJoinRadius(value);
			break;
		case 2:
			m.SetBaseExponent(value);
			break;
		case 3:
			m.SetCapHeight(value);
			break;
		default:
			if (field < 7)
			{
				m.SetSectionPosition(field - 4, value);
			}
			else
			{
				if (value < -180 || value > 180)
				{
					throw std::out_of_range("Angle");
				}

				m.SetSectionRotation(field - 7, value);
			}
		}
	}

	bool ParametersPanel::Commit(ViewModels::MainViewModel& model)
	{
		if (activeField_ < 0)
		{
			return true;
		}

		try
		{
			std::string number = input_;
			std::replace(number.begin(), number.end(), ',', '.');
			size_t end = 0;
			double value = std::stod(number, &end);

			if (end != number.size())
			{
				throw std::invalid_argument("Number");
			}

			SetValue(model, activeField_, value);
			activeField_ = -1;
			error_.clear();

			return true;
		}
		catch (const std::exception&)
		{
			error_ = "Введите число в указанном диапазоне";

			return false;
		}
	}

	void ParametersPanel::Drag(int x, ViewModels::MainViewModel& model)
	{
		const double t = std::clamp((x - x_ - 20) / 364.0, 0.0, 1.0);
		const double low = dragging_ < 7 ? -20 : -180, high = dragging_ < 7 ? 20 : 180;
		const double value = std::round((low + t * (high - low)) * 10) / 10;
		SetValue(model, dragging_, value);
	}

	bool ParametersPanel::Mouse(bool down, int x, int y, int windowHeight, ViewModels::MainViewModel& model)
	{
		if (lightingTab_)
		{
			if (down)
			{
				SetFocus(parent_);
			}

			return Contains(x, y, windowHeight);
		}

		if (down)
		{
			SetFocus(parent_);
		}

		if (!down)
		{
			bool handled = dragging_ >= 0;
			dragging_ = -1;

			return handled;
		}

		if (!Contains(x, y, windowHeight))
		{
			return activeField_ >= 0 && !Commit(model);
		}

		if (!Commit(model))
		{
			return true;
		}

		int localX = x - x_, localY = height_ - (windowHeight - y - y_);

		if (localY >= 323 && localY <= 349 && localX >= 16 && localX < 384)
		{
			int preset = (localX - 16) / 125;
			model.SetSectionPreset(preset == 0 ? 2 : preset == 1 ? 1
			                                                     : 0);

			return true;
		}

		for (int i = 4; i < 10; ++i)
		{
			int top = FieldTop(i);

			if (localX >= 294 && localX <= 384 && localY >= top && localY <= top + 27)
			{
				activeField_ = i;
				char value[64];
				std::snprintf(value, sizeof(value), "%.3f", Value(model, i));
				input_ = value;
				replaceInput_ = true;

				return true;
			}

			if (i >= 4 && localX >= 12 && localX <= 390 && localY >= top + 27 && localY <= top + 38)
			{
				dragging_ = i;
				Drag(x, model);

				return true;
			}
		}

		return true;
	}

	bool ParametersPanel::Motion(int x, ViewModels::MainViewModel& model)
	{
		if (dragging_ < 0)
		{
			return false;
		}

		Drag(x, model);

		return true;
	}

	bool ParametersPanel::Keyboard(unsigned char key, ViewModels::MainViewModel& model)
	{
		if (lightingTab_)
		{
			return true;
		}

		if (activeField_ < 0)
		{
			return false;
		}

		if (key == 27)
		{
			activeField_ = -1;
			error_.clear();
		}
		else if (key == 13 || key == 9)
		{
			int next = 4 + (activeField_ - 4 + 1) % 6;

			if (Commit(model) && key == 9)
			{
				activeField_ = next;
				char value[64];
				std::snprintf(value, sizeof(value), "%.3f", Value(model, next));
				input_ = value;
				replaceInput_ = true;
			}
		}
		else if (key == 8 || key == 127)
		{
			if (replaceInput_)
			{
				input_.clear();
			}
			else if (!input_.empty())
			{
				input_.pop_back();
			}

			replaceInput_ = false;
		}
		else if (key == 1)
		{
			input_.clear();
		}
		else if (input_.size() < 10 && ((key >= '0' && key <= '9') || key == '.' || key == ',' || key == '-' || key == '+'))
		{
			if (replaceInput_)
			{
				input_.clear();
			}

			replaceInput_ = false;
			input_ += static_cast<char>(key);
		}

		return true;
	}
}
