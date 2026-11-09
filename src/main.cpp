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

// Set up the model, window, input, and renderer before starting the game loop.
int main() {
    const auto modelPath = findModelPath();

    std::vector<float> vertices;
    std::vector<Part> parts;
    std::vector<TextureImage> images;
    std::vector<InteractiveObject> interactiveObjects;

    if (modelPath.empty()) {
        std::cerr << "No GLB model found. Place office.glb or "
                     "tutorial_donut1.glb inside assets/models/.\n";
        return 1;
    }

    if (!loadOfficeModel(modelPath, vertices, parts, images, interactiveObjects)) {
        return 1;
    }

    if (glfwInit() != GLFW_TRUE) {
        std::cerr << "Could not initialize GLFW.\n";
        return 1;
    }

    // Request an OpenGL 3.3 window for the renderer.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "One Man Restaurant - Office",
        nullptr,
        nullptr
    );

    if (!window) {
        std::cerr << "Could not create the OpenGL window.\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
    )) {
        std::cerr << "Could not initialize GLAD.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // GLFW callbacks use this camera state to store mouse movement and pending clicks.
    CameraState camera;

    glfwSetWindowUserPointer(window, &camera);
    glfwSetFramebufferSizeCallback(window, onFramebufferResize);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursorMove);

    // Create the GPU resources once instead of reloading the model each frame.
    RendererState renderer;
    if (!initializeRenderer(renderer, vertices, images, modelPath)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::cout << "Hold middle mouse and drag to rotate. "
                 "Left-click the clipboard, click outside or right-click to return. Esc to exit.\n";

    // Initializing the game state and for the time tracking before the main loop
    GameState gameState;
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

       // --- FEATURE #1:Update delata time and shift timer
        double currentTime = glfwGetTime();
        float deltaTime = static_cast<float>(currentTime - lastTime);
        lastTime = currentTime;

        // The input callback saved a left click, so we now check what was clicked.
        if (camera.clickPending) {
            camera.clickPending = false;
            int width = 0, height = 0;
            glfwGetWindowSize(window, &width, &height);
            if (width > 0 && height > 0) {
                const Ray ray = makeMouseRay(camera, camera.clickX, camera.clickY, width, height);
                if (gameState.clipboardViewState == ClipboardViewState::OnDesk) {
                    // If the ray hits more than one box, we use the closest one.
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
                    // The matched object name decides which interaction runs.
                    if (closestObject == "Clipboard") {
                        openClipboard(gameState, camera, renderer.paperTextures.size());
                    } else if (!closestObject.empty()) {
                        std::cout << "Clicked: " << closestObject << '\n';
                    }
                } else if (gameState.clipboardViewState == ClipboardViewState::Raised ||
                           gameState.clipboardViewState == ClipboardViewState::Raising) {
                    // Clicking outside the raised clipboard sends it back to the desk.
                    bool clickedClipboard = false;
                    for (const InteractiveObject& object : interactiveObjects) {
                        if (object.name != "Clipboard") continue;
                        // Undo the clipboard movement before testing its original bounding box.
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
        // Right click closes the clipboard without needing a ray check.
        if (camera.closeClipboardPending) {
            camera.closeClipboardPending = false;
            closeClipboard(gameState);
        }

        // Advance game timers and clipboard movement using the time since last frame.
        updateGame(gameState, window, deltaTime, currentTime);
        if (gameState.clipboardViewState == ClipboardViewState::OnDesk) camera.locked = false;

        int width = 0;
        int height = 0;

        // Use the framebuffer size for rendering and skip frames while minimized.
        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0) {
            continue;
        }
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
