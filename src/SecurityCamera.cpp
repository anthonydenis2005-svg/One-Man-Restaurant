#include "SecurityCamera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <iostream>

bool SecurityCamera::initialize(const std::vector<ImportedSecurityCamera>& views,
                                int width, int height) {
    destroy();
    if (views.empty() || width < 1 || height < 1) return false;
    views_ = views;
    width_ = width;
    height_ = height;

    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

    glGenTextures(1, &colorTexture_);
    glBindTexture(GL_TEXTURE_2D, colorTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, colorTexture_, 0);

    glGenRenderbuffers(1, &depthBuffer_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthBuffer_);

    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!complete) {
        std::cerr << "Security camera framebuffer could not be created.\n";
        destroy();
        return false;
    }
#ifdef OMR_VERBOSE_LOGS
    std::cout << "Security cameras ready: " << views_.size()
              << " viewpoints. Click CameraScreen to switch.\n";
#endif
    return true;
}

void SecurityCamera::destroy() {
    if (depthBuffer_) glDeleteRenderbuffers(1, &depthBuffer_);
    if (colorTexture_) glDeleteTextures(1, &colorTexture_);
    if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
    depthBuffer_ = 0;
    colorTexture_ = 0;
    framebuffer_ = 0;
    views_.clear();
    activeView_ = 0;
}

bool SecurityCamera::available() const {
    return framebuffer_ != 0 && !views_.empty();
}

void SecurityCamera::nextView() {
    if (!available()) return;
    activeView_ = (activeView_ + 1) % views_.size();
#ifdef OMR_VERBOSE_LOGS
    std::cout << "Security camera: " << views_[activeView_].name << '\n';
#endif
}

void SecurityCamera::beginCapture() const {
    if (available()) glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
}

void SecurityCamera::endCapture() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

glm::mat4 SecurityCamera::viewProjection() const {
    if (!available()) return glm::mat4(1.0f);
    const ImportedSecurityCamera& camera = views_[activeView_];
    const glm::vec3 forward = glm::normalize(camera.forward);
    glm::vec3 up = glm::normalize(camera.up);
    if (glm::length(glm::cross(forward, up)) < 0.0001f) up = glm::vec3(0, 1, 0);

    const glm::mat4 view = glm::lookAt(
        camera.position, camera.position + forward, up
    );
    const float fov = std::clamp(camera.verticalFovRadians, 0.08f, 3.0f);
    const glm::mat4 projection = glm::perspective(
        fov, static_cast<float>(width_) / static_cast<float>(height_),
        camera.nearClip, camera.farClip
    );
    return projection * view;
}
