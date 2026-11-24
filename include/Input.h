#pragma once

struct GLFWwindow;

// GLFW calls these functions for mouse input and window resizing.
void onMouseButton(GLFWwindow* window, int button, int action, int mods);
void onCursorMove(GLFWwindow* window, double mouseX, double mouseY);
void onFramebufferResize(GLFWwindow* window, int width, int height);
