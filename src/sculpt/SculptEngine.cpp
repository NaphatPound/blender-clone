#include "sculpt/SculptEngine.h"

namespace sculpt {

void SculptEngine::setMesh(HalfEdgeMesh* mesh) {
    m_mesh = mesh;
    if (m_mesh) {
        rebuildBVH();
    }
}

void SculptEngine::rebuildBVH() {
    if (m_mesh) {
        m_bvh.build(*m_mesh);
    }
}

bool SculptEngine::stroke(const Vec3& worldPos, const Vec3& direction) {
    if (!m_mesh || !m_bvh.isBuilt()) return false;

    auto vertices = m_bvh.findVerticesInSphere(worldPos, m_brush.settings().radius, *m_mesh);
    if (vertices.empty()) return false;

    switch (m_brush.settings().type) {
        case BrushType::Draw:
            m_brush.applyDraw(*m_mesh, vertices, worldPos, direction);
            break;
        case BrushType::Smooth:
            m_brush.applySmooth(*m_mesh, vertices, worldPos);
            break;
        default:
            return false;
    }

    // Recompute normals for affected area
    m_mesh->recomputeNormals();
    return true;
}

bool SculptEngine::grabStroke(const Vec3& worldPos, const Vec3& delta) {
    if (!m_mesh || !m_bvh.isBuilt()) return false;

    auto vertices = m_bvh.findVerticesInSphere(worldPos, m_brush.settings().radius, *m_mesh);
    if (vertices.empty()) return false;

    m_brush.applyGrab(*m_mesh, vertices, worldPos, delta);
    m_mesh->recomputeNormals();
    return true;
}

RayHit SculptEngine::raycast(const Ray& ray) const {
    if (!m_mesh || !m_bvh.isBuilt()) return RayHit{};
    return m_bvh.raycast(ray, *m_mesh);
}

} // namespace sculpt
