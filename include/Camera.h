#pragma once

#include <glm/glm.hpp>

struct BoundingBox;

// These angles are where the office camera starts and resets when opening the clipboard.
constexpr float kInitialCameraYaw = -90.29f;
constexpr float kInitialCameraPitch = -13.107f;

// Stores the camera direction and mouse actions that main.cpp processes each frame.
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

// A ray starts at the camera and points toward whatever the player clicked.
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

glm::vec3 getCameraPosition();
void resetCameraDirection(CameraState& camera);
glm::mat4 calculateViewProjection(const CameraState& camera, int width, int height);
// These are used to turn a mouse click into a 3D ray and test clickable objects.
Ray makeMouseRay(const CameraState& camera, double mouseX, double mouseY, int width, int height);
bool rayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance);
