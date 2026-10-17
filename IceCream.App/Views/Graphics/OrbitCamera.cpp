#include "OrbitCamera.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace IceCream::App::Views::Graphics
{
    glm::vec3 OrbitCamera::Position() const
    {
        const float yaw = glm::radians(yaw_);
        const float pitch = glm::radians(pitch_);

        return target_ + glm::vec3(
            distance_ * glm::cos(pitch) * glm::cos(yaw),
            distance_ * glm::cos(pitch) * glm::sin(yaw),
            distance_ * glm::sin(pitch));
    }

    glm::mat4 OrbitCamera::ViewMatrix() const
    {
        return glm::lookAt(
            Position(),
            target_,
            glm::vec3(0.0f, 0.0f, 1.0f));
    }

    void OrbitCamera::BeginRotate(int x, int y)
    {
        dragMode_ = DragMode::Rotate;
        lastMouseX_ = x;
        lastMouseY_ = y;
    }

    void OrbitCamera::EndRotate()
    {
        if (dragMode_ == DragMode::Rotate) dragMode_ = DragMode::None;
    }

    void OrbitCamera::Rotate(int x, int y)
    {
        if (dragMode_ != DragMode::Rotate)
        {
            return;
        }

        constexpr float Sensitivity = 0.4f;

        yaw_ -= static_cast<float>(x - lastMouseX_) * Sensitivity;
        pitch_ += static_cast<float>(y - lastMouseY_) * Sensitivity;
        pitch_ = std::clamp(pitch_, -89.0f, 89.0f);

        lastMouseX_ = x;
        lastMouseY_ = y;
    }

    void OrbitCamera::BeginPan(int x, int y)
    {
        dragMode_ = DragMode::Pan;
        lastMouseX_ = x;
        lastMouseY_ = y;
    }

    void OrbitCamera::EndPan()
    {
        if (dragMode_ == DragMode::Pan) dragMode_ = DragMode::None;
    }

    void OrbitCamera::Pan(int x, int y, int viewportHeight)
    {
        if (dragMode_ != DragMode::Pan) return;
        const glm::vec3 forward = glm::normalize(target_ - Position());
        const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0,0,1)));
        const glm::vec3 up = glm::cross(right, forward);
        // Perspective field of view is 45 degrees. Keep screen-space drag speed
        // consistent with the viewport and zoom, including after camera rotation.
        const float unitsPerPixel = 2.0f * distance_ * glm::tan(glm::radians(22.5f))
            / static_cast<float>(std::max(viewportHeight, 1));
        target_ += (-right * static_cast<float>(x-lastMouseX_)
            + up * static_cast<float>(y-lastMouseY_)) * unitsPerPixel;
        lastMouseX_ = x;
        lastMouseY_ = y;
    }

    void OrbitCamera::Zoom(int direction)
    {
        distance_ *= (direction > 0) ? 0.9f : 1.1f;
        distance_ = std::clamp(distance_, 3.0f, 40.0f);
    }

    void OrbitCamera::Reset()
    {
        yaw_ = -58.0f;
        pitch_ = 23.0f;
        distance_ = 10.0f;
        target_ = glm::vec3(0);
        dragMode_ = DragMode::None;
    }
}
