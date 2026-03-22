#pragma once

#include "core/HalfEdgeMesh.h"
#include <array>
#include <cmath>

namespace sculpt {

class Camera {
public:
    Camera();

    // Orbit controls
    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY);
    void zoom(float delta);

    // Get matrices (column-major 4x4)
    std::array<float, 16> getViewMatrix() const;
    std::array<float, 16> getProjectionMatrix(float aspectRatio) const;

    // Camera parameters
    Vec3 getPosition() const;
    Vec3 getTarget() const { return m_target; }
    Vec3 getForward() const;
    Vec3 getRight() const;
    Vec3 getUp() const;

    void setTarget(const Vec3& target) { m_target = target; }
    void setDistance(float dist) { m_distance = dist; }

    float getFov() const { return m_fov; }
    void setFov(float fov) { m_fov = fov; }
    float getNearPlane() const { return m_near; }
    float getFarPlane() const { return m_far; }

    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }
    void setYaw(float yaw) { m_yaw = yaw; }
    void setPitch(float pitch) { m_pitch = pitch; }
    float getDistance() const { return m_distance; }

private:
    Vec3 m_target = {0, 0, 0};
    float m_distance = 5.0f;
    float m_yaw = 0.0f;     // horizontal rotation in radians
    float m_pitch = 0.3f;   // vertical rotation in radians
    float m_fov = 45.0f;    // field of view in degrees
    float m_near = 0.1f;
    float m_far = 100.0f;
    float m_panSpeed = 0.01f;
    float m_orbitSpeed = 0.005f;
    float m_zoomSpeed = 0.1f;
};

} // namespace sculpt
