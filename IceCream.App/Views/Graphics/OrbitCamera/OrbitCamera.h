#pragma once

#include <glm/glm.hpp>

namespace IceCream::App::Views::Graphics
{
	/// <summary>
	/// Орбитальная камера с перемещаемым центром наблюдения. Ось Z направлена вверх.
	/// </summary>
	class OrbitCamera
	{
	private:
		float yaw_ = -58.0f;
		float pitch_ = 23.0f;
		float distance_ = 10.0f;
		enum class DragMode
		{
			None,
			Rotate,
			Pan
		};
		DragMode dragMode_ = DragMode::None;
		glm::vec3 target_{0.0f};
		int lastMouseX_ = 0;
		int lastMouseY_ = 0;

	public:
		float Yaw() const
		{
			return yaw_;
		}

		float Pitch() const
		{
			return pitch_;
		}

		float Distance() const
		{
			return distance_;
		}

		const glm::vec3& Target() const
		{
			return target_;
		}

		glm::vec3 Position() const;
		glm::mat4 ViewMatrix() const;

		void BeginRotate(int x, int y);
		void EndRotate();
		void Rotate(int x, int y);
		void BeginPan(int x, int y);
		void EndPan();
		void Pan(int x, int y, int viewportHeight);
		void Zoom(int direction);
		void Reset();
	};
}
