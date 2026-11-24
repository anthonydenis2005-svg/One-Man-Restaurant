#pragma once

#include "ModelLoader.h"

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

class SecurityCamera {
public:
    bool initialize(const std::vector<ImportedSecurityCamera>& views,
                    int width = 640, int height = 360);
    void destroy();

    bool available() const;
    void nextView();
    void beginCapture() const;
    void endCapture() const;
    glm::mat4 viewProjection() const;

    GLuint texture() const { return colorTexture_; }
    int width() const { return width_; }
    int height() const { return height_; }

private:
    std::vector<ImportedSecurityCamera> views_;
    std::size_t activeView_ = 0;
    GLuint framebuffer_ = 0;
    GLuint colorTexture_ = 0;
    GLuint depthBuffer_ = 0;
    int width_ = 640;
    int height_ = 360;
};
