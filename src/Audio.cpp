#include "Audio.h"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>

namespace {
    constexpr std::size_t kSoundCount = static_cast<std::size_t>(SoundId::Count);

    struct SoundConfig {
        const char* path;
        bool looping;
        bool isMusic;
    };

    // Add future station/customer audio files here as you expand SoundId in Audio.h
    constexpr std::array<SoundConfig, kSoundCount> kSoundConfigs = { {
        { "assets/audio/office_ambience.mp3",   true,  true  }, // OfficeAmbience
        { "assets/audio/fire_alarm.mp3",        true,  false }, // FireAlarm
        { "assets/audio/sprinklers.mp3",        true,  false }, // Sprinklers
        { "assets/audio/paperwork_task.mp3",    false, false }, // PaperworkTask
        { "assets/audio/clipboard_move.mp3",    false, false }, // ClipboardMove
        { "assets/audio/happiness_penalty.mp3", false, false }, // HappinessPenalty
        { "assets/audio/game_over.mp3",         false, false }  // GameOver
    } };

    void setLoopPlaying(ma_sound& sound, bool loaded, bool shouldPlay, float volume) {
        if (!loaded) return;

        ma_sound_set_volume(&sound, volume);
        ma_bool32 isPlaying = ma_sound_is_playing(&sound);

        if (shouldPlay && !isPlaying) {
            ma_sound_start(&sound);
        }
        else if (!shouldPlay && isPlaying) {
            ma_sound_stop(&sound);
        }
    }
}

struct AudioEngine {
    ma_engine engine{};
    std::array<ma_sound, kSoundCount> loopSounds{};
    std::array<bool, kSoundCount> loopLoaded{};
    bool wasPaused = false;
};

bool initializeAudio(AudioState& audio) {
    if (audio.initialized) return true;

    audio.engine = new AudioEngine();
    if (ma_engine_init(nullptr, &audio.engine->engine) != MA_SUCCESS) {
        std::cerr << "[AUDIO] Failed to initialize miniaudio engine.\n";
        delete audio.engine;
        audio.engine = nullptr;
        return false;
    }

    ma_engine_set_volume(&audio.engine->engine, audio.masterVolume);

    // Pre-load looping sounds if their files exist on disk
    for (std::size_t i = 0; i < kSoundCount; ++i) {
        if (!kSoundConfigs[i].looping) continue;

        const char* path = kSoundConfigs[i].path;
        if (!std::filesystem::exists(path)) {
            std::cout << "[AUDIO] Optional loop file not found yet (skipping): " << path << "\n";
            continue;
        }

        ma_result res = ma_sound_init_from_file(
            &audio.engine->engine,
            path,
            MA_SOUND_FLAG_DECODE,
            nullptr,
            nullptr,
            &audio.engine->loopSounds[i]
        );

        if (res == MA_SUCCESS) {
            ma_sound_set_looping(&audio.engine->loopSounds[i], MA_TRUE);
            audio.engine->loopLoaded[i] = true;
            std::cout << "[AUDIO] Loaded: " << path << "\n"; // CHANGED: confirms the file loaded
        }
        else {
            std::cerr << "[AUDIO] Could not load loop sound: " << path << "\n";
        }
    }

    audio.initialized = true;
    std::cout << "[AUDIO] Audio engine initialized.\n";
    std::cout << "[AUDIO] Running from: " << std::filesystem::current_path() << "\n"; // CHANGED: shows the working folder
    return true;
}

void shutdownAudio(AudioState& audio) {
    if (!audio.initialized || !audio.engine) return;

    for (std::size_t i = 0; i < kSoundCount; ++i) {
        if (audio.engine->loopLoaded[i]) {
            ma_sound_uninit(&audio.engine->loopSounds[i]);
            audio.engine->loopLoaded[i] = false;
        }
    }

    ma_engine_uninit(&audio.engine->engine);
    delete audio.engine;
    audio.engine = nullptr;
    audio.initialized = false;
}

void playSound(AudioState& audio, SoundId id) {
    if (!audio.initialized || !audio.engine) return;

    const std::size_t index = static_cast<std::size_t>(id);
    if (index >= kSoundCount) return;

    const char* path = kSoundConfigs[index].path;
    if (!std::filesystem::exists(path)) return;

    // ma_engine_play_sound fires overlapping one-shots cleanly without blocking
    ma_engine_play_sound(&audio.engine->engine, path, nullptr);
}

void syncLoopingAudio(AudioState& audio, const GameState& gameState) {
    if (!audio.initialized || !audio.engine) return;

    ma_engine_set_volume(&audio.engine->engine, audio.masterVolume);

    // Pause/resume the entire audio engine cleanly when the game is paused
    if (gameState.isPaused && !audio.engine->wasPaused) {
        ma_engine_stop(&audio.engine->engine);
        audio.engine->wasPaused = true;
        return;
    }
    else if (!gameState.isPaused && audio.engine->wasPaused) {
        ma_engine_start(&audio.engine->engine);
        audio.engine->wasPaused = false;
    }

    if (gameState.isPaused) return;

    const bool activeShift = !gameState.gameOver;

    setLoopPlaying(
        audio.engine->loopSounds[static_cast<std::size_t>(SoundId::OfficeAmbience)],
        audio.engine->loopLoaded[static_cast<std::size_t>(SoundId::OfficeAmbience)],
        activeShift,
        audio.officeMusicVolume
    );

    setLoopPlaying(
        audio.engine->loopSounds[static_cast<std::size_t>(SoundId::FireAlarm)],
        audio.engine->loopLoaded[static_cast<std::size_t>(SoundId::FireAlarm)],
        activeShift && gameState.alarmActive,
        audio.sfxVolume
    );

    setLoopPlaying(
        audio.engine->loopSounds[static_cast<std::size_t>(SoundId::Sprinklers)],
        audio.engine->loopLoaded[static_cast<std::size_t>(SoundId::Sprinklers)],
        activeShift && gameState.customers.sprinklersActive,
        audio.sfxVolume * 0.7f
    );
}

void adjustMasterVolume(AudioState& audio, float delta) {
    audio.masterVolume = std::clamp(audio.masterVolume + delta, 0.0f, 1.0f);
    if (audio.initialized && audio.engine) {
        ma_engine_set_volume(&audio.engine->engine, audio.masterVolume);
    }
    std::cout << "[AUDIO] Master volume: " << static_cast<int>(audio.masterVolume * 100.0f) << "%\n";
}