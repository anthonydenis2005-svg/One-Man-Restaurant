#pragma once

// The clipboard can be on the desk, moving up, fully raised, or moving back down.
enum class ClipboardViewState {
    OnDesk, Raising, Raised, Lowering
};

// Active customer incidents and restaurant condition toggles
struct CustomerState {
    // Annoying / special customers
    bool sodaHogActive = false;         // Drains soda; sprinklers stop it
    float sodaHogTimer = 0.0f;

    bool trenchcoatKidsActive = false;  // 2 kids asking for 1 alcohol; -15% if alcohol sold
    float trenchcoatKidsTimer = 0.0f;

    bool smellyCustomerActive = false;  // -5%/sec until leaves, 0% if washed by sprinklers
    bool smellyCustomerWashed = false;
    float smellyCustomerTimer = 0.0f;

    bool kidWithForkActive = false;     // Kid with fork in outlet; -100% / game over if he dies
    float kidWithForkTimer = 0.0f;

    bool oldManTeethTriggered = false;  // One-shot -5% when triggered

    // Restaurant conditions & Day 4+ rush hour
    bool rushHourActive = false;        // Enabled starting on Day 4
    bool sprinklersActive = false;      // Fire alarm or manual toggle
    bool breakerOff = false;            // Off = safe from fork/outlet or breaker drain depending on rule
    bool beerTapOff = false;            // -1% per customer served while off
    float roomTemp = 72.0f;
    float comfortTemp = 72.0f;
};

// Adding game state structure
struct GameState {

    int currentDay = 1;                 // set to one for now but we will change this when the difficulty increases in the future
    float shiftTimer = 180.0f;          // for now we will have a 3-minute "shift" (180 seconds)
    float customerHappiness = 100.0f;   // Starts at 100% going down over time or due to mistakes
    bool isPaused = false;              // Indicates if the game is paused

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
	bool gameOver = false;              // Indicates if the game has ended due to customer happiness reaching 0%
    float timeSinceLastPaperworkWarning = 0.0f;

    // Customer & restaurant state for Phase 2
    CustomerState customers;

    // These values keep track of the clipboard animation and which paper image is shown.
    ClipboardViewState clipboardViewState = ClipboardViewState::OnDesk;
    float clipboardProgress = 0.0f;
    int selectedPaperTexture = -1;
};
