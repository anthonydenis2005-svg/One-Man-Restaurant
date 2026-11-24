#include "Game.h"
#include "Camera.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <iostream>

// Clamps happiness and triggers game over if it drops to 0 or below.
void finalizeHappinessCheck(GameState& state, const char* reason) {
    if (state.customerHappiness <= 0.0f) {
        state.customerHappiness = 0.0f;
        state.gameOver = true;
        if (reason) {
            std::cout << "[GAME OVER] " << reason << "\n";
        }
    }
}

// Continuous happiness drain over time (rate = happiness lost per second).
void applyHappinessDrain(GameState& state, float deltaTime, float drainPerSecond, const char* reason) {
    if (state.gameOver || drainPerSecond <= 0.0f) return;
    state.customerHappiness -= drainPerSecond * deltaTime;
    finalizeHappinessCheck(state, reason);
}

// Flat happiness change (negative delta = penalty) or immediate game over.
void applyHappinessDelta(GameState& state, float delta, bool instantGameOver, const char* reason) {
    if (state.gameOver) return;
    if (instantGameOver) {
        state.customerHappiness = 0.0f;
        state.gameOver = true;
        if (reason) {
            std::cout << "[GAME OVER] " << reason << "\n";
        }
        return;
    }
    state.customerHappiness = std::min(100.0f, state.customerHappiness + delta);
    finalizeHappinessCheck(state, reason);
}

// Applies -1% per customer served if the beer tap is off.
void onCustomerServed(GameState& state) {
    if (state.gameOver) return;
    if (state.customers.beerTapOff) {
        applyHappinessDelta(state, -1.0f, false, "Served customer with beer tap off!");
    }
}

// Evaluates active customer states, sprinklers, AC, breaker, and Day 4+ rush hour.
void updateCustomerStates(GameState& state, float deltaTime) {
    if (state.gameOver) return;

    CustomerState& c = state.customers;
    c.rushHourActive = (state.currentDay >= 4);


    // Fire alarm activates sprinklers if not already on.
    if (state.alarmActive) {
        c.sprinklersActive = true;
    }

    // Air conditioning penalty scales weakly-to-moderately with distance from comfortTemp.
    float tempDiff = std::abs(c.roomTemp - c.comfortTemp);
    if (tempDiff > 0.5f) {
        float acDrain = std::min(4.0f, tempDiff * 0.35f);
        applyHappinessDrain(state, deltaTime, acDrain, "Room temperature ruined satisfaction!");
    }

    // Soda Hog: drains soda over time unless sprinklers are active.
    if (c.sodaHogActive) {
        c.sodaHogTimer += deltaTime;
        if (!c.sprinklersActive) {
            float sodaDrain = c.rushHourActive ? 2.5f : 1.5f;
            applyHappinessDrain(state, deltaTime, sodaDrain, "Soda Hog drained too much soda!");
        }
    }

    // Smelly customer in line: -5%/sec until they leave, 0% if washed by sprinklers.
    if (c.smellyCustomerActive) {
        c.smellyCustomerTimer += deltaTime;
        if (!c.smellyCustomerWashed && c.sprinklersActive) {
            c.smellyCustomerWashed = true;
            std::cout << "[CUSTOMER] Sprinklers washed the smelly customer!\n";
        }
        if (!c.smellyCustomerWashed) {
            applyHappinessDrain(state, deltaTime, 5.0f, "Smelly customer in line tanked happiness!");
        }
        if (c.smellyCustomerTimer >= 12.0f) {
            c.smellyCustomerActive = false;
            c.smellyCustomerWashed = false;
            c.smellyCustomerTimer = 0.0f;
        }
    }

    // Kid with fork in outlet: counts down; if he dies before breaker is turned off, -100% / game over.
    if (c.kidWithForkActive) {
        c.kidWithForkTimer += deltaTime;
        if (c.breakerOff) {
            c.kidWithForkActive = false;
            c.kidWithForkTimer = 0.0f;
            std::cout << "[SAFE] Breaker turned off in time—kid with fork stopped!\n";
        }
        else if (c.kidWithForkTimer >= 8.0f) {
            c.kidWithForkActive = false;
            applyHappinessDelta(state, -100.0f, true, "Kid with fork in outlet was harmed (-100%)!");
        }
    }

    // Old man throwing teeth: instant -5% hit when triggered.
    if (c.oldManTeethTriggered) {
        c.oldManTeethTriggered = false;
        applyHappinessDelta(state, -5.0f, false, "Old man threw his teeth (-5%)!");
    }
}


// Function to toggle the fire alarm state and reset its duration, printing a message to the console. (YAY)
//Flips the state.alarmActive (switching it on or off) 
// and resets the state.alarmDuration to 0.0f. 
// It also prints a message to the console indicating whether the fire alarm has been triggered or turned off.
void toggleAlarm(GameState& state) {
    state.alarmActive = !state.alarmActive;
    state.alarmDuration = 0.0f;
    if (state.alarmActive) {
        std::cout << "[ALARM] Fire alarm triggered!\n";
    }
    else {
        std::cout << "[ALARM] Fire alarm turned off.\n";
    }
}

// Funtion to handle the completion of individual paperwork tasks and update the game state accordingly.
void completePaperworkTask(GameState& state, int taskIndex) {
    if (taskIndex == 1 && !state.payrollCompleted) {
        state.payrollCompleted = true;
        std::cout << "[PAPERWORK] Employee Payroll completed!\n";
    }
    else if (taskIndex == 2 && !state.safetyCheckCompleted) {
        state.safetyCheckCompleted = true;
        std::cout << "[PAPERWORK] Safety Check completed!\n";
    }
    else if (taskIndex == 3 && !state.inventoryChecked) {
        state.inventoryChecked = true;
        std::cout << "[PAPERWORK] Inventory Check completed!\n";
    }

    // Clear batch if all done
    if (state.payrollCompleted && state.safetyCheckCompleted && state.inventoryChecked) {
        state.paperworkActive = false;
        std::cout << "[PAPERWORK] All tasks completed successfully! Task cleared.\n";
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
			// Check for a possible new alarm after 15 seconds without one being active. added function utilization for toggling the alarm state and resetting its duration
            gameState.alarmTimer += deltaTime;
            if (!gameState.alarmActive && gameState.alarmTimer > 15.0f) {
                if (static_cast<int>(currentTime) % 2 == 0) {
                    toggleAlarm(gameState);
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
         // Paperwork task shortcuts (1, 2, 3)
        if (gameState.paperworkActive) {
            gameState.paperworkDuration += deltaTime;
            if (gameState.paperworkDuration > 10.0f) {
                applyHappinessDrain(gameState, deltaTime, 1.5f);
            }

            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) completePaperworkTask(gameState, 1);
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) completePaperworkTask(gameState, 2);
            if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) completePaperworkTask(gameState, 3);
        }

        if (gameState.alarmActive) {
            gameState.alarmDuration += deltaTime;
            if (gameState.alarmDuration > 10.0f) {
                applyHappinessDrain(gameState, deltaTime, 2.5f);
            }
        }

        // Run customer & restaurant conditions while the shift is active
        updateCustomerStates(gameState, deltaTime);

        // Toggle alarm off using 'E' key calling the modular function
        static bool eKeyWasPressed = false;
        bool eKeyIsPressed = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS);
        if (eKeyIsPressed && !eKeyWasPressed && gameState.alarmActive) {
            toggleAlarm(gameState);
        }
        eKeyWasPressed = eKeyIsPressed;
    }
}

// The window title shows the shift time, happiness, pending tasks, and alarm warning.
void updateWindowTitle(GLFWwindow* window, const GameState& gameState) {
        // --- FEATURE #1: the window title with timer and happiness
        char titleBuffer[256];
        if (gameState.isPaused) {
            snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | PAUSED - Press [P] to Resume");
        }
        else if (gameState.gameOver) {
            snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | GAME OVER: Customer Happiness Reached 0%%! Press [R] to Restart");
        }
        else {
            if (gameState.paperworkActive) {
                snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | Day %d | Time: %.1fs | Happy: %.1f%% | PW: %s%s%s %s",
                    gameState.currentDay, gameState.shiftTimer, gameState.customerHappiness,
                    gameState.payrollCompleted ? "[P]" : "[ ]",
                    gameState.safetyCheckCompleted ? "[S]" : "[ ]",
                    gameState.inventoryChecked ? "[I]" : "[ ]",
                    gameState.alarmActive ? "[FIRE ALARM! - Press E]" : "");
            }
            else {
                snprintf(titleBuffer, sizeof(titleBuffer), "One Man Restaurant | Day %d | Time: %.1fs | Happy: %.1f%% | [Task Completed] %s",
                    gameState.currentDay, gameState.shiftTimer, gameState.customerHappiness,
                    gameState.alarmActive ? "[FIRE ALARM! - Press E]" : "");
            }
        }

        glfwSetWindowTitle(window, titleBuffer);
}

void resetGame(GameState& state) {
    // Reset core stats
    state.customerHappiness = 100.0f;
    state.shiftTimer = 180.0f;
    state.gameOver = false;
    state.isPaused = false;

    // Reset alarm state completely
    state.alarmActive = false;
    state.alarmTimer = 0.0f;
    state.alarmDuration = 0.0f;

    // Reset paperwork and task states
    state.paperworkActive = false;
    state.paperworkTimer = 0.0f;
    state.paperworkDuration = 0.0f;
    state.payrollCompleted = true;       // Match initial safe state
    state.safetyCheckCompleted = true;   // Match initial safe state
    state.inventoryChecked = true;       // Match initial safe state
    state.timeSinceLastPaperworkWarning = 0.0f;

    // Reset customer & restaurant states
    state.customers = CustomerState{};

    // Reset clipboard view back to desk
    state.clipboardViewState = ClipboardViewState::OnDesk;
    state.clipboardProgress = 0.0f;
    state.selectedPaperTexture = -1;

    std::cout << "[GAME] Game state fully reset, all tasks cleared.\n";
}