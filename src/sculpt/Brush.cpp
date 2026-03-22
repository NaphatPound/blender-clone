#include "sculpt/Brush.h"
#include <cmath>

namespace sculpt {

float Brush::calculateFalloff(float distance) const {
    if (distance >= m_settings.radius) return 0.0f;
    if (distance <= 0.0f) return 1.0f;

    float t = distance / m_settings.radius;

    switch (m_settings.falloff) {
        case FalloffType::Smooth:
            // Smooth hermite interpolation
            return 1.0f - (3.0f * t * t - 2.0f * t * t * t);
        case FalloffType::Linear:
            return 1.0f - t;
        case FalloffType::Constant:
            return 1.0f;
        default:
            return 1.0f - t;
    }
}

void Brush::applyDraw(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                       const Vec3& center, const Vec3& direction) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;

    for (int32_t vi : vertexIndices) {
        Vec3& pos = mesh.vertex(vi).position;
        const Vec3& normal = mesh.vertex(vi).normal;

        float dist = (pos - center).length();
        float weight = calculateFalloff(dist) * m_settings.strength * sign;

        // Displace along vertex normal
        pos += normal * weight;
    }
}

void Brush::applySmooth(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                          const Vec3& center) const {
    // First pass: compute target positions (Laplacian smoothing)
    std::vector<Vec3> targetPositions(vertexIndices.size());

    for (size_t i = 0; i < vertexIndices.size(); i++) {
        int32_t vi = vertexIndices[i];
        auto neighbors = mesh.getVertexNeighbors(vi);

        if (neighbors.empty()) {
            targetPositions[i] = mesh.vertex(vi).position;
            continue;
        }

        Vec3 avg = {0, 0, 0};
        for (int32_t ni : neighbors) {
            avg += mesh.vertex(ni).position;
        }
        avg = avg / static_cast<float>(neighbors.size());
        targetPositions[i] = avg;
    }

    // Second pass: blend toward target based on falloff
    for (size_t i = 0; i < vertexIndices.size(); i++) {
        int32_t vi = vertexIndices[i];
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float weight = calculateFalloff(dist) * m_settings.strength;

        pos = pos + (targetPositions[i] - pos) * weight;
    }
}

void Brush::applyGrab(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                       const Vec3& center, const Vec3& delta) const {
    for (int32_t vi : vertexIndices) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float weight = calculateFalloff(dist);

        pos += delta * weight;
    }
}

} // namespace sculpt
