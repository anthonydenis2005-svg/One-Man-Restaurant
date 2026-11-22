#pragma once

struct GLFWwindow;

void onMouseButton(GLFWwindow* window, int button, int action, int mods);
void onCursorMove(GLFWwindow* window, double mouseX, double mouseY);
void onFramebufferResize(GLFWwindow* window, int width, int height);
