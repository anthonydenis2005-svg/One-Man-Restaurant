#include "Camera.h"
#include "Game.h"
#include "GameState.h"
#include "Input.h"
#include "ModelLoader.h"
#include "Renderer.h"

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <limits>
#include <string>
#include <vector>

int main() {
    const auto modelPath = findModelPath();
    if (modelPath.empty()) {
        std::cerr << "No GLB model found. Place office.glb inside assets/models/.\n";
        return 1;
    }

    std::vector<float> vertices;
    std::vector<Part> parts;
    std::vector<TextureImage> images;
    std::vector<InteractiveObject> interactiveObjects;
    if (!loadOfficeModel(modelPath, vertices, parts, images, interactiveObjects)) return 1;

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Could not initialize GLFW.\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "One Man Restaurant - Office", nullptr, nullptr);
    if (!window) {
        std::cerr << "Could not create the OpenGL window.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Could not initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    CameraState camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursorMove);

    RendererState renderer;
    if (!initializeRenderer(renderer, vertices, images, modelPath)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "Middle-mouse drag to look around. Left-click the clipboard to open.\n"
              << "Click the FireAlarmBox or press E to clear alarms.\n"
              << "Press 1, 2, 3 for paperwork. P pauses, R restarts after the shift, ESC quits.\n";

    GameState gameState;
    double lastTime = glfwGetTime();
    bool pKeyWasPressed = false;
    bool rKeyWasPressed = false;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GLFW_TRUE);

        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        const bool pKeyPressed = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pKeyPressed && !pKeyWasPressed && !gameState.gameOver && !gameState.shiftCompleted) {
            gameState.isPaused = !gameState.isPaused;
            if (gameState.isPaused) {
                camera.rotating = false;
                camera.locked = true;
                std::cout << "[GAME] Paused.\n";
            } else {
                camera.locked = gameState.clipboardViewState != ClipboardViewState::OnDesk;
                std::cout << "[GAME] Resumed.\n";
            }
        }
        pKeyWasPressed = pKeyPressed;

        const bool rKeyPressed = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (rKeyPressed && !rKeyWasPressed && (gameState.gameOver || gameState.shiftCompleted)) {
            resetGame(gameState);
            resetCameraDirection(camera);
            camera.locked = false;
            camera.rotating = false;
            camera.clickPending = false;
            camera.closeClipboardPending = false;
        }
        rKeyWasPressed = rKeyPressed;

        const bool stopped = gameState.isPaused || gameState.gameOver || gameState.shiftCompleted;
        if (stopped) {
            camera.clickPending = false;
            camera.closeClipboardPending = false;
            if (gameState.gameOver || gameState.shiftCompleted) {
                camera.rotating = false;
                camera.locked = true;
            }
        } else {
            if (camera.clickPending) {
                camera.clickPending = false;
                int width = 0, height = 0;
                glfwGetWindowSize(window, &width, &height);
                if (width > 0 && height > 0) {
                    const Ray ray = makeMouseRay(camera, camera.clickX, camera.clickY, width, height);
                    if (gameState.clipboardViewState == ClipboardViewState::OnDesk) {
                        float closestDistance = std::numeric_limits<float>::max();
                        std::string closestObject;
                        for (const InteractiveObject& object : interactiveObjects) {
                            float distance = 0.0f;
                            if (rayIntersectsBox(ray, object.bounds, distance) &&
                                distance < closestDistance) {
                                closestDistance = distance;
                                closestObject = object.name;
                            }
                        }
                        if (closestObject == "Clipboard") {
                            openClipboard(gameState, camera, renderer.paperTextures.size());
                        } else if (closestObject == "FireAlarmBox") {
                            // Connect the current office model's clickable alarm to Jaylena's alarm state.
                            if (gameState.alarmActive) toggleAlarm(gameState);
                        } else if (!closestObject.empty()) {
                            std::cout << "Clicked: " << closestObject << '\n';
                        }
                    } else if (gameState.clipboardViewState == ClipboardViewState::Raised ||
                               gameState.clipboardViewState == ClipboardViewState::Raising) {
                        bool clickedClipboard = false;
                        for (const InteractiveObject& object : interactiveObjects) {
                            if (object.name != "Clipboard") continue;
                            const glm::mat4 model = clipboardModelTransform(object.bounds, gameState.clipboardProgress);
                            const glm::mat4 inverseModel = glm::inverse(model);
                            const glm::vec3 origin = glm::vec3(inverseModel * glm::vec4(ray.origin, 1.0f));
                            const glm::vec3 direction = glm::vec3(inverseModel * glm::vec4(ray.direction, 0.0f));
                            const Ray localRay{origin, direction};
                            float distance = 0.0f;
                            clickedClipboard = rayIntersectsBox(localRay, object.bounds, distance);
                            break;
                        }
                        if (!clickedClipboard) closeClipboard(gameState);
                    }
                }
            }

            if (camera.closeClipboardPending) {
                camera.closeClipboardPending = false;
                closeClipboard(gameState);
            }

            updateGame(gameState, window, deltaTime, currentTime);
            if (gameState.clipboardViewState == ClipboardViewState::OnDesk && !gameState.isPaused &&
                !gameState.gameOver && !gameState.shiftCompleted) {
                camera.locked = false;
            }
        }

        int width = 0, height = 0;
        glfwGetFramebufferSize(window, &width, &height);
        if (width == 0 || height == 0) continue;

        const glm::mat4 viewProjection = calculateViewProjection(camera, width, height);
        renderFrame(renderer, parts, interactiveObjects, viewProjection, gameState, width, height);
        updateWindowTitle(window, gameState);
        glfwSwapBuffers(window);
    }

    destroyRenderer(renderer);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
