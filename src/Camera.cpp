#include "Camera.h"
#include "ModelLoader.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

glm::vec3 getCameraPosition() {
    return glm::vec3(4.6f, 1.45f, 6.4f);
}

void resetCameraDirection(CameraState& camera) {
    camera.yaw = kInitialCameraYaw;
    camera.pitch = kInitialCameraPitch;
    camera.rotating = false;
}

glm::mat4 calculateViewProjection(const CameraState& camera, int width, int height) {
    const glm::vec3 cameraPosition = getCameraPosition();
    const float yawRadians = glm::radians(camera.yaw);
    const float pitchRadians = glm::radians(camera.pitch);
    const glm::vec3 forward(
        std::cos(yawRadians) * std::cos(pitchRadians),
        std::sin(pitchRadians),
        std::sin(yawRadians) * std::cos(pitchRadians)
    );
    const glm::mat4 view = glm::lookAt(
        cameraPosition, cameraPosition + glm::normalize(forward), glm::vec3(0.0f, 1.0f, 0.0f)
    );
    const float aspect = static_cast<float>(width) / static_cast<float>(height);
    const glm::mat4 projection = glm::perspective(glm::radians(50.0f), aspect, 0.01f, 500.0f);
    return projection * view;
}

Ray makeMouseRay(const CameraState& camera, double mouseX, double mouseY, int width, int height) {
    const glm::mat4 inverse = glm::inverse(calculateViewProjection(camera, width, height));
    const float x = static_cast<float>(2.0 * mouseX / width - 1.0);
    const float y = static_cast<float>(1.0 - 2.0 * mouseY / height);
    glm::vec4 nearPoint = inverse * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint = inverse * glm::vec4(x, y, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    return {getCameraPosition(), glm::normalize(glm::vec3(farPoint - nearPoint))};
}

bool rayIntersectsBox(const Ray& ray, const BoundingBox& box, float& distance) {
    float nearDistance = 0.0f;
    float farDistance = std::numeric_limits<float>::max();
    for (int axis = 0; axis < 3; ++axis) {
        if (std::abs(ray.direction[axis]) < 0.000001f) {
            if (ray.origin[axis] < box.min[axis] || ray.origin[axis] > box.max[axis]) return false;
            continue;
        }
        float first = (box.min[axis] - ray.origin[axis]) / ray.direction[axis];
        float second = (box.max[axis] - ray.origin[axis]) / ray.direction[axis];
        if (first > second) std::swap(first, second);
        nearDistance = std::max(nearDistance, first);
        farDistance = std::min(farDistance, second);
        if (nearDistance > farDistance) return false;
    }
    distance = nearDistance;
    return true;
}
