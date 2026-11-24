#include "Game.h"
#include "Camera.h"
#include "Paperwork.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {

void applyHappinessPenalty(GameState& state, float deltaTime, float drainRate) {
    if (state.gameOver || state.shiftCompleted) return;
    state.customerHappiness = std::max(0.0f, state.customerHappiness - drainRate * deltaTime);
    if (state.customerHappiness <= 0.0f) {
        state.gameOver = true;
        std::cout << "[GAME OVER] Customer happiness dropped to 0%! Restaurant failed.\n";
    }
}

}

// Function to toggle the fire alarm state and reset its duration, printing a message to the console. (YAY)
//Flips the state.alarmActive (switching it on or off)
// and resets the state.alarmDuration to 0.0f.
// It also prints a message to the console indicating whether the fire alarm has been triggered or turned off.
void toggleAlarm(GameState& state) {
    if (state.gameOver || state.shiftCompleted) return;
    state.alarmActive = !state.alarmActive;
    state.alarmDuration = 0.0f;
    state.alarmTimer = 0.0f;
    std::cout << (state.alarmActive
        ? "[ALARM] Fire alarm triggered! Press E or click the fire alarm to turn it off.\n"
        : "[ALARM] Fire alarm turned off.\n");
}

// Start raising the clipboard when the player clicks its bounding box.
void openClipboard(GameState& state, CameraState& camera, std::size_t paperTextureCount) {
    if (state.clipboardViewState != ClipboardViewState::OnDesk) return;
    // Lock camera movement so the clipboard stays in the same view while it is open.
    resetCameraDirection(camera);
    camera.locked = true;
    state.clipboardViewState = ClipboardViewState::Raising;
    state.clipboardProgress = 0.0f;
    const PaperSheet* sheet = currentPaperSheet(state);
    state.selectedPaperTexture = sheet ? sheet->textureIndex : -1;
    (void)paperTextureCount;
}

// This starts the reverse animation instead of moving the clipboard instantly.
void closeClipboard(GameState& state) {
    if (state.clipboardViewState == ClipboardViewState::Raising ||
        state.clipboardViewState == ClipboardViewState::Raised) {
        state.clipboardViewState = ClipboardViewState::Lowering;
    }
}

// Update the clipboard animation and the restaurant events every frame.
void updateGame(GameState& state, GLFWwindow* window, float deltaTime, double currentTime) {
    // Raising takes 0.28 seconds and lowering takes 0.24 seconds.
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

    if (state.shiftTimer > 0.0f) {
        state.shiftTimer -= deltaTime;
        if (state.shiftTimer <= 0.0f) {
            state.shiftTimer = 0.0f;
            state.shiftCompleted = true;
            std::cout << "[VICTORY] Shift completed successfully!\n";
            return;
        }
    }

    // --- FEATURE #1:Random alamr trigger and key resolution using "E"
    // Check for a possible new alarm after 15 seconds without one being active. added function utilization for toggling the alarm state and resetting its duration
    if (!state.alarmActive) {
        state.alarmTimer += deltaTime;
        if (state.alarmTimer > 15.0f) {
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


    // Toggle alarm off using 'E' key calling the modular function
    static bool priorE = false;
    const bool keyE = glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
    if (keyE && !priorE && state.alarmActive) toggleAlarm(state);
    priorE = keyE;
}

// The window title shows the shift time, happiness, pending tasks, and alarm warning.
void updateWindowTitle(GLFWwindow* window, const GameState& state) {
    // --- FEATURE #1: the window title with timer and happiness
    char title[256];
    if (state.isPaused) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | PAUSED - Press P to Resume");
    } else if (state.shiftCompleted) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | SHIFT COMPLETE! Press R to Restart");
    } else if (state.gameOver) {
        std::snprintf(title, sizeof(title), "One Man Restaurant | GAME OVER! Press R to Restart");
    } else {
        std::snprintf(title, sizeof(title),
            "One Man Restaurant | Day %d | Time %.1fs | Happy %.1f%% | Papers %zu/%zu %s",
            state.currentDay, state.shiftTimer, state.customerHappiness,
            state.activePaperIndex, state.paperSheets.size(),
            state.alarmActive ? "[FIRE ALARM - E / Click]" : "");
    }
    glfwSetWindowTitle(window, title);
}

void resetGame(GameState& state) {
    const auto assignedSheets = state.paperSheets;
    state = GameState{};
    state.paperSheets = assignedSheets;
    restartDailyPaperwork(state);
    std::cout << "[GAME] Game state fully reset, all tasks cleared.\n";
}
