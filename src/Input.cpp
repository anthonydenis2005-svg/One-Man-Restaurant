#include "Input.h"
#include "Camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

void onMouseButton(GLFWwindow* window, int button, int action, int) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (!camera) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        camera->clickPending = true;
        glfwGetCursorPos(window, &camera->clickX, &camera->clickY);
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        camera->closeClipboardPending = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        if (camera->locked) return;
        if (action == GLFW_PRESS) {
            camera->rotating = true;
            glfwGetCursorPos(window, &camera->lastMouseX, &camera->lastMouseY);
        } else if (action == GLFW_RELEASE) {
            camera->rotating = false;
        }
    }
}

void onCursorMove(GLFWwindow* window, double mouseX, double mouseY) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (!camera || !camera->rotating || camera->locked) return;

    constexpr float sensitivity = 0.15f;
    const float changeX = static_cast<float>(mouseX - camera->lastMouseX);
    const float changeY = static_cast<float>(mouseY - camera->lastMouseY);
    camera->lastMouseX = mouseX;
    camera->lastMouseY = mouseY;
    camera->yaw += changeX * sensitivity;
    camera->pitch = std::clamp(camera->pitch - changeY * sensitivity, -89.0f, 89.0f);
}

void onFramebufferResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
