#pragma once

#include "GameState.h"
#include "ModelLoader.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <vector>

struct RendererState {
    std::vector<GLuint> textures;
    std::vector<GLuint> paperTextures;
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    GLint viewProjectionLocation = -1;
    GLint modelLocation = -1;
    GLint colorLocation = -1;
    GLint texturedLocation = -1;
    GLint paperOverrideLocation = -1;
    GLint textureLocation = -1;
};

bool initializeRenderer(
    RendererState& renderer,
    const std::vector<float>& vertices,
    const std::vector<TextureImage>& images,
    const std::filesystem::path& modelPath
);
void spawnText(const std::string& modelName);
void hideText();
glm::mat4 clipboardModelTransform(const BoundingBox& originalBounds, float progress);
void renderFrame(
    const RendererState& renderer,
    const std::vector<Part>& parts,
    const std::vector<InteractiveObject>& interactiveObjects,
    const glm::mat4& viewProjection,
    const GameState& gameState, int width, int height
);
void destroyRenderer(const RendererState& renderer);
