#include "Game.h"
#include "Camera.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <random>

namespace {

// Only the active simulation can lose happiness.
void applyHappinessPenalty(GameState& state, float deltaTime, float drainRate) {
    if (state.gameOver || state.shiftCompleted) return;
    state.customerHappiness = std::max(0.0f, state.customerHappiness - drainRate * deltaTime);
    if (state.customerHappiness <= 0.0f) {
        state.gameOver = true;
        std::cout << "[GAME OVER] Customer happiness dropped to 0%! Restaurant failed.\n";
    }
}

} // namespace

// The alarm created by the timed event is the same alarm that the player clears
// by clicking the FireAlarmBox in the 3D office or pressing E.
void toggleAlarm(GameState& state) {
    if (state.gameOver || state.shiftCompleted) return;
    state.alarmActive = !state.alarmActive;
    state.alarmDuration = 0.0f;
    state.alarmTimer = 0.0f; // Always wait for another alarm cycle after clearing one.
    std::cout << (state.alarmActive
        ? "[ALARM] Fire alarm triggered! Press E or click the fire alarm to turn it off.\n"
        : "[ALARM] Fire alarm turned off.\n");
}

void completePaperworkTask(GameState& state, int taskIndex) {
    if (!state.paperworkActive || state.gameOver || state.shiftCompleted) return;

    if (taskIndex == 1 && !state.payrollCompleted) {
        state.payrollCompleted = true;
        std::cout << "[PAPERWORK] Employee Payroll completed!\n";
    } else if (taskIndex == 2 && !state.safetyCheckCompleted) {
        state.safetyCheckCompleted = true;
        std::cout << "[PAPERWORK] Safety Check completed!\n";
    } else if (taskIndex == 3 && !state.inventoryChecked) {
        state.inventoryChecked = true;
        std::cout << "[PAPERWORK] Inventory Check completed!\n";
    }

    if (state.payrollCompleted && state.safetyCheckCompleted && state.inventoryChecked) {
        state.paperworkActive = false;
        state.paperworkDuration = 0.0f;
        state.paperworkTimer = 0.0f;
        std::cout << "[PAPERWORK] All tasks completed successfully! Task cleared.\n";
    }
}

void openClipboard(GameState& state, CameraState& camera, std::size_t paperTextureCount) {
    if (state.clipboardViewState != ClipboardViewState::OnDesk) return;
    resetCameraDirection(camera);
    camera.locked = true;
    state.clipboardViewState = ClipboardViewState::Raising;
    state.clipboardProgress = 0.0f;
    state.selectedPaperTexture = -1;
    if (paperTextureCount > 0) {
        static std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(paperTextureCount) - 1);
        state.selectedPaperTexture = distribution(generator);
    }
}

void closeClipboard(GameState& state) {
    if (state.clipboardViewState == ClipboardViewState::Raising ||
        state.clipboardViewState == ClipboardViewState::Raised) {
        state.clipboardViewState = ClipboardViewState::Lowering;
    }
}

void updateGame(GameState& state, GLFWwindow* window, float deltaTime, double currentTime) {
    // Clipboard animation uses the 3D transform in Renderer.cpp.
    if (state.clipboardViewState == ClipboardViewState::Raising) {
        state.clipboardProgress = std::min(1.0f, state.clipboardProgress + deltaTime / 0.28f);
        if (state.clipboardProgress >= 1.0f)
            state.clipboardViewState = ClipboardViewState::Raised;
    } else if (state.clipboardViewState == ClipboardViewState::Lowering) {
        state.clipboardProgress = std::max(0.0f, state.clipboardProgress - deltaTime / 0.24f);
        if (state.clipboardProgress <= 0.0f)
            state.clipboardViewState = ClipboardViewState::OnDesk;
    }

    if (state.gameOver || state.shiftCompleted || state.isPaused) return;

    // Reaching the end of the shift is a terminal win state.
    if (state.shiftTimer > 0.0f) {
        state.shiftTimer -= deltaTime;
        if (state.shiftTimer <= 0.0f) {
            state.shiftTimer = 0.0f;
            state.shiftCompleted = true;
            std::cout << "[VICTORY] Shift completed successfully!\n";
            return;
        }
    }

    // Timed fire alarm. Timer does not build up while the alarm is already active.
    if (!state.alarmActive) {
        state.alarmTimer += deltaTime;
        if (state.alarmTimer > 15.0f) {
            // Keep Jaylena's original approximate 50% trigger check each cycle.
            if (static_cast<int>(currentTime) % 2 == 0) {
                toggleAlarm(state);
            } else {
                state.alarmTimer = 0.0f;
            }
        }
    } else {
        state.alarmDuration += deltaTime;
        if (state.alarmDuration > 10.0f)
            applyHappinessPenalty(state, deltaTime, 2.5f);
    }

    // Timed paperwork. A new batch starts only after the previous one was cleared.
    if (!state.paperworkActive) {
        state.paperworkTimer += deltaTime;
        if (state.paperworkTimer > 22.0f) {
            state.paperworkActive = true;
            state.paperworkDuration = 0.0f;
            state.payrollCompleted = false;
            state.safetyCheckCompleted = false;
            state.inventoryChecked = false;
            state.paperworkTimer = 0.0f;
            std::cout << "[PAPERWORK] New tasks arrived! Press 1, 2, and 3.\n";
        }
    } else {
        state.paperworkDuration += deltaTime;
        if (state.paperworkDuration > 10.0f)
            applyHappinessPenalty(state, deltaTime, 1.5f);
    }

    // Detect fresh key presses, not a key held for multiple frames.
    static bool prior1 = false, prior2 = false, prior3 = false, priorE = false;
    const bool key1 = glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS;
    const bool key2 = glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS;
    const bool key3 = glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS;
    const bool keyE = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;

    if (state.paperworkActive) {
        if (key1 && !prior1) completePaperworkTask(state, 1);
        if (key2 && !prior2) completePaperworkTask(state, 2);
        if (key3 && !prior3) completePaperworkTask(state, 3);
    }
    if (keyE && !priorE && state.alarmActive) toggleAlarm(state);

    prior1 = key1;
    prior2 = key2;
    prior3 = key3;
    priorE = keyE;
}

void updateWindowTitle(GLFWwindow* window, const GameState& state) {
    char title[256];
    if (state.isPaused) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | PAUSED - Press P to Resume");
    } else if (state.shiftCompleted) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | SHIFT COMPLETE! Press R to Restart");
    } else if (state.gameOver) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | GAME OVER! Press R to Restart");
    } else {
        std::snprintf(title, sizeof(title),
            "One Man Restaurant | Day %d | Time %.1fs | Happy %.1f%% | PW %s%s%s %s",
            state.currentDay, state.shiftTimer, state.customerHappiness,
            state.paperworkActive ? (state.payrollCompleted ? "[P]" : "[ ]") : "",
            state.paperworkActive ? (state.safetyCheckCompleted ? "[S]" : "[ ]") : "",
            state.paperworkActive ? (state.inventoryChecked ? "[I]" : "[ ]") : "",
            state.alarmActive ? "[FIRE ALARM - E / Click]" : "");
    }
    glfwSetWindowTitle(window, title);
}

void resetGame(GameState& state) {
    state = GameState{};
    std::cout << "[GAME] Game state fully reset, all tasks cleared.\n";
}
