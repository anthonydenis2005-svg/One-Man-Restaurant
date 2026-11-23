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
void toggleAlarm(GameState& state);
void completePaperworkTask(GameState& state, int taskIndex);
void resetGame(GameState& state);

// Happiness & Phase 2 customer/restaurant state helpers.
void finalizeHappinessCheck(GameState& state, const char* reason = nullptr);
void applyHappinessDrain(GameState& state, float deltaTime, float drainPerSecond, const char* reason = nullptr);
void applyHappinessDelta(GameState& state, float delta, bool instantGameOver = false, const char* reason = nullptr);
void onCustomerServed(GameState& state);
void updateCustomerStates(GameState& state, float deltaTime);