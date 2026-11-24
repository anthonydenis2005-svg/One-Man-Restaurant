#pragma once

#include "Camera.h"
#include "GameState.h"
#include "ModelLoader.h"

#include <string>
#include <vector>

constexpr float kPaperworkHitboxInset = 0.22f;
constexpr float kPaperworkGridTop = 205.0f / 1685.0f;
constexpr float kPaperworkSlideSeconds = 0.72f;
constexpr float kPaperworkPenaltyFeedbackSeconds = 1.35f;

void assignDailyPaperwork(GameState& state, const std::vector<std::string>& pngFilenames);

int hitTestClipboardPaper(const Ray& ray,
                          const std::vector<float>& vertices,
                          const std::vector<Part>& parts,
                          const std::vector<InteractiveObject>& interactiveObjects,
                          const GameState& state);

void clickPaperworkSlot(GameState& state, int slot);

void updatePaperwork(GameState& state, float deltaTime);

void restartDailyPaperwork(GameState& state);

const PaperSheet* currentPaperSheet(const GameState& state);
