#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <filesystem>
#include <string>
#include <vector>

struct BoundingBox {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

struct InteractiveObject {
    std::string name;
    BoundingBox bounds;
};

struct Part {
    std::string name;
    std::string interactiveName;
    GLint first = 0;
    GLsizei count = 0;
    glm::vec3 color{0.8f};
    int baseColorTextureIndex = -1;
    bool isClipboardPaper = false;
    glm::vec3 center{0.0f};
    float modelWidth = 1.0f;
};

struct TextureImage {
    std::vector<unsigned char> encodedBytes;
    std::vector<unsigned char> rgbaPixels;
    unsigned int width = 0;
    unsigned int height = 0;
};

std::filesystem::path findModelPath();
bool loadOfficeModel(
    const std::filesystem::path& path,
    std::vector<float>& vertices,
    std::vector<Part>& parts,
    std::vector<TextureImage>& images,
    std::vector<InteractiveObject>& interactiveObjects
);
