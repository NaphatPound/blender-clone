#include "render/Camera.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace sculpt {

Camera::Camera() {}

void Camera::orbit(float deltaX, float deltaY) {
    m_yaw += deltaX * m_orbitSpeed;
    m_pitch += deltaY * m_orbitSpeed;

    // Clamp pitch to avoid flipping
    float limit = static_cast<float>(M_PI) * 0.49f;
    m_pitch = std::max(-limit, std::min(limit, m_pitch));
}

void Camera::pan(float deltaX, float deltaY) {
    Vec3 right = getRight();
    Vec3 up = getUp();

    m_target += right * (-deltaX * m_panSpeed * m_distance);
    m_target += up * (deltaY * m_panSpeed * m_distance);
}

void Camera::zoom(float delta) {
    m_distance *= (1.0f - delta * m_zoomSpeed);
    m_distance = std::max(0.1f, std::min(100.0f, m_distance));
}

Vec3 Camera::getPosition() const {
    float x = m_distance * std::cos(m_pitch) * std::sin(m_yaw);
    float y = m_distance * std::sin(m_pitch);
    float z = m_distance * std::cos(m_pitch) * std::cos(m_yaw);
    return m_target + Vec3(x, y, z);
}

Vec3 Camera::getForward() const {
    return (m_target - getPosition()).normalized();
}

Vec3 Camera::getRight() const {
    Vec3 forward = getForward();
    Vec3 worldUp(0, 1, 0);
    return forward.cross(worldUp).normalized();
}

Vec3 Camera::getUp() const {
    Vec3 forward = getForward();
    Vec3 right = getRight();
    return right.cross(forward).normalized();
}

std::array<float, 16> Camera::getViewMatrix() const {
    Vec3 pos = getPosition();
    Vec3 forward = getForward();
    Vec3 right = getRight();
    Vec3 up = right.cross(forward);

    // Column-major lookAt matrix
    std::array<float, 16> m = {};

    m[0]  = right.x;    m[4]  = right.y;    m[8]  = right.z;    m[12] = -right.dot(pos);
    m[1]  = up.x;       m[5]  = up.y;       m[9]  = up.z;       m[13] = -up.dot(pos);
    m[2]  = -forward.x; m[6]  = -forward.y; m[10] = -forward.z; m[14] = forward.dot(pos);
    m[3]  = 0;          m[7]  = 0;          m[11] = 0;          m[15] = 1;

    return m;
}

std::array<float, 16> Camera::getProjectionMatrix(float aspectRatio) const {
    float fovRad = m_fov * static_cast<float>(M_PI) / 180.0f;
    float tanHalf = std::tan(fovRad * 0.5f);

    std::array<float, 16> m = {};

    m[0]  = 1.0f / (aspectRatio * tanHalf);
    m[5]  = 1.0f / tanHalf;
    m[10] = -(m_far + m_near) / (m_far - m_near);
    m[11] = -1.0f;
    m[14] = -(2.0f * m_far * m_near) / (m_far - m_near);

    return m;
}

} // namespace sculpt
