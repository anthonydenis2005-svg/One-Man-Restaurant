#include "Camera.h"
#include "Audio.h"
#include "Game.h"
#include "GameState.h"
#include "Input.h"
#include "ModelLoader.h"
#include "Renderer.h"
#include "SecurityCamera.h"
#include "UIController.h"
#include "Paperwork.h"

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

    std::vector<Part> restaurantParts;
    std::vector<ImportedSecurityCamera> restaurantViews;
    const auto restaurantPath = findRestaurantModelPath(modelPath);
    if (restaurantPath.empty()) {
        std::cerr << "Missing assets/models/Restaurants.glb. CCTV will be unavailable.\n";
    } else if (!loadRestaurantModel(restaurantPath, vertices, restaurantParts,
                                     images, restaurantViews)) {
        std::cerr << "Could not load the restaurant scene. CCTV will be unavailable.\n";
    } else if (restaurantViews.empty()) {
        std::cerr << "Restaurants.glb contains no exported Blender cameras.\n";
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
        "One Man Restaurant",
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
    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int button, int action, int mods) {
        if (button != GLFW_MOUSE_BUTTON_MIDDLE) onMouseButton(w, button, action, mods);
    });
    glfwSetCursorPosCallback(window, onCursorMove);

    // Create the GPU resources once instead of reloading the model each frame.
    RendererState renderer;
    if (!initializeRenderer(renderer, vertices, images, modelPath)) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    UIController uiController;
    if (!initializeUIController(uiController)) {
        std::cerr << "Could not initialize screen menus and HUD.\n";
        destroyRenderer(renderer);
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    SecurityCamera securityCamera;
    if (!restaurantParts.empty() && !restaurantViews.empty()) {
        securityCamera.initialize(restaurantViews);
    }
    bool foundMonitor = false;
    for (const Part& part : parts) {
        if (part.name == "CameraScreen" || part.interactiveName == "CameraScreen") {
            foundMonitor = true;
            break;
        }
    }
    if (!foundMonitor) {
        std::cerr << "office.glb is missing the CameraScreen mesh; export office05.blend.\n";
    }

#ifdef OMR_VERBOSE_LOGS
    std::cout << "Use the on-screen arrows to pan the office; click CameraScreen to switch feeds. "
                 "Left-click the clipboard and right-click to close it. P or Esc opens pause.\n";
#endif

    // Initializing the game state and for the time tracking before the main loop
    GameState gameState;
    assignDailyPaperwork(gameState, renderer.paperTextureNames);
    AudioState audio;
    initializeAudio(audio);
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        prepareUIInput(uiController, gameState, camera);
        glfwPollEvents();

        processUIInput(uiController, gameState, camera, securityCamera, window);

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
                    } else if (closestObject == "CameraScreen") {
                        securityCamera.nextView();
                    } else if (closestObject == "FireAlarmBox" && gameState.alarmActive) {
                        toggleAlarm(gameState);
                    } else if (!closestObject.empty()) {
#ifdef OMR_VERBOSE_LOGS
                        std::cout << "Clicked: " << closestObject << '\n';
#endif
                    }
                } else if (gameState.clipboardViewState == ClipboardViewState::Raised ||
                           gameState.clipboardViewState == ClipboardViewState::Raising) {
                    const int paperSlot = hitTestClipboardPaper(ray, vertices, parts,
                                                                interactiveObjects, gameState);
                    if (paperSlot != 0) {
                        clickPaperworkSlot(gameState, paperSlot);
                    } else {
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
                    if (!clickedClipboard && !gameState.paperSliding) closeClipboard(gameState);
                    }
                }
            }
        }
        // Right click closes the clipboard without needing a ray check.
        if (camera.closeClipboardPending) {
            camera.closeClipboardPending = false;
            if (!gameState.paperSliding) closeClipboard(gameState);
        }

        if (screenUIHasMenu(gameState)) deltaTime = 0.0f;
        // Advance game timers and clipboard movement using the time since last frame.
        updateGame(gameState, window, deltaTime, currentTime);
        updatePaperwork(gameState, deltaTime);
        updateUIController(uiController, gameState, camera, deltaTime);
        if (gameState.clipboardViewState == ClipboardViewState::OnDesk) camera.locked = false;

        int width = 0;
        int height = 0;

        // Use the framebuffer size for rendering and skip frames while minimized.
        glfwGetFramebufferSize(window, &width, &height);

        if (width == 0 || height == 0) {
            continue;
        }
        if (!screenUIHasMenu(gameState)) {
        if (securityCamera.available()) {
            securityCamera.beginCapture();
            renderRestaurantScene(renderer, restaurantParts,
                                  securityCamera.viewProjection(),
                                  securityCamera.width(), securityCamera.height());
            securityCamera.endCapture();
        }
        }
        const glm::mat4 viewProjection = calculateViewProjection(camera, width, height);
        renderFrame(renderer, parts, interactiveObjects, viewProjection, gameState,
                    width, height, securityCamera.texture());
        renderUIController(uiController, gameState, window, width, height);
        syncLoopingAudio(audio, gameState);

        glfwSwapBuffers(window);
    }
    securityCamera.destroy();
    destroyUIController(uiController);
    shutdownAudio(audio);
    destroyRenderer(renderer);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
