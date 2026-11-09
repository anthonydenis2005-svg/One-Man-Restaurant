#pragma once

// The clipboard can be on the desk, moving up, fully raised, or moving back down.
enum class ClipboardViewState {
    OnDesk, Raising, Raised, Lowering
};

// Adding game state structure
struct GameState {

    int currentDay = 1;                 // set to one for now but we will change this when the difficulty increases in the future
    float shiftTimer = 180.0f;          // for now we will have a 3-minute "shift" (180 seconds)
    float customerHappiness = 100.0f;   // Starts at 100% going down over time or due to mistakes

    // Paperwork event state and individual task flags (for next features Phase2 :> to be advanced)
    bool paperworkActive = false;
    float paperworkTimer = 0.0f;
    float paperworkDuration = 0.0f;
    bool payrollCompleted = true;      // Starts true so menu is hidden initially and doesnt start immediately with a penalty
    bool safetyCheckCompleted = true;
    bool inventoryChecked = true;

    // Alarm state & timer for random triggers
    bool alarmActive = false;
    float alarmTimer = 0.0f;
    float alarmDuration = 0.0f;         // Tracks how long current alarm has been active so we can penalize satisfaction state
    bool gameOver = false;
    float timeSinceLastPaperworkWarning = 0.0f;

    // These values keep track of the clipboard animation and which paper image is shown.
    ClipboardViewState clipboardViewState = ClipboardViewState::OnDesk;
    float clipboardProgress = 0.0f;
    int selectedPaperTexture = -1;
};
