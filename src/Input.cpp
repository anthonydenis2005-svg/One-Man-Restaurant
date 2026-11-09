#include "Input.h"
#include "Camera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <algorithm>

// These callbacks record input; main.cpp decides which 3D object was clicked.
void onMouseButton(GLFWwindow* window, int button, int action, int) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (!camera) return;

    // Save the cursor position so main.cpp can check bounding boxes after the event.
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        camera->clickPending = true;
        glfwGetCursorPos(window, &camera->clickX, &camera->clickY);
    }
    // Right click requests that the clipboard close.
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        camera->closeClipboardPending = true;
    }
    // Middle mouse drag rotates the camera unless the clipboard has locked it.
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

// Update yaw and pitch while dragging, and prevent the camera from flipping over.
void onCursorMove(GLFWwindow* window, double mouseX, double mouseY) {
    auto* camera = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
    if (!camera || !camera->rotating || camera->locked) return;

    constexpr float sensitivity = 0.15f;
    const float changeX = static_cast<float>(mouseX - camera->lastMouseX);
    const float changeY = static_cast<float>(mouseY - camera->lastMouseY);
    camera->lastMouseX = mouseX;
    camera->lastMouseY = mouseY;
    camera->yaw += changeX * sensitivity;
    camera->pitch = std::clamp(
        camera->pitch - changeY * sensitivity,
        -89.0f, 89.0f
    );
}

// Keep the OpenGL drawing area matched to the resized window.
void onFramebufferResize(GLFWwindow*, int width, int height) {
    glViewport(0, 0, width, height);
}
