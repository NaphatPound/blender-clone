#include "sculpt/SculptEngine.h"
#include <unordered_set>
#include <algorithm>

namespace sculpt {

void SculptEngine::setMesh(HalfEdgeMesh* mesh) {
    m_mesh = mesh;
    if (m_mesh) rebuildBVH();
}

void SculptEngine::rebuildBVH() {
    if (m_mesh) m_bvh.build(*m_mesh);
}

// =====================================================
// Partial Normal Update — only recompute affected region
// =====================================================
void SculptEngine::recomputeNormalsPartial(const std::vector<int32_t>& affectedVerts) {
    if (!m_mesh || affectedVerts.empty()) return;

    // If too many affected vertices, just do full recompute (faster than partial overhead)
    if (affectedVerts.size() > m_mesh->vertexCount() / 3) {
        m_mesh->recomputeNormals();
        return;
    }

    // Collect all faces adjacent to affected vertices
    std::unordered_set<int32_t> facesToUpdate;
    for (int32_t vi : affectedVerts) {
        auto faces = m_mesh->getVertexFaces(vi);
        for (int32_t fi : faces) facesToUpdate.insert(fi);
    }

    // Recompute face normals for affected faces only
    for (int32_t fi : facesToUpdate) {
        if (fi < 0 || fi >= static_cast<int32_t>(m_mesh->faceCount())) continue;
        if (m_mesh->isFaceDeleted(fi)) continue;

        auto fv = m_mesh->faceVertices(fi);
        if (fv[0] < 0) continue;

        const Vec3& p0 = m_mesh->vertex(fv[0]).position;
        const Vec3& p1 = m_mesh->vertex(fv[1]).position;
        const Vec3& p2 = m_mesh->vertex(fv[2]).position;
        Vec3 edge1 = p1 - p0;
        Vec3 edge2 = p2 - p0;
        m_mesh->face(fi).normal = edge1.cross(edge2).normalized();
    }

    // Recompute vertex normals for affected vertices + their neighbors
    std::unordered_set<int32_t> vertsToUpdate(affectedVerts.begin(), affectedVerts.end());
    for (int32_t vi : affectedVerts) {
        auto neighbors = m_mesh->getVertexNeighbors(vi);
        for (int32_t ni : neighbors) vertsToUpdate.insert(ni);
    }

    for (int32_t vi : vertsToUpdate) {
        if (vi < 0 || vi >= static_cast<int32_t>(m_mesh->vertexCount())) continue;
        Vec3 normal = {0, 0, 0};
        auto adjFaces = m_mesh->getVertexFaces(vi);
        for (int32_t fi : adjFaces) {
            if (fi >= 0 && fi < static_cast<int32_t>(m_mesh->faceCount()))
                normal += m_mesh->face(fi).normal;
        }
        m_mesh->vertex(vi).normal = adjFaces.empty() ? Vec3(0, 1, 0) : normal.normalized();
    }
}

// =====================================================
// Single stroke at one position (no symmetry)
// =====================================================
bool SculptEngine::strokeAt(const Vec3& worldPos, const Vec3& direction) {
    auto vertices = m_bvh.findVerticesInSphere(worldPos, m_brush.settings().radius, *m_mesh);
    if (vertices.empty()) return false;

    switch (m_brush.settings().type) {
        case BrushType::Draw:
            m_brush.applyDraw(*m_mesh, vertices, worldPos, direction);
            break;
        case BrushType::Smooth:
            m_brush.applySmooth(*m_mesh, vertices, worldPos);
            break;
        case BrushType::Clay:
            m_brush.applyClay(*m_mesh, vertices, worldPos, direction);
            break;
        case BrushType::Flatten:
            m_brush.applyFlatten(*m_mesh, vertices, worldPos);
            break;
        case BrushType::Crease:
            m_brush.applyCrease(*m_mesh, vertices, worldPos);
            break;
        case BrushType::Pinch:
            m_brush.applyPinch(*m_mesh, vertices, worldPos);
            break;
        case BrushType::Inflate:
            m_brush.applyInflate(*m_mesh, vertices, worldPos);
            break;
        case BrushType::Scrape:
            m_brush.applyScrape(*m_mesh, vertices, worldPos, direction);
            break;
        case BrushType::Mask:
            m_brush.applyMask(*m_mesh, vertices, worldPos);
            break;
        default:
            return false;
    }

    recomputeNormalsPartial(vertices);
    return true;
}

bool SculptEngine::grabAt(const Vec3& worldPos, const Vec3& delta) {
    auto vertices = m_bvh.findVerticesInSphere(worldPos, m_brush.settings().radius, *m_mesh);
    if (vertices.empty()) return false;
    m_brush.applyGrab(*m_mesh, vertices, worldPos, delta);
    recomputeNormalsPartial(vertices);
    return true;
}

// =====================================================
// Main stroke — applies symmetry
// =====================================================
bool SculptEngine::stroke(const Vec3& worldPos, const Vec3& direction) {
    if (!m_mesh || !m_bvh.isBuilt()) return false;

    bool hit = strokeAt(worldPos, direction);

    // Symmetry: mirror across selected axes
    if (m_brush.settings().symmetryX) {
        Vec3 mirrorPos = {-worldPos.x, worldPos.y, worldPos.z};
        Vec3 mirrorDir = {-direction.x, direction.y, direction.z};
        hit |= strokeAt(mirrorPos, mirrorDir);
    }
    if (m_brush.settings().symmetryY) {
        Vec3 mirrorPos = {worldPos.x, -worldPos.y, worldPos.z};
        Vec3 mirrorDir = {direction.x, -direction.y, direction.z};
        hit |= strokeAt(mirrorPos, mirrorDir);
    }
    if (m_brush.settings().symmetryZ) {
        Vec3 mirrorPos = {worldPos.x, worldPos.y, -worldPos.z};
        Vec3 mirrorDir = {direction.x, direction.y, -direction.z};
        hit |= strokeAt(mirrorPos, mirrorDir);
    }

    return hit;
}

bool SculptEngine::grabStroke(const Vec3& worldPos, const Vec3& delta) {
    if (!m_mesh || !m_bvh.isBuilt()) return false;

    bool hit = grabAt(worldPos, delta);

    if (m_brush.settings().symmetryX) {
        Vec3 mp = {-worldPos.x, worldPos.y, worldPos.z};
        Vec3 md = {-delta.x, delta.y, delta.z};
        hit |= grabAt(mp, md);
    }
    if (m_brush.settings().symmetryY) {
        Vec3 mp = {worldPos.x, -worldPos.y, worldPos.z};
        Vec3 md = {delta.x, -delta.y, delta.z};
        hit |= grabAt(mp, md);
    }
    if (m_brush.settings().symmetryZ) {
        Vec3 mp = {worldPos.x, worldPos.y, -worldPos.z};
        Vec3 md = {delta.x, delta.y, -delta.z};
        hit |= grabAt(mp, md);
    }

    return hit;
}

RayHit SculptEngine::raycast(const Ray& ray) const {
    if (!m_mesh || !m_bvh.isBuilt()) return RayHit{};
    return m_bvh.raycast(ray, *m_mesh);
}

// =====================================================
// Undo/Redo
// =====================================================
SculptEngine::MeshSnapshot SculptEngine::captureSnapshot() const {
    MeshSnapshot snap;
    if (!m_mesh) return snap;
    size_t n = m_mesh->vertexCount();
    snap.positions.resize(n);
    snap.normals.resize(n);
    snap.colors.resize(n);
    snap.masks.resize(n);
    for (size_t i = 0; i < n; i++) {
        const auto& v = m_mesh->vertex(static_cast<int32_t>(i));
        snap.positions[i] = v.position;
        snap.normals[i] = v.normal;
        snap.colors[i] = v.color;
        snap.masks[i] = v.mask;
    }
    return snap;
}

void SculptEngine::restoreSnapshot(const MeshSnapshot& snap) {
    if (!m_mesh) return;
    size_t n = std::min(snap.positions.size(), m_mesh->vertexCount());
    for (size_t i = 0; i < n; i++) {
        auto& v = m_mesh->vertex(static_cast<int32_t>(i));
        v.position = snap.positions[i];
        v.normal = snap.normals[i];
        v.color = snap.colors[i];
        v.mask = snap.masks[i];
    }
}

void SculptEngine::pushUndo() {
    if (!m_mesh) return;
    m_undoStack.push_back(captureSnapshot());
    if (static_cast<int>(m_undoStack.size()) > MAX_UNDO) {
        m_undoStack.erase(m_undoStack.begin());
    }
    m_redoStack.clear(); // new action clears redo
}

bool SculptEngine::undo() {
    if (m_undoStack.empty() || !m_mesh) return false;
    m_redoStack.push_back(captureSnapshot()); // save current for redo
    restoreSnapshot(m_undoStack.back());
    m_undoStack.pop_back();
    return true;
}

bool SculptEngine::redo() {
    if (m_redoStack.empty() || !m_mesh) return false;
    m_undoStack.push_back(captureSnapshot()); // save current for undo
    restoreSnapshot(m_redoStack.back());
    m_redoStack.pop_back();
    return true;
}

void SculptEngine::clearMask() {
    if (!m_mesh) return;
    for (size_t i = 0; i < m_mesh->vertexCount(); i++)
        m_mesh->vertex(static_cast<int32_t>(i)).mask = 0;
}

void SculptEngine::invertMask() {
    if (!m_mesh) return;
    for (size_t i = 0; i < m_mesh->vertexCount(); i++) {
        float& m = m_mesh->vertex(static_cast<int32_t>(i)).mask;
        m = 1.0f - m;
    }
}

} // namespace sculpt
