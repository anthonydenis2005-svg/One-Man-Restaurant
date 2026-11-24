#include "ScreenUI.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cctype>
#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

struct Color {
    float r, g, b, a;
};

constexpr Color kWhite{0.97f, 0.97f, 0.95f, 1.0f};
constexpr Color kInk{0.16f, 0.14f, 0.11f, 1.0f};
constexpr Color kMuted{0.40f, 0.36f, 0.30f, 1.0f};
constexpr Color kPaper{0.87f, 0.83f, 0.73f, 1.0f};
constexpr Color kPaperLight{0.94f, 0.90f, 0.81f, 1.0f};
constexpr Color kPaperDark{0.65f, 0.59f, 0.49f, 1.0f};
constexpr Color kShadow{0.04f, 0.03f, 0.02f, 0.80f};

struct Layout {
    float width, height, scale;
    float centerX;
    float titleY;
    float firstButtonY;
    float buttonHeight;
    float buttonGap;
    float buttonWidth;
    float arrowY;
    float arrowX;
};

Layout getLayout(int width, int height) {
    const float scale = std::max(0.55f, std::min(width / 1280.0f, height / 720.0f));
    return {static_cast<float>(width), static_cast<float>(height), scale,
            width * 0.5f, height * 0.285f, height * 0.465f,
            52.0f * scale, 17.0f * scale, 345.0f * scale,
            height * 0.58f, 50.0f * scale};
}

bool inRect(float px, float py, float x, float y, float w, float h) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

int menuButtonCount(const GameState& state) {
    return state.isPaused && !state.gameOver && !state.shiftCompleted ? 3 : 2;
}

ScreenUIAction actionForButton(const GameState& state, int button) {
    if (state.isPaused && !state.gameOver && !state.shiftCompleted) {
        if (button == 0) return ScreenUIAction::Resume;
        if (button == 1) return ScreenUIAction::Restart;
        return ScreenUIAction::Quit;
    }
    return button == 0 ? ScreenUIAction::Restart : ScreenUIAction::Quit;
}

const char* labelForButton(const GameState& state, int button) {
    if (state.isPaused && !state.gameOver && !state.shiftCompleted) {
        if (button == 0) return "CONTINUE";
        if (button == 1) return "RESTART SHIFT";
        return "QUIT GAME";
    }
    return button == 0 ? "RESTART SHIFT" : "QUIT GAME";
}

using Glyph = std::array<unsigned char, 7>;
Glyph glyph(char c) {
    switch (std::toupper(static_cast<unsigned char>(c))) {
        case 'A': return {14,17,17,31,17,17,17};
        case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {15,16,16,16,16,16,15};
        case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31};
        case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {15,16,16,23,17,17,15};
        case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {31,4,4,4,4,4,31};
        case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17};
        case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17};
        case 'N': return {17,25,21,19,17,17,17};
        case 'O': return {14,17,17,17,17,17,14};
        case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13};
        case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30};
        case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14};
        case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10};
        case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4};
        case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14};
        case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31};
        case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2};
        case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14};
        case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14};
        case '9': return {14,17,17,15,1,1,14};
        case '%': return {17,18,2,4,8,9,17};
        case ':': return {0,4,4,0,4,4,0};
        case '.': return {0,0,0,0,0,12,12};
        case '-': return {0,0,0,31,0,0,0};
        case '/': return {1,2,2,4,8,8,16};
        case '!': return {4,4,4,4,4,0,4};
        case '?': return {14,17,1,2,4,0,4};
        case '[': return {14,8,8,8,8,8,14};
        case ']': return {14,2,2,2,2,2,14};
        case '+': return {0,4,4,31,4,4,0};
        case ' ': return {0,0,0,0,0,0,0};
        default: return {0,0,0,0,0,0,0};
    }
}

void rectangle(ScreenUI& ui, float x, float y, float w, float h, Color color) {
    if (w <= 0.0f || h <= 0.0f) return;
    auto add = [&](float px, float py) {
        ui.vertices.push_back({px, py, color.r, color.g, color.b, color.a});
    };
    add(x, y);     add(x + w, y); add(x + w, y + h);
    add(x, y);     add(x + w, y + h); add(x, y + h);
}

void line(ScreenUI& ui, float ax, float ay, float bx, float by, float thickness, Color color) {
    const float dx = bx - ax;
    const float dy = by - ay;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len < 0.001f) return;
    const float nx = -dy / len * thickness * 0.5f;
    const float ny =  dx / len * thickness * 0.5f;
    auto add = [&](float x, float y) {
        ui.vertices.push_back({x, y, color.r, color.g, color.b, color.a});
    };
    add(ax - nx, ay - ny); add(ax + nx, ay + ny); add(bx + nx, by + ny);
    add(ax - nx, ay - ny); add(bx + nx, by + ny); add(bx - nx, by - ny);
}

#ifdef _WIN32
bool initializeUIFont(ScreenUI& ui) {
    if (kUIFontFilePath[0] != '\0') {
        if (AddFontResourceExA(kUIFontFilePath, FR_PRIVATE, nullptr) > 0)
            ui.registeredFontPath = kUIFontFilePath;
        else
            std::cerr << "Could not register font file: " << kUIFontFilePath << '\n';
    }

    constexpr int cols = 16;
    constexpr int cellWidth = 80;
    constexpr int cellHeight = 82;
    constexpr int atlasWidth = cols * cellWidth;
    constexpr int atlasHeight = 6 * cellHeight;

    HDC dc = CreateCompatibleDC(nullptr);
    if (!dc) return false;
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = atlasWidth;
    bitmapInfo.bmiHeader.biHeight = -atlasHeight;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS,
                                      &pixels, nullptr, 0);
    if (!bitmap || !pixels) {
        if (bitmap) DeleteObject(bitmap);
        DeleteDC(dc);
        return false;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontA(-54, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                            kUIFontFamily);
    if (!font) {
        SelectObject(dc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
        return false;
    }
    HGDIOBJ oldFont = SelectObject(dc, font);
    PatBlt(dc, 0, 0, atlasWidth, atlasHeight, BLACKNESS);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(255, 255, 255));
    SetTextAlign(dc, TA_LEFT | TA_TOP);

    TEXTMETRICA metrics{};
    GetTextMetricsA(dc, &metrics);
    ui.fontLineHeight = static_cast<float>(std::max(metrics.tmHeight, 1L) + 4);

    for (int i = 0; i < 95; ++i) {
        const char character = static_cast<char>(i + 32);
        const int col = i % cols;
        const int row = i / cols;
        const int cellX = col * cellWidth;
        const int cellY = row * cellHeight;
        SIZE textSize{};
        GetTextExtentPoint32A(dc, &character, 1, &textSize);
        const int advance = std::clamp(static_cast<int>(textSize.cx), 1, cellWidth - 12);
        const int drawWidth = std::min(cellWidth - 4, advance + 10);
        TextOutA(dc, cellX + 2, cellY + 2, &character, 1);
        ScreenUIFontGlyph& glyph = ui.fontGlyphs[static_cast<size_t>(i)];
        glyph.u0 = static_cast<float>(cellX) / atlasWidth;
        glyph.v0 = static_cast<float>(cellY) / atlasHeight;
        glyph.u1 = static_cast<float>(cellX + drawWidth) / atlasWidth;
        glyph.v1 = static_cast<float>(cellY + cellHeight) / atlasHeight;
        glyph.drawWidth = static_cast<float>(drawWidth);
        glyph.advance = static_cast<float>(advance);
    }

    const auto* bgra = static_cast<const std::uint8_t*>(pixels);
    std::vector<std::uint8_t> rgba(static_cast<size_t>(atlasWidth) * atlasHeight * 4u);
    for (size_t i = 0, n = static_cast<size_t>(atlasWidth) * atlasHeight; i < n; ++i) {
        const size_t j = i * 4u;
        const std::uint8_t value = std::max({bgra[j], bgra[j + 1], bgra[j + 2]});
        rgba[j] = rgba[j + 1] = rgba[j + 2] = 255;
        rgba[j + 3] = value;
    }
    SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(font);
    DeleteObject(bitmap);
    DeleteDC(dc);

    GLint priorActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &priorActiveTexture);
    glActiveTexture(GL_TEXTURE1);
    GLint priorTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &priorTexture);
    glGenTextures(1, &ui.fontTexture);
    glBindTexture(GL_TEXTURE_2D, ui.fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, atlasWidth, atlasHeight,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(priorTexture));
    glActiveTexture(static_cast<GLenum>(priorActiveTexture));
    ui.fontReady = true;
    return true;
}
#else
bool initializeUIFont(ScreenUI&) { return false; }
#endif

float fontScale(const ScreenUI& ui, float pixel) {
    return 7.6f * pixel / std::max(ui.fontLineHeight, 1.0f);
}

float textWidth(const ScreenUI& ui, const std::string& text, float pixel) {
    if (!ui.fontReady)
        return static_cast<float>(text.size()) * 6.0f * pixel - (text.empty() ? 0.0f : pixel);
    const float scale = fontScale(ui, pixel);
    float width = 0.0f;
    for (unsigned char c : text) {
        if (c >= 32 && c <= 126)
            width += ui.fontGlyphs[static_cast<size_t>(c - 32)].advance * scale;
    }
    return width;
}

void fontRectangle(ScreenUI& ui, float x, float y, float w, float h,
                   const ScreenUIFontGlyph& glyph, Color color) {
    auto add = [&](float px, float py, float u, float v) {
        ui.vertices.push_back({px, py, color.r, color.g, color.b, color.a, u, v, 1.0f});
    };
    add(x, y, glyph.u0, glyph.v0);
    add(x + w, y, glyph.u1, glyph.v0);
    add(x + w, y + h, glyph.u1, glyph.v1);
    add(x, y, glyph.u0, glyph.v0);
    add(x + w, y + h, glyph.u1, glyph.v1);
    add(x, y + h, glyph.u0, glyph.v1);
}

void pixelText(ScreenUI& ui, const std::string& text, float x, float y,
               float pixel, Color color, bool center = false) {
    if (center) x -= textWidth(ui, text, pixel) * 0.5f;
    if (ui.fontReady) {
        const float scale = fontScale(ui, pixel);
        for (unsigned char c : text) {
            if (c < 32 || c > 126) continue;
            const ScreenUIFontGlyph& glyph = ui.fontGlyphs[static_cast<size_t>(c - 32)];
            if (c != ' ')
                fontRectangle(ui, x, y, glyph.drawWidth * scale,
                              82.0f * scale, glyph, color);
            x += glyph.advance * scale;
        }
        return;
    }
    for (char c : text) {
        const Glyph bits = glyph(c);
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if ((bits[row] & (1 << (4 - col))) != 0)
                    rectangle(ui, x + col * pixel, y + row * pixel,
                              pixel * 0.92f, pixel * 0.92f, color);
            }
        }
        x += 6.0f * pixel;
    }
}

void outlinedText(ScreenUI& ui, const std::string& text, float x, float y,
                  float pixel, Color color, bool center = false) {
    pixelText(ui, text, x + pixel * 0.45f, y + pixel * 0.50f,
              pixel, Color{kShadow.r,kShadow.g,kShadow.b,0.16f}, center);
    pixelText(ui, text, x, y, pixel, color, center);
}

void drawArrow(ScreenUI& ui, const Layout& layout, bool lookingRight, bool hovered) {
    const float s = layout.scale;
    const float x = lookingRight ? layout.arrowX : layout.width - layout.arrowX;
    const float y = layout.arrowY;
    const float direction = lookingRight ? -1.0f : 1.0f;
    const Color color = hovered ? kPaperLight : kWhite;
    rectangle(ui, x - 39 * s, y - 44 * s, 78 * s, 88 * s,
              hovered ? Color{0.21f, 0.20f, 0.17f, 0.56f} : Color{0.0f, 0.0f, 0.0f, 0.23f});
    line(ui, x - direction * 12 * s, y - 25 * s,
         x + direction * 14 * s, y, 6 * s, color);
    line(ui, x + direction * 14 * s, y,
         x - direction * 12 * s, y + 25 * s, 6 * s, color);
}

void menu(ScreenUI& ui, const Layout& layout, const GameState& game,
          ScreenUIAction hoverAction) {
    const float s = layout.scale;
    const float paperW = 590.0f * s;
    const float paperH = 510.0f * s;
    const float paperX = layout.centerX - paperW * 0.5f;
    const float paperY = (layout.height - paperH) * 0.5f;

    rectangle(ui, 0, 0, layout.width, layout.height, {0.018f, 0.016f, 0.013f, 0.76f});
    rectangle(ui, paperX + 13*s, paperY + 16*s, paperW, paperH,
              {0.0f, 0.0f, 0.0f, 0.45f});
    rectangle(ui, paperX - 5*s, paperY - 5*s, paperW + 10*s, paperH + 10*s,
              kPaperDark);
    rectangle(ui, paperX, paperY, paperW, paperH, kPaper);
    rectangle(ui, paperX + 7*s, paperY + 7*s, paperW - 14*s, paperH - 14*s,
              {0.93f, 0.89f, 0.79f, 0.66f});

    for (int i=0; i<10; ++i) {
        rectangle(ui, paperX + 24*s, paperY + (70 + 39*i)*s,
                  paperW - 48*s, 1*s, {0.29f, 0.25f, 0.20f, 0.075f});
    }
    rectangle(ui, paperX + 51*s, paperY + 18*s, 2*s, paperH - 36*s,
              {0.53f, 0.30f, 0.24f, 0.24f});
    rectangle(ui, paperX + 34*s, paperY + 29*s, 13*s, 5*s, kPaperDark);
    rectangle(ui, paperX + paperW - 48*s, paperY + 29*s, 13*s, 5*s, kPaperDark);

    const char* title = game.gameOver ? "YOU'RE FIRED" :
                        game.shiftCompleted ? "SHIFT COMPLETED" : "GAME PAUSED";
    const char* subtitle = game.gameOver ? "CUSTOMER SATISFACTION REACHED ZERO" :
                           game.shiftCompleted ? "END OF SHIFT REPORT" :
                                                 "MANAGER'S DESK";

    pixelText(ui, "ONE MAN RESTAURANT", layout.centerX,
              paperY + 37*s, 1.85f*s, kMuted, true);
    outlinedText(ui, title, layout.centerX,
                 paperY + 86*s, 4.65f*s, kInk, true);
    pixelText(ui, subtitle, layout.centerX,
              paperY + 139*s, 1.85f*s, kMuted, true);
    rectangle(ui, paperX + 75*s, paperY + 184*s,
              paperW - 150*s, 2*s, kInk);

    const int count = menuButtonCount(game);
    for (int i=0; i<count; ++i) {
        const ScreenUIAction action = actionForButton(game, i);
        const bool hovered = hoverAction == action;
        const float y = layout.firstButtonY + i*(layout.buttonHeight + layout.buttonGap);
        const float x = layout.centerX - layout.buttonWidth*0.5f;
        rectangle(ui, x, y, layout.buttonWidth, layout.buttonHeight,
                  hovered ? Color{0.72f, 0.66f, 0.55f, 0.58f} :
                            Color{0.94f, 0.90f, 0.81f, 0.43f});
        rectangle(ui, x + 15*s, y + layout.buttonHeight - 5*s,
                  layout.buttonWidth - 30*s, 1*s,
                  hovered ? kInk : Color{0.26f, 0.22f, 0.17f, 0.40f});
        if (hovered) {
            line(ui, x + 19*s, y + 18*s, x + 29*s, y + 26*s, 3*s, kInk);
            line(ui, x + 29*s, y + 26*s, x + 19*s, y + 34*s, 3*s, kInk);
        }
        pixelText(ui, labelForButton(game, i), layout.centerX,
                  y + 12.5f*s, 3.10f*s, kInk, true);
    }

    const char* footer = game.isPaused && !game.gameOver && !game.shiftCompleted ?
                         "ESC OR P TO RESUME" : "R TO RESTART";
    pixelText(ui, footer, layout.centerX, paperY + paperH - 34*s,
              1.75f*s, kMuted, true);
}

int stressLevel(const GameState&) {
    return 0;
}

void hud(ScreenUI& ui, const Layout& layout, const GameState& game,
         bool lookingRight, ScreenUIAction hoverAction) {
    const float s = layout.scale;
    const float x = 22*s;
    const float y = 22*s;
    const Color hudPanel{0.025f, 0.022f, 0.020f, 0.66f};

    rectangle(ui, x - 9*s, y - 8*s, 228*s, 48*s, hudPanel);
    pixelText(ui, "SATISFACTION", x, y - 1*s, 2.10f*s, kWhite);
    char percent[32]{};
    std::snprintf(percent, sizeof(percent), "%d%%",
                  static_cast<int>(std::round(std::clamp(game.customerHappiness, 0.0f, 100.0f))));
    pixelText(ui, percent, x + 157*s, y - 1*s, 2.65f*s,
              game.customerHappiness <= 25.0f ?
              Color{1.0f, 0.50f, 0.46f, 1.0f} : kWhite);

    const float stressX = x;
    const float stressY = y + 54*s;
    rectangle(ui, stressX - 9*s, stressY - 7*s, 151*s, 36*s, hudPanel);
    const int stress = stressLevel(game);
    for (int i=0; i<4; ++i) {
        const float bx = stressX + i*32*s;
        const Color filled = i < 2 ? Color{0.67f, 0.86f, 0.58f, 0.98f} :
                             i == 2 ? Color{0.98f, 0.80f, 0.43f, 0.98f} :
                                      Color{0.97f, 0.40f, 0.36f, 0.98f};
        rectangle(ui, bx, stressY + 4*s, 26*s, 12*s,
                  i < stress ? filled : Color{0.33f, 0.31f, 0.28f, 0.72f});
    }

    if (game.paperworkPenaltyFeedback > 0.0f) {
        const float fade = std::min(1.0f, game.paperworkPenaltyFeedback / 0.4f);
        const float rise = (1.35f - game.paperworkPenaltyFeedback) * 18.0f * s;
        pixelText(ui, "-PENALTY", layout.width * 0.50f,
                  layout.height * 0.42f - rise, 2.8f*s,
                  Color{0.98f, 0.18f, 0.15f, fade}, true);
    }

    drawArrow(ui, layout, lookingRight,
              hoverAction == (lookingRight ? ScreenUIAction::PanCenter : ScreenUIAction::PanRight));
}

GLuint compileShader(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        char log[1024] = {};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "ScreenUI shader compilation failed: " << log << '\n';
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

}

bool initializeScreenUI(ScreenUI& ui) {
    static constexpr const char* vs = R"GLSL(#version 330 core
layout(location=0) in vec2 aPosition;
layout(location=1) in vec4 aColor;
layout(location=2) in vec2 aUV;
layout(location=3) in float aIsGlyph;
uniform vec2 uViewport;
out vec4 vColor;
out vec2 vUV;
out float vIsGlyph;
void main() {
    vec2 ndc = (aPosition / uViewport) * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
    vColor = aColor;
    vUV = aUV;
    vIsGlyph = aIsGlyph;
})GLSL";
    static constexpr const char* fs = R"GLSL(#version 330 core
in vec4 vColor;
in vec2 vUV;
in float vIsGlyph;
uniform sampler2D uFontAtlas;
out vec4 fragmentColor;
void main() {
    fragmentColor = vColor;
    if (vIsGlyph > 0.5) fragmentColor.a *= texture(uFontAtlas, vUV).a;
}
)GLSL";
    const GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vs);
    const GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!vertexShader || !fragmentShader) {
        if (vertexShader) glDeleteShader(vertexShader);
        if (fragmentShader) glDeleteShader(fragmentShader);
        return false;
    }
    ui.program = glCreateProgram();
    glAttachShader(ui.program, vertexShader);
    glAttachShader(ui.program, fragmentShader);
    glLinkProgram(ui.program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    GLint linked = GL_FALSE;
    glGetProgramiv(ui.program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
        std::cerr << "ScreenUI shader linking failed.\n";
        destroyScreenUI(ui);
        return false;
    }
    ui.viewportLocation = glGetUniformLocation(ui.program, "uViewport");
    ui.fontSamplerLocation = glGetUniformLocation(ui.program, "uFontAtlas");
    glGenVertexArrays(1, &ui.vertexArray);
    glGenBuffers(1, &ui.vertexBuffer);
    glBindVertexArray(ui.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, ui.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4096 * sizeof(ScreenUIVertex), nullptr, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenUIVertex),
                          reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ScreenUIVertex),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(ScreenUIVertex),
                          reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(ScreenUIVertex),
                          reinterpret_cast<void*>(8 * sizeof(float)));
    glBindVertexArray(0);
    if (!initializeUIFont(ui))
        std::cerr << "Font atlas unavailable; using built-in pixel font.\n";
    return true;
}

void destroyScreenUI(ScreenUI& ui) {
    if (ui.vertexBuffer) glDeleteBuffers(1, &ui.vertexBuffer);
    if (ui.vertexArray) glDeleteVertexArrays(1, &ui.vertexArray);
    if (ui.program) glDeleteProgram(ui.program);
    if (ui.fontTexture) glDeleteTextures(1, &ui.fontTexture);
#ifdef _WIN32
    if (!ui.registeredFontPath.empty())
        RemoveFontResourceExA(ui.registeredFontPath.c_str(), FR_PRIVATE, nullptr);
#endif
    ui.fontTexture = 0;
    ui.fontReady = false;
    ui.vertexBuffer = ui.vertexArray = ui.program = 0;
    ui.vertices.clear();
}

bool screenUIHasMenu(const GameState& game) {
    return game.isPaused || game.gameOver || game.shiftCompleted;
}

bool screenUIPanActive(const ScreenUI& ui) {
    return ui.lookingRight || std::abs(ui.panDegrees) > 0.01f;
}

ScreenUIAction hitTestScreenUI(const ScreenUI& ui, const GameState& game,
                               double mouseX, double mouseY,
                               int windowWidth, int windowHeight,
                               int framebufferWidth, int framebufferHeight) {
    if (windowWidth < 1 || windowHeight < 1 || framebufferWidth < 1 || framebufferHeight < 1)
        return ScreenUIAction::None;
    const float px = static_cast<float>(mouseX * framebufferWidth / windowWidth);
    const float py = static_cast<float>(mouseY * framebufferHeight / windowHeight);
    const Layout layout = getLayout(framebufferWidth, framebufferHeight);
    if (screenUIHasMenu(game)) {
        const int count = menuButtonCount(game);
        for (int i = 0; i < count; ++i) {
            const float y = layout.firstButtonY + i * (layout.buttonHeight + layout.buttonGap);
            if (inRect(px, py, layout.centerX - layout.buttonWidth * 0.5f, y,
                       layout.buttonWidth, layout.buttonHeight))
                return actionForButton(game, i);
        }
        return ScreenUIAction::None;
    }

    if (game.clipboardViewState == ClipboardViewState::OnDesk) {
        const bool lookingRight = ui.lookingRight;
        const float x = lookingRight ? layout.arrowX : layout.width - layout.arrowX;
        if (inRect(px, py, x - 39 * layout.scale, layout.arrowY - 44 * layout.scale,
                   78 * layout.scale, 88 * layout.scale)) {
            return lookingRight ? ScreenUIAction::PanCenter : ScreenUIAction::PanRight;
        }
    }
    return ScreenUIAction::None;
}

void startScreenUIPan(ScreenUI& ui, CameraState& camera, bool lookingRight) {
    if (lookingRight && !ui.lookingRight && std::abs(ui.panDegrees) < 0.01f) {
        ui.panBaseYaw = camera.yaw;
    }
    ui.lookingRight = lookingRight;
    camera.rotating = false;
}

void updateScreenUIPan(ScreenUI& ui, CameraState& camera, float deltaTime) {
    const bool wasPanning = screenUIPanActive(ui);
    const float target = ui.lookingRight ? kOfficePanAngleDegrees : 0.0f;
    const float step = std::max(0.0f, kOfficePanSpeedDegreesPerSecond) * std::max(0.0f, deltaTime);
    if (std::abs(ui.panDegrees - target) <= step)
        ui.panDegrees = target;
    else
        ui.panDegrees += ui.panDegrees < target ? step : -step;
    if (wasPanning || screenUIPanActive(ui))
        camera.yaw = ui.panBaseYaw + ui.panDegrees;
}

void resetScreenUIPan(ScreenUI& ui) {
    ui.lookingRight = false;
    ui.panDegrees = 0.0f;
    ui.panBaseYaw = kInitialCameraYaw;
}

void renderScreenUI(ScreenUI& ui, const GameState& game,
                    int framebufferWidth, int framebufferHeight,
                    double cursorWindowX, double cursorWindowY,
                    int windowWidth, int windowHeight) {
    if (framebufferWidth < 1 || framebufferHeight < 1 || !ui.program) return;
    ui.vertices.clear();
    const Layout layout = getLayout(framebufferWidth, framebufferHeight);
    const ScreenUIAction hovered = hitTestScreenUI(ui, game, cursorWindowX, cursorWindowY,
                                                  windowWidth, windowHeight,
                                                  framebufferWidth, framebufferHeight);
    if (screenUIHasMenu(game)) menu(ui, layout, game, hovered);
    else hud(ui, layout, game, ui.lookingRight, hovered);

    GLint oldProgram = 0, oldVertexArray = 0, oldArrayBuffer = 0;
    GLint oldSrcRGB = GL_ONE, oldDstRGB = GL_ZERO;
    GLint oldSrcAlpha = GL_ONE, oldDstAlpha = GL_ZERO;
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &oldVertexArray);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &oldArrayBuffer);
    glGetIntegerv(GL_BLEND_SRC_RGB, &oldSrcRGB);
    glGetIntegerv(GL_BLEND_DST_RGB, &oldDstRGB);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &oldSrcAlpha);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &oldDstAlpha);
    GLint oldActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActiveTexture);
    glActiveTexture(GL_TEXTURE1);
    GLint oldFontTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldFontTexture);
    glBindTexture(GL_TEXTURE_2D, ui.fontTexture);
    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(ui.program);
    glUniform2f(ui.viewportLocation, static_cast<float>(framebufferWidth),
                static_cast<float>(framebufferHeight));
    glUniform1i(ui.fontSamplerLocation, 1);
    glBindVertexArray(ui.vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, ui.vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(ui.vertices.size() * sizeof(ScreenUIVertex)),
                 ui.vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(ui.vertices.size()));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(oldFontTexture));
    glActiveTexture(static_cast<GLenum>(oldActiveTexture));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(oldArrayBuffer));
    glBindVertexArray(static_cast<GLuint>(oldVertexArray));
    glUseProgram(static_cast<GLuint>(oldProgram));
    glBlendFuncSeparate(static_cast<GLenum>(oldSrcRGB), static_cast<GLenum>(oldDstRGB),
                        static_cast<GLenum>(oldSrcAlpha), static_cast<GLenum>(oldDstAlpha));
    if (!blendEnabled) glDisable(GL_BLEND);
    if (depthEnabled) glEnable(GL_DEPTH_TEST);
    if (cullEnabled) glEnable(GL_CULL_FACE);
}
