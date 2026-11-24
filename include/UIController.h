#pragma once

#include "ScreenUI.h"

struct GLFWwindow;
struct GameState;
struct CameraState;
class SecurityCamera;

struct UIController {
    ScreenUI screenUI;
    bool priorP = false;
    bool priorEscape = false;
    bool priorR = false;
    bool priorEnter = false;
};

bool initializeUIController(UIController& controller);
void destroyUIController(UIController& controller);
void prepareUIInput(UIController& controller, const GameState& game, CameraState& camera);
void processUIInput(UIController& controller, GameState& game, CameraState& camera,
                    SecurityCamera& securityCamera, GLFWwindow* window);
void updateUIController(UIController& controller, const GameState& game,
                        CameraState& camera, float deltaTime);
void renderUIController(UIController& controller, const GameState& game,
                        GLFWwindow* window, int framebufferWidth, int framebufferHeight);
