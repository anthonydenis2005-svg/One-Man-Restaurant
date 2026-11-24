#include "UIController.h"

#include "Camera.h"
#include "Game.h"
#include "GameState.h"
#include "Renderer.h"
#include "SecurityCamera.h"

#include <GLFW/glfw3.h>

namespace {

void restartShift(UIController& controller, GameState& game, CameraState& camera) {
    resetGame(game);
    resetCameraDirection(camera);
    resetScreenUIPan(controller.screenUI);
    hideText();
    camera.locked = false;
    camera.rotating = false;
    camera.clickPending = false;
    camera.closeClipboardPending = false;
}

void applyMenuAction(UIController& controller, GameState& game,
                     CameraState& camera, GLFWwindow* window,
                     ScreenUIAction action) {
    switch (action) {
        case ScreenUIAction::Resume:
            game.isPaused = false;
            break;
        case ScreenUIAction::Restart:
            restartShift(controller, game, camera);
            break;
        case ScreenUIAction::Quit:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case ScreenUIAction::PanRight:
            startScreenUIPan(controller.screenUI, camera, true);
            break;
        case ScreenUIAction::PanCenter:
            startScreenUIPan(controller.screenUI, camera, false);
            break;
        case ScreenUIAction::None:
            break;
    }
}

}

bool initializeUIController(UIController& controller) {
    return initializeScreenUI(controller.screenUI);
}

void destroyUIController(UIController& controller) {
    destroyScreenUI(controller.screenUI);
}

void prepareUIInput(UIController& controller, const GameState& game, CameraState& camera) {
    camera.locked = screenUIHasMenu(game) ||
                    game.clipboardViewState != ClipboardViewState::OnDesk ||
                    screenUIPanActive(controller.screenUI);
    if (camera.locked) camera.rotating = false;
}

void processUIInput(UIController& controller, GameState& game, CameraState& camera,
                    SecurityCamera&, GLFWwindow* window) {
    const bool pressedP = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
    const bool pressedEscape = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    const bool pressedR = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
    const bool pressedEnter = glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS;

    if (pressedEscape) glfwSetWindowShouldClose(window, GLFW_FALSE);

    if (((pressedP && !controller.priorP) ||
         (pressedEscape && !controller.priorEscape)) &&
        !game.gameOver && !game.shiftCompleted) {
        game.isPaused = !game.isPaused;
        camera.rotating = false;
    }
    if (game.isPaused && pressedEnter && !controller.priorEnter)
        game.isPaused = false;
    if ((game.gameOver || game.shiftCompleted) && pressedR && !controller.priorR)
        restartShift(controller, game, camera);

    controller.priorP = pressedP;
    controller.priorEscape = pressedEscape;
    controller.priorR = pressedR;
    controller.priorEnter = pressedEnter;

    if (camera.clickPending) {
        int windowWidth = 0, windowHeight = 0;
        int framebufferWidth = 0, framebufferHeight = 0;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        const ScreenUIAction action = hitTestScreenUI(
            controller.screenUI, game, camera.clickX, camera.clickY,
            windowWidth, windowHeight, framebufferWidth, framebufferHeight);
        if (action != ScreenUIAction::None) {
            camera.clickPending = false;
            applyMenuAction(controller, game, camera, window, action);
        } else if (screenUIHasMenu(game) || screenUIPanActive(controller.screenUI)) {
            camera.clickPending = false;
        }
    }
    if (screenUIHasMenu(game)) camera.closeClipboardPending = false;
}

void updateUIController(UIController& controller, const GameState& game,
                        CameraState& camera, float deltaTime) {
    if (!screenUIHasMenu(game) && game.clipboardViewState == ClipboardViewState::OnDesk)
        updateScreenUIPan(controller.screenUI, camera, deltaTime);
    prepareUIInput(controller, game, camera);
}

void renderUIController(UIController& controller, const GameState& game,
                        GLFWwindow* window, int framebufferWidth, int framebufferHeight) {
    int windowWidth = 0, windowHeight = 0;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);
    double mouseX = 0.0, mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    renderScreenUI(controller.screenUI, game, framebufferWidth, framebufferHeight,
                   mouseX, mouseY, windowWidth, windowHeight);
}
