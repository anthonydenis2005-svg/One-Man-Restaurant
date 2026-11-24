#pragma once

#include "GameState.h"

enum class SoundId {
    OfficeAmbience,   // Looping low-volume background office audio
    FireAlarm,        // Looping alarm when state.alarmActive == true
    Sprinklers,       // Looping water sound when sprinklersActive == true
    PaperworkTask,    // One-shot click/stamp on [1], [2], [3]
    ClipboardMove,    // One-shot raise/lower sound
    HappinessPenalty, // One-shot alert on flat happiness drop
    GameOver,         // One-shot fail sound
    Count
};

struct AudioEngine; // Forward-declared so miniaudio stays isolated in Audio.cpp

struct AudioState {
    AudioEngine* engine = nullptr;
    bool initialized = false;

    float masterVolume = 0.8f;
    float officeMusicVolume = 0.5f; // Low background volume by default
    float sfxVolume = 0.65f;
};

bool initializeAudio(AudioState& audio);
void shutdownAudio(AudioState& audio);

// Trigger overlapping one-shot sound effects from anywhere (Game.cpp, main.cpp, etc.)
void playSound(AudioState& audio, SoundId id);

// Automatically manages looping sounds (office music, fire alarm, sprinklers) & pause state
void syncLoopingAudio(AudioState& audio, const GameState& gameState);

// Optional quick volume adjustment (e.g., - / = keys)
void adjustMasterVolume(AudioState& audio, float delta);