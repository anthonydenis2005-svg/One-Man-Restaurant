#include "Game.h"
#include "Camera.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cstdio>
#include <random>
#include <iostream>

// Helper function to manage customer happiness drain and game over state <3
void applyHappinessPenalty(GameState& state, float deltaTime, float drainRate) {
    state.customerHappiness -= drainRate * deltaTime;
    if (state.customerHappiness <= 0.0f) {
        state.customerHappiness = 0.0f;
        state.gameOver = true;
        std::cout << "[GAME OVER] Customer happiness dropped to 0%! Restaurant failed.\n";
    }
}

// Start raising the clipboard when the player clicks its bounding box.
void openClipboard(GameState& gameState, CameraState& camera, std::size_t paperTextureCount) {
    if (gameState.clipboardViewState != ClipboardViewState::OnDesk) return;
    // Lock camera movement so the clipboard stays in the same view while it is open.
    resetCameraDirection(camera);
    camera.locked = true;
    gameState.clipboardViewState = ClipboardViewState::Raising;
    gameState.clipboardProgress = 0.0f;
    // Pick one of the PNGs from the Clipboard texture folder each time it opens.
    if (paperTextureCount > 0) {
        static std::mt19937 generator(std::random_device{}());
        std::uniform_int_distribution<int> distribution(0, static_cast<int>(paperTextureCount) - 1);
        gameState.selectedPaperTexture = distribution(generator);
    }
}

// This starts the reverse animation instead of moving the clipboard instantly.
void closeClipboard(GameState& gameState) {
    if (gameState.clipboardViewState == ClipboardViewState::Raising ||
        gameState.clipboardViewState == ClipboardViewState::Raised) {
        gameState.clipboardViewState = ClipboardViewState::Lowering;
    }
}

// Update the clipboard animation and the restaurant events every frame.
void updateGame(GameState& gameState, GLFWwindow* window, float deltaTime, double currentTime) {
        // Raising takes 0.28 seconds and lowering takes 0.24 seconds.
        if (gameState.clipboardViewState == ClipboardViewState::Raising) {
            gameState.clipboardProgress = std::min(1.0f, gameState.clipboardProgress + deltaTime / 0.28f);
            if (gameState.clipboardProgress >= 1.0f)
                gameState.clipboardViewState = ClipboardViewState::Raised;
        } else if (gameState.clipboardViewState == ClipboardViewState::Lowering) {
            gameState.clipboardProgress = std::max(0.0f, gameState.clipboardProgress - deltaTime / 0.24f);
            if (gameState.clipboardProgress <= 0.0f)
                gameState.clipboardViewState = ClipboardViewState::OnDesk;
        }

        // These timers and keyboard tasks run while the game has not ended.
        if (!gameState.gameOver) {
            if (gameState.shiftTimer > 0.0f) {
                gameState.shiftTimer -= deltaTime;
                if (gameState.shiftTimer <= 0.0f) {
                    gameState.shiftTimer = 0.0f;
                    std::cout << "[VICTORY] Shift completed successfully!\n";
                }
            }

            // --- FEATURE #1:Random alamr trigger and key resolution using "E"
            // Check for a possible new alarm after 15 seconds without one being active.
            gameState.alarmTimer += deltaTime;
            if (!gameState.alarmActive && gameState.alarmTimer > 15.0f) {
                if (static_cast<int>(currentTime) % 2 == 0) {
                    gameState.alarmActive = true;
                    gameState.alarmDuration = 0.0f; // Reset duration when new alarm hits
                    std::cout << "[ALARM] Fire alarm triggered! Press 'E' to turn it off.\n"; //until we get the button we will just be pressing E :3
                }
                gameState.alarmTimer = 0.0f;
            }

            // --- RANDOM PAPERWORK EVENT TRIGGER (ADDED) ---
            gameState.paperworkTimer += deltaTime;
            // Will trigger a new paperwork batch roughly every 22 seconds if no active paperwork is pending
            if (!gameState.paperworkActive && gameState.paperworkTimer > 22.0f) {
                gameState.paperworkActive = true;
                gameState.paperworkDuration = 0.0f;
                gameState.payrollCompleted = false;
                gameState.safetyCheckCompleted = false;
                gameState.inventoryChecked = false;
                gameState.paperworkTimer = 0.0f;
                std::cout << "[PAPERWORK] New paperwork tasks arrived! Press 1, 2, and 3 to clear them.\n";
            }

            // --- FEATURE #2: PAPERWORK CHECKBOX INPUT LOGIC ---
            if (gameState.paperworkActive) {

                gameState.paperworkDuration += deltaTime;
                if (gameState.paperworkDuration > 10.0f) {
                    applyHappinessPenalty(gameState, deltaTime, 1.5f); // Drains 1.5% happiness per second
                }

                if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                    if (!gameState.payrollCompleted) {
                        gameState.payrollCompleted = true;
                        std::cout << "[PAPERWORK] Employee Payroll completed!\n";
                    }
                }
                if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                    if (!gameState.safetyCheckCompleted) {
                        gameState.safetyCheckCompleted = true;
                        std::cout << "[PAPERWORK] Safety Check completed!\n";
                    }
                }
                if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
                    if (!gameState.inventoryChecked) {
                        gameState.inventoryChecked = true;
                        std::cout << "[PAPERWORK] Inventory Check completed!\n";
                    }
                }
                // The current paperwork batch is cleared only after all three tasks are done.
                if (gameState.payrollCompleted && gameState.safetyCheckCompleted && gameState.inventoryChecked) {
                    gameState.paperworkActive = false;
                    std::cout << "[PAPERWORK] All tasks completed successfully! Task cleared.\n";
                }
            }

            // If alarm is active, track duration. If past 10 seconds, drain happiness!
            if (gameState.alarmActive) {
                gameState.alarmDuration += deltaTime;
                if (gameState.alarmDuration > 10.0f) {
                    applyHappinessPenalty(gameState, deltaTime, 2.5f);
                }
            }

            // E currently turns the alarm off directly; it is not tied to a 3D button yet.
            if (gameState.alarmActive && glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
                gameState.alarmActive = false;
                gameState.alarmDuration = 0.0f;
                std::cout << "[ALARM] Fire alarm turned off.\n";
            }
        }
}

// The window title shows the shift time, happiness, pending tasks, and alarm warning.
void updateWindowTitle(GLFWwindow* window, const GameState& gameState) {
        // --- FEATURE #1: the window title with timer and happiness
        char titleBuffer[256];
        if (gameState.gameOver) {
            snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | GAME OVER: Customer Happiness Reached 0%!");
        }
        else {
            if (gameState.paperworkActive) {
                snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | Day %d | Time: %.1fs | Happy: %.1f%% | PW: %s%s%s %s",
                    gameState.currentDay, gameState.shiftTimer, gameState.customerHappiness,
                    gameState.payrollCompleted ? "[P]" : "[ ]",
                    gameState.safetyCheckCompleted ? "[S]" : "[ ]",
                    gameState.inventoryChecked ? "[I]" : "[ ]",
                    gameState.alarmActive ? "[FIRE ALARM!]" : "");
            }
            else {
                snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | Day %d | Time: %.1fs | Happy: %.1f%% | [Task Completed] %s",
                    gameState.currentDay, gameState.shiftTimer, gameState.customerHappiness,
                    gameState.alarmActive ? "[FIRE ALARM!]" : "");
            }
        }

        glfwSetWindowTitle(window, titleBuffer);
}
