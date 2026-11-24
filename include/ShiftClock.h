#pragma once

#include "GameState.h"
#include "ModelLoader.h"

#include <glm/glm.hpp>

constexpr float kShiftStartHour = 9.0f;
constexpr float kShiftEndHour = 17.0f;

float getShiftClockHour(const GameState& gameState);

glm::mat4 clockHandTransform(const Part& part, const GameState& gameState,
                              bool hourHand);
