#pragma once

#include "Camera.h"
#include "GameState.h"

#include <glad/glad.h>
#include <vector>
#include <array>
#include <string>

constexpr float kOfficePanAngleDegrees = 70.0f;
constexpr float kOfficePanSpeedDegreesPerSecond = 150.0f;

constexpr const char* kUIFontFamily = "Georgia";

constexpr const char* kUIFontFilePath = "";

enum class ScreenUIAction {
    None, Resume, Restart, Quit, PanRight, PanCenter
};

struct ScreenUIVertex {
    float x = 0.0f;
    float y = 0.0f;
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
    float u = 0.0f;
    float v = 0.0f;
    float isGlyph = 0.0f;
};

struct ScreenUIFontGlyph {
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    float drawWidth = 0.0f, advance = 0.0f;
};

struct ScreenUI {
    GLuint program = 0;
    GLuint vertexArray = 0;
    GLuint vertexBuffer = 0;
    GLint viewportLocation = -1;
    GLint fontSamplerLocation = -1;
    GLuint fontTexture = 0;
    std::array<ScreenUIFontGlyph, 95> fontGlyphs{};
    float fontLineHeight = 70.0f;
    bool fontReady = false;
    std::string registeredFontPath;

    bool lookingRight = false;
    float panDegrees = 0.0f;
    float panBaseYaw = kInitialCameraYaw;
    std::vector<ScreenUIVertex> vertices;
};

bool initializeScreenUI(ScreenUI& ui);
void destroyScreenUI(ScreenUI& ui);

bool screenUIHasMenu(const GameState& game);
bool screenUIPanActive(const ScreenUI& ui);

ScreenUIAction hitTestScreenUI(const ScreenUI& ui, const GameState& game,
                               double mouseX, double mouseY,
                               int windowWidth, int windowHeight,
                               int framebufferWidth, int framebufferHeight);

void startScreenUIPan(ScreenUI& ui, CameraState& camera, bool lookingRight);
void updateScreenUIPan(ScreenUI& ui, CameraState& camera, float deltaTime);
void resetScreenUIPan(ScreenUI& ui);

void renderScreenUI(ScreenUI& ui, const GameState& game,
                    int framebufferWidth, int framebufferHeight,
                    double cursorWindowX, double cursorWindowY,
                    int windowWidth, int windowHeight);
