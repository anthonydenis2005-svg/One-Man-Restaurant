#pragma once

#include <glm/glm.hpp>

struct BoundingBox;

constexpr float kInitialCameraYaw = -90.29f;
constexpr float kInitialCameraPitch = -13.107f;

struct CameraState {
    float yaw = kInitialCameraYaw;
    float pitch = kInitialCameraPitch;
    bool rotating = false;
    bool locked = false;
    bool clickPending = false;
    bool closeClipboardPending = false;
    double clickX = 0.0;
    double clickY = 0.0;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
};

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

glm::vec3 getCameraPosition();
void resetCameraDirection(CameraState& camera);
glm::mat4 calculateViewProjection(const CameraState& camera, int width, int height);
Ray makeMouseRay(const CameraState& camera, double mouseX, double mouseY, int width, int height);
bool rayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance);
