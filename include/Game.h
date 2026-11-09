#pragma once

#include "GameState.h"

#include <cstddef>

struct GLFWwindow;
struct CameraState;

// The game functions update timers, tasks, and the clipboard state.
void updateGame(GameState& gameState, GLFWwindow* window, float deltaTime, double currentTime);
void updateWindowTitle(GLFWwindow* window, const GameState& gameState);
void openClipboard(GameState& gameState, CameraState& camera, std::size_t paperTextureCount);
void closeClipboard(GameState& gameState);
