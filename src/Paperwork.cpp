#include "Paperwork.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

namespace {

unsigned int maskFromFilename(const std::string& filename) {
    const std::size_t extension = filename.find_last_of('.');
    const std::string stem = filename.substr(0, extension);
    std::size_t first = stem.size();
    while (first > 0 && stem[first - 1] >= '0' && stem[first - 1] <= '9') --first;
    unsigned int result = 0;
    for (std::size_t i = first; i < stem.size(); ++i) {
        const char digit = stem[i];
        if (digit >= '1' && digit <= '9') result |= 1u << (digit - '1');
    }
    return result;
}

bool intersectTriangle(const Ray& ray, const glm::vec3& a, const glm::vec3& b,
                       const glm::vec3& c, float& distance, float& u, float& v) {
    const glm::vec3 ab = b - a;
    const glm::vec3 ac = c - a;
    const glm::vec3 p = glm::cross(ray.direction, ac);
    const float det = glm::dot(ab, p);
    if (std::abs(det) < 0.000001f) return false;
    const float inverse = 1.0f / det;
    const glm::vec3 t = ray.origin - a;
    u = glm::dot(t, p) * inverse;
    if (u < 0.0f || u > 1.0f) return false;
    const glm::vec3 q = glm::cross(t, ab);
    v = glm::dot(ray.direction, q) * inverse;
    if (v < 0.0f || u + v > 1.0f) return false;
    distance = glm::dot(ac, q) * inverse;
    return distance > 0.00001f;
}

int slotFromPaperUV(const glm::vec2& uv) {
    const float visualV = 1.0f - uv.y;
    if (uv.x < 0.0f || visualV < 0.0f || uv.x >= 1.0f || visualV >= 1.0f)
        return 0;
    if (visualV < kPaperworkGridTop) return 0;
    const float cx = uv.x * 3.0f;
    const float cy = ((visualV - kPaperworkGridTop) / (1.0f - kPaperworkGridTop)) * 4.0f;
    const int col = static_cast<int>(std::floor(cx));
    const int row = static_cast<int>(std::floor(cy));
    const float fx = cx - static_cast<float>(col);
    const float fy = cy - static_cast<float>(row);
    if (fx < kPaperworkHitboxInset || fx > 1.0f - kPaperworkHitboxInset ||
        fy < kPaperworkHitboxInset || fy > 1.0f - kPaperworkHitboxInset)
        return 0;
    return row * 3 + col + 1;
}

}

void assignDailyPaperwork(GameState& state, const std::vector<std::string>& pngFilenames) {
    state.paperSheets.clear();
    constexpr std::size_t sheetCount = 4;
    for (std::size_t i = 0; i < pngFilenames.size() &&
                            state.paperSheets.size() < sheetCount; ++i) {
        const unsigned int requiredMask = maskFromFilename(pngFilenames[i]);
        if (!requiredMask) {
            std::cerr << "Skipping paperwork image without a trailing 1-9 checkbox suffix: "
                      << pngFilenames[i] << '\n';
            continue;
        }
        PaperSheet sheet;
        sheet.filename = pngFilenames[i];
        sheet.textureIndex = static_cast<int>(i);
        sheet.requiredMask = requiredMask;
        state.paperSheets.push_back(sheet);
    }
    const std::size_t uniqueSheets = state.paperSheets.size();
    while (uniqueSheets > 0 && state.paperSheets.size() < sheetCount) {
        state.paperSheets.push_back(state.paperSheets[state.paperSheets.size() % uniqueSheets]);
    }
    restartDailyPaperwork(state);
}

void restartDailyPaperwork(GameState& state) {
    state.activePaperIndex = 0;
    state.paperSliding = false;
    state.paperSlideProgress = 0.0f;
    state.paperworkPenaltyFeedback = 0.0f;
    state.paperworkStampedCount = 0;
    state.paperworkCorrectCount = 0;
    state.paperworkAccuracy = 0.0f;
    for (PaperSheet& sheet : state.paperSheets) {
        sheet.checkedMask = 0;
        sheet.stamped = false;
        sheet.completedOnStamp = false;
    }
    state.paperworkActive = !state.paperSheets.empty();
    state.selectedPaperTexture = state.paperworkActive ? state.paperSheets[0].textureIndex : -1;
}

const PaperSheet* currentPaperSheet(const GameState& state) {
    if (state.activePaperIndex >= state.paperSheets.size()) return nullptr;
    return &state.paperSheets[state.activePaperIndex];
}

int hitTestClipboardPaper(const Ray& ray, const std::vector<float>& vertices,
                          const std::vector<Part>& parts,
                          const std::vector<InteractiveObject>& interactiveObjects,
                          const GameState& state) {
    const PaperSheet* sheet = currentPaperSheet(state);
    if (!sheet || sheet->stamped || state.paperSliding ||
        state.clipboardViewState != ClipboardViewState::Raised) return 0;

    glm::mat4 clipboardModel(1.0f);
    for (const InteractiveObject& object : interactiveObjects) {
        if (object.name == "Clipboard") {
            clipboardModel = clipboardModelTransform(object.bounds, state.clipboardProgress);
            break;
        }
    }
    const glm::mat4 inverseModel = glm::inverse(clipboardModel);
    const Ray localRay{
        glm::vec3(inverseModel * glm::vec4(ray.origin, 1.0f)),
        glm::vec3(inverseModel * glm::vec4(ray.direction, 0.0f))
    };

    float nearest = std::numeric_limits<float>::max();
    glm::vec2 hitUV(0.0f);
    bool hit = false;
    for (const Part& part : parts) {
        if (!part.isClipboardPaper) continue;
        for (GLint offset = 0; offset + 2 < part.count; offset += 3) {
            const std::size_t first = static_cast<std::size_t>(part.first + offset) * 10;
            if (first + 29 >= vertices.size()) continue;
            const glm::vec3 a(vertices[first], vertices[first+1], vertices[first+2]);
            const glm::vec3 b(vertices[first+10], vertices[first+11], vertices[first+12]);
            const glm::vec3 c(vertices[first+20], vertices[first+21], vertices[first+22]);
            float distance = 0.0f, u = 0.0f, v = 0.0f;
            if (!intersectTriangle(localRay, a, b, c, distance, u, v) || distance >= nearest)
                continue;
            nearest = distance;
            const glm::vec2 uv0(vertices[first+8], vertices[first+9]);
            const glm::vec2 uv1(vertices[first+18], vertices[first+19]);
            const glm::vec2 uv2(vertices[first+28], vertices[first+29]);
            hitUV = (1.0f - u - v) * uv0 + u * uv1 + v * uv2;
            hit = true;
        }
    }
    if (!hit) return 0;
    const int slot = slotFromPaperUV(hitUV);
    if (slot >= 1 && slot <= 9 && !(sheet->requiredMask & (1u << (slot-1)))) return 0;
    return slot == 12 || (slot >= 1 && slot <= 9) ? slot : 0;
}

void clickPaperworkSlot(GameState& state, int slot) {
    if (state.isPaused || state.gameOver || state.shiftCompleted || state.paperSliding ||
        state.activePaperIndex >= state.paperSheets.size()) return;
    PaperSheet& sheet = state.paperSheets[state.activePaperIndex];
    if (sheet.stamped) return;
    if (slot >= 1 && slot <= 9) {
        const unsigned int bit = 1u << (slot - 1);
        if (sheet.requiredMask & bit) sheet.checkedMask ^= bit;
        return;
    }
    if (slot != 12) return;

    sheet.stamped = true;
    sheet.completedOnStamp = (sheet.checkedMask & sheet.requiredMask) == sheet.requiredMask;
    ++state.paperworkStampedCount;
    if (sheet.completedOnStamp) ++state.paperworkCorrectCount;
    else state.paperworkPenaltyFeedback = kPaperworkPenaltyFeedbackSeconds;
    state.paperworkAccuracy = static_cast<float>(state.paperworkCorrectCount) /
                              static_cast<float>(state.paperworkStampedCount);
    state.paperSliding = true;
    state.paperSlideProgress = 0.0f;
}

void updatePaperwork(GameState& state, float deltaTime) {
    if (deltaTime <= 0.0f || state.isPaused || state.gameOver || state.shiftCompleted) return;
    state.paperworkPenaltyFeedback = std::max(0.0f, state.paperworkPenaltyFeedback - deltaTime);
    if (!state.paperSliding) return;
    state.paperSlideProgress = std::min(1.0f, state.paperSlideProgress +
                                            deltaTime / kPaperworkSlideSeconds);
    if (state.paperSlideProgress < 1.0f) return;
    state.paperSliding = false;
    ++state.activePaperIndex;
    const PaperSheet* next = currentPaperSheet(state);
    state.paperworkActive = next != nullptr;
    state.selectedPaperTexture = next ? next->textureIndex : -1;
}
