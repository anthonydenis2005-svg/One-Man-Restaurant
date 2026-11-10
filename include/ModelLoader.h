#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <filesystem>
#include <string>
#include <vector>

// A bounding box is the minimum and maximum corner around a 3D object.
struct BoundingBox {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

// Clickable objects have a name and a box that the mouse ray can hit.
struct InteractiveObject {
    std::string name;
    BoundingBox bounds;
};

// Each mesh part stores its material and which vertices to draw.
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

// Image data is loaded here before the renderer uploads it to the graphics card.
struct TextureImage {
    std::vector<unsigned char> encodedBytes;
    std::vector<unsigned char> rgbaPixels;
    unsigned int width = 0;
    unsigned int height = 0;
};

std::filesystem::path findModelPath();

// Loads the model, its textures, and the bounding boxes for interactive objects.
bool loadOfficeModel(
    const std::filesystem::path& path,
    std::vector<float>& vertices,
    std::vector<Part>& parts,
    std::vector<TextureImage>& images,
    std::vector<InteractiveObject>& interactiveObjects
);
