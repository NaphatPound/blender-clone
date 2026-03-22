#include "sculpt/Brush.h"
#include <cmath>
#include <algorithm>

namespace sculpt {

float Brush::calculateFalloff(float distance) const {
    if (distance >= m_settings.radius) return 0.0f;
    if (distance <= 0.0f) return 1.0f;

    float t = distance / m_settings.radius;

    switch (m_settings.falloff) {
        case FalloffType::Smooth:
            return 1.0f - (3.0f * t * t - 2.0f * t * t * t);
        case FalloffType::Linear:
            return 1.0f - t;
        case FalloffType::Constant:
            return 1.0f;
        default:
            return 1.0f - t;
    }
}

// =====================================================
// DRAW — displace along vertex normals
// =====================================================
void Brush::applyDraw(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                       const Vec3& center, const Vec3& direction) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;
    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        const Vec3& normal = mesh.vertex(vi).normal;
        float dist = (pos - center).length();
        float weight = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength * sign);
        pos += normal * weight;
    }
}

// =====================================================
// SMOOTH — Laplacian smoothing toward neighbor average
// =====================================================
void Brush::applySmooth(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                          const Vec3& center) const {
    std::vector<Vec3> targets(verts.size());
    for (size_t i = 0; i < verts.size(); i++) {
        int32_t vi = verts[i];
        auto neighbors = mesh.getVertexNeighbors(vi);
        if (neighbors.empty()) { targets[i] = mesh.vertex(vi).position; continue; }
        Vec3 avg = {0, 0, 0};
        for (int32_t ni : neighbors) avg += mesh.vertex(ni).position;
        targets[i] = avg / static_cast<float>(neighbors.size());
    }
    for (size_t i = 0; i < verts.size(); i++) {
        int32_t vi = verts[i];
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float weight = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength);
        pos = pos + (targets[i] - pos) * weight;
    }
}

// =====================================================
// GRAB — translate vertices by delta
// =====================================================
void Brush::applyGrab(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                       const Vec3& center, const Vec3& delta) const {
    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float weight = maskedWeight(mesh, vi, calculateFalloff(dist));
        pos += delta * weight;
    }
}

// =====================================================
// CLAY — add material toward a plane above the surface
// =====================================================
void Brush::applyClay(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                       const Vec3& center, const Vec3& normal) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;
    Vec3 planeNormal = normal.normalized();
    // Clay plane sits slightly above the surface
    float planeOffset = m_settings.strength * m_settings.radius * 0.3f * sign;
    Vec3 planePoint = center + planeNormal * planeOffset;

    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float falloff = maskedWeight(mesh, vi, calculateFalloff(dist));

        // Push vertex toward the clay plane (only if below it for add, above for subtract)
        float height = (pos - planePoint).dot(planeNormal);
        if ((sign > 0 && height < 0) || (sign < 0 && height > 0)) {
            float displacement = -height * falloff * m_settings.strength;
            pos += planeNormal * displacement;
        }
    }
}

// =====================================================
// FLATTEN — push vertices toward average height plane
// =====================================================
void Brush::applyFlatten(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                           const Vec3& center) const {
    if (verts.empty()) return;

    // Compute average position and normal of affected vertices
    Vec3 avgPos = {0, 0, 0};
    Vec3 avgNormal = {0, 0, 0};
    for (int32_t vi : verts) {
        avgPos += mesh.vertex(vi).position;
        avgNormal += mesh.vertex(vi).normal;
    }
    avgPos = avgPos / static_cast<float>(verts.size());
    avgNormal = avgNormal.normalized();

    // Push each vertex toward the average plane
    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float falloff = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength);

        float height = (pos - avgPos).dot(avgNormal);
        pos -= avgNormal * (height * falloff);
    }
}

// =====================================================
// CREASE — create sharp valleys/ridges
// =====================================================
void Brush::applyCrease(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                          const Vec3& center) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;

    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        const Vec3& normal = mesh.vertex(vi).normal;
        float dist = (pos - center).length();
        float falloff = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength);

        // Push inward along normal (crease)
        pos -= normal * (falloff * 0.5f * sign);

        // Pinch toward center line
        Vec3 toCenter = center - pos;
        float lateralDist = toCenter.length();
        if (lateralDist > 1e-6f) {
            Vec3 lateral = toCenter.normalized();
            pos += lateral * (falloff * 0.3f);
        }
    }
}

// =====================================================
// PINCH — pull vertices toward brush center
// =====================================================
void Brush::applyPinch(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                         const Vec3& center) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;

    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float falloff = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength);

        Vec3 toCenter = (center - pos);
        if (toCenter.lengthSq() > 1e-8f) {
            pos += toCenter.normalized() * (falloff * 0.5f * sign);
        }
    }
}

// =====================================================
// INFLATE — expand/contract along each vertex's normal
// =====================================================
void Brush::applyInflate(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                           const Vec3& center) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;

    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        const Vec3& normal = mesh.vertex(vi).normal;
        float dist = (pos - center).length();
        float weight = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength * sign);

        // Displace along vertex normal uniformly
        pos += normal * weight;
    }
}

// =====================================================
// SCRAPE — remove peaks (only flatten vertices ABOVE average plane)
// =====================================================
void Brush::applyScrape(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                          const Vec3& center, const Vec3& normal) const {
    if (verts.empty()) return;

    Vec3 planeNormal = normal.normalized();
    float sign = m_settings.invert ? -1.0f : 1.0f;

    // Compute average height along the plane normal
    float avgHeight = 0;
    for (int32_t vi : verts) {
        avgHeight += (mesh.vertex(vi).position - center).dot(planeNormal);
    }
    avgHeight /= static_cast<float>(verts.size());

    for (int32_t vi : verts) {
        Vec3& pos = mesh.vertex(vi).position;
        float dist = (pos - center).length();
        float falloff = maskedWeight(mesh, vi, calculateFalloff(dist) * m_settings.strength);

        float height = (pos - center).dot(planeNormal) - avgHeight;
        // Only scrape peaks (positive height) when not inverted
        if (height * sign > 0) {
            pos -= planeNormal * (height * falloff * 0.8f);
        }
    }
}

// =====================================================
// MASK — paint mask values on vertices
// =====================================================
void Brush::applyMask(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                       const Vec3& center) const {
    float sign = m_settings.invert ? -1.0f : 1.0f;

    for (int32_t vi : verts) {
        float dist = (mesh.vertex(vi).position - center).length();
        float falloff = calculateFalloff(dist) * m_settings.strength;

        float& mask = mesh.vertex(vi).mask;
        if (sign > 0) {
            // Add mask
            mask = std::min(1.0f, mask + falloff);
        } else {
            // Remove mask
            mask = std::max(0.0f, mask - falloff);
        }
    }
}

} // namespace sculpt
