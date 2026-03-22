#include "edit/EditEngine.h"

namespace sculpt {

void EditEngine::setMesh(HalfEdgeMesh* mesh) {
    m_mesh = mesh;
    m_selection.clear();
    m_dirty = false;
}

void EditEngine::setBVH(BVH* bvh) {
    m_bvh = bvh;
}

bool EditEngine::pick(const Ray& ray, bool addToSelection) {
    if (!m_mesh || !m_bvh || !m_bvh->isBuilt()) return false;

    RayHit hit = m_bvh->raycast(ray, *m_mesh);
    if (!hit.hit) return false;

    switch (m_selection.mode) {
        case SelectionMode::Vertex: {
            int32_t vi = m_selection.pickVertex(*m_mesh, hit.faceIdx, hit.position);
            if (addToSelection) m_selection.toggleVertex(vi);
            else m_selection.setVertex(vi);
            break;
        }
        case SelectionMode::Edge: {
            EdgeId e = m_selection.pickEdge(*m_mesh, hit.faceIdx, hit.position);
            if (addToSelection) m_selection.toggleEdge(e);
            else m_selection.setEdge(e);
            break;
        }
        case SelectionMode::Face: {
            if (addToSelection) m_selection.toggleFace(hit.faceIdx);
            else m_selection.setFace(hit.faceIdx);
            break;
        }
    }
    return true;
}

void EditEngine::updateHover(const Ray& ray) {
    if (!m_mesh || !m_bvh || !m_bvh->isBuilt()) {
        m_hoveredEdgeHE = -1;
        return;
    }
    RayHit hit = m_bvh->raycast(ray, *m_mesh);
    if (!hit.hit) {
        m_hoveredEdgeHE = -1;
        return;
    }
    // Find the closest edge half-edge on the hit face
    EdgeId e = m_selection.pickEdge(*m_mesh, hit.faceIdx, hit.position);
    m_hoveredEdgeHE = m_mesh->findHalfEdge(e.v0, e.v1);
    if (m_hoveredEdgeHE < 0)
        m_hoveredEdgeHE = m_mesh->findHalfEdge(e.v1, e.v0);
}

bool EditEngine::extrude(float offset) {
    if (!m_mesh || !m_selection.hasSelection()) return false;
    bool ok = EditTools::extrude(*m_mesh, m_selection, offset);
    if (ok) m_dirty = true;
    return ok;
}

bool EditEngine::loopCut(int segments) {
    if (!m_mesh || m_hoveredEdgeHE < 0) return false;
    bool ok = EditTools::loopCut(*m_mesh, m_hoveredEdgeHE, segments);
    if (ok) {
        m_selection.clear();
        m_dirty = true;
    }
    return ok;
}

bool EditEngine::bevel(float amount) {
    if (!m_mesh || !m_selection.hasSelection()) return false;
    bool ok = EditTools::bevel(*m_mesh, m_selection, amount);
    if (ok) m_dirty = true;
    return ok;
}

bool EditEngine::insetFace(float amount) {
    if (!m_mesh || !m_selection.hasSelection()) return false;
    bool ok = EditTools::insetFace(*m_mesh, m_selection, amount);
    if (ok) m_dirty = true;
    return ok;
}

void EditEngine::selectAll() {
    if (m_mesh) m_selection.selectAll(*m_mesh);
}

void EditEngine::deselectAll() {
    m_selection.clear();
}

} // namespace sculpt
