#include "paint/PaintEngine.h"

namespace sculpt {

PaintEngine::PaintEngine() {
    // Default lowpoly color palette
    m_palette = {
        {1.0f, 1.0f, 1.0f},    // white
        {0.15f, 0.15f, 0.15f},  // dark gray
        {0.85f, 0.25f, 0.20f},  // red
        {0.95f, 0.55f, 0.15f},  // orange
        {0.95f, 0.85f, 0.20f},  // yellow
        {0.30f, 0.75f, 0.30f},  // green
        {0.20f, 0.50f, 0.85f},  // blue
        {0.60f, 0.30f, 0.80f},  // purple
        {0.90f, 0.60f, 0.45f},  // skin
        {0.50f, 0.35f, 0.20f},  // brown
        {0.35f, 0.55f, 0.35f},  // forest green
        {0.55f, 0.75f, 0.90f},  // sky blue
        {0.75f, 0.75f, 0.70f},  // stone gray
        {0.85f, 0.80f, 0.65f},  // sand
        {0.20f, 0.35f, 0.15f},  // dark green
        {0.40f, 0.20f, 0.10f},  // dark brown
    };
}

bool PaintEngine::paintStroke(const Vec3& worldPos) {
    if (!m_mesh || !m_bvh || !m_bvh->isBuilt()) return false;

    auto verts = m_bvh->findVerticesInSphere(worldPos, m_settings.radius, *m_mesh);
    if (verts.empty()) return false;

    for (int32_t vi : verts) {
        if (vi < 0 || vi >= static_cast<int32_t>(m_mesh->vertexCount())) continue;
        Vec3& col = m_mesh->vertex(vi).color;
        float dist = (m_mesh->vertex(vi).position - worldPos).length();
        float t = dist / m_settings.radius;
        // Smooth falloff
        float weight = (1.0f - (3.0f * t * t - 2.0f * t * t * t)) * m_settings.strength;
        weight = std::max(0.0f, std::min(1.0f, weight));
        col = col + (m_settings.color - col) * weight;
    }
    return true;
}

bool PaintEngine::fillFace(const Ray& ray) {
    if (!m_mesh || !m_bvh || !m_bvh->isBuilt()) return false;

    RayHit hit = m_bvh->raycast(ray, *m_mesh);
    if (!hit.hit) return false;

    auto fv = m_mesh->faceVertices(hit.faceIdx);
    for (int i = 0; i < 3; i++) {
        if (fv[i] >= 0 && fv[i] < static_cast<int32_t>(m_mesh->vertexCount()))
            m_mesh->vertex(fv[i]).color = m_settings.color;
    }
    return true;
}

Vec3 PaintEngine::sampleColor(const Ray& ray) const {
    if (!m_mesh || !m_bvh || !m_bvh->isBuilt()) return {1, 1, 1};

    RayHit hit = m_bvh->raycast(ray, *m_mesh);
    if (!hit.hit) return {1, 1, 1};

    auto fv = m_mesh->faceVertices(hit.faceIdx);
    if (fv[0] < 0) return {1, 1, 1};

    // Average color of hit face
    Vec3 c = m_mesh->vertex(fv[0]).color + m_mesh->vertex(fv[1]).color + m_mesh->vertex(fv[2]).color;
    return c / 3.0f;
}

} // namespace sculpt
