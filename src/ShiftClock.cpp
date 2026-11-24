#include "ShiftClock.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

float getShiftClockHour(const GameState& gameState) {
    static const float originalShiftSeconds = GameState{}.shiftTimer;
    const float progress = originalShiftSeconds > 0.0f
        ? std::clamp(1.0f - gameState.shiftTimer / originalShiftSeconds, 0.0f, 1.0f)
        : 1.0f;
    return kShiftStartHour + (kShiftEndHour - kShiftStartHour) * progress;
}

glm::mat4 clockHandTransform(const Part& part, const GameState& gameState,
                              bool hourHand) {
    const float hoursSinceNine = getShiftClockHour(gameState) - kShiftStartHour;
    const float degrees = hourHand
        ? 90.0f - hoursSinceNine * 30.0f
        : -hoursSinceNine * 360.0f;
    return glm::translate(glm::mat4(1.0f), part.objectOrigin) *
           glm::rotate(glm::mat4(1.0f), glm::radians(degrees),
                       glm::vec3(0.0f, 0.0f, 1.0f)) *
           glm::translate(glm::mat4(1.0f), -part.objectOrigin);
}
