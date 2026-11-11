#pragma once

enum class ClipboardViewState {
    OnDesk, Raising, Raised, Lowering
};

struct GameState {
    int currentDay = 1;
    float shiftTimer = 180.0f;
    float customerHappiness = 100.0f;
    bool isPaused = false;
    bool shiftCompleted = false;

    bool paperworkActive = false;
    float paperworkTimer = 0.0f;
    float paperworkDuration = 0.0f;
    bool payrollCompleted = true;
    bool safetyCheckCompleted = true;
    bool inventoryChecked = true;

    bool alarmActive = false;
    float alarmTimer = 0.0f;
    float alarmDuration = 0.0f;
    bool gameOver = false;
    float timeSinceLastPaperworkWarning = 0.0f;

    ClipboardViewState clipboardViewState = ClipboardViewState::OnDesk;
    float clipboardProgress = 0.0f;
    int selectedPaperTexture = -1;
};
