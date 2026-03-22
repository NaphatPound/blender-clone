#include "edit/EditSelection.h"
#include <cmath>

namespace sculpt {

void EditSelection::clear() {
    selectedVerts.clear();
    selectedEdges.clear();
    selectedFaces.clear();
}

void EditSelection::selectAll(const HalfEdgeMesh& mesh) {
    clear();
    switch (mode) {
        case SelectionMode::Vertex:
            for (int32_t i = 0; i < static_cast<int32_t>(mesh.vertexCount()); i++)
                selectedVerts.insert(i);
            break;
        case SelectionMode::Edge:
            for (int32_t i = 0; i < static_cast<int32_t>(mesh.halfEdgeCount()); i++) {
                auto [v0, v1] = mesh.edgeVertices(i);
                selectedEdges.insert(EdgeId(v0, v1));
            }
            break;
        case SelectionMode::Face:
            for (int32_t i = 0; i < static_cast<int32_t>(mesh.faceCount()); i++) {
                if (!mesh.isFaceDeleted(i))
                    selectedFaces.insert(i);
            }
            break;
    }
}

void EditSelection::toggleVertex(int32_t idx) {
    if (selectedVerts.count(idx)) selectedVerts.erase(idx);
    else selectedVerts.insert(idx);
}

void EditSelection::toggleEdge(EdgeId e) {
    if (selectedEdges.count(e)) selectedEdges.erase(e);
    else selectedEdges.insert(e);
}

void EditSelection::toggleFace(int32_t idx) {
    if (selectedFaces.count(idx)) selectedFaces.erase(idx);
    else selectedFaces.insert(idx);
}

void EditSelection::setVertex(int32_t idx) {
    selectedVerts.clear();
    selectedVerts.insert(idx);
}

void EditSelection::setEdge(EdgeId e) {
    selectedEdges.clear();
    selectedEdges.insert(e);
}

void EditSelection::setFace(int32_t idx) {
    selectedFaces.clear();
    selectedFaces.insert(idx);
}

bool EditSelection::hasSelection() const {
    switch (mode) {
        case SelectionMode::Vertex: return !selectedVerts.empty();
        case SelectionMode::Edge:   return !selectedEdges.empty();
        case SelectionMode::Face:   return !selectedFaces.empty();
    }
    return false;
}

std::vector<int32_t> EditSelection::getAffectedVertices(const HalfEdgeMesh& mesh) const {
    std::unordered_set<int32_t> verts;

    switch (mode) {
        case SelectionMode::Vertex:
            return std::vector<int32_t>(selectedVerts.begin(), selectedVerts.end());

        case SelectionMode::Edge:
            for (const auto& e : selectedEdges) {
                verts.insert(e.v0);
                verts.insert(e.v1);
            }
            break;

        case SelectionMode::Face:
            for (int32_t fi : selectedFaces) {
                if (fi < 0 || fi >= static_cast<int32_t>(mesh.faceCount())) continue;
                if (mesh.isFaceDeleted(fi)) continue;
                auto fv = mesh.faceVertices(fi);
                if (fv[0] >= 0) verts.insert(fv[0]);
                if (fv[1] >= 0) verts.insert(fv[1]);
                if (fv[2] >= 0) verts.insert(fv[2]);
            }
            break;
    }

    return std::vector<int32_t>(verts.begin(), verts.end());
}

int32_t EditSelection::pickVertex(const HalfEdgeMesh& mesh, int32_t faceIdx,
                                    const Vec3& hitPos) const {
    auto verts = mesh.faceVertices(faceIdx);
    float bestDist = 1e30f;
    int32_t bestVert = verts[0];

    for (int i = 0; i < 3; i++) {
        float d = (mesh.vertex(verts[i]).position - hitPos).lengthSq();
        if (d < bestDist) {
            bestDist = d;
            bestVert = verts[i];
        }
    }
    return bestVert;
}

EdgeId EditSelection::pickEdge(const HalfEdgeMesh& mesh, int32_t faceIdx,
                                 const Vec3& hitPos) const {
    auto verts = mesh.faceVertices(faceIdx);
    float bestDist = 1e30f;
    EdgeId bestEdge;

    for (int i = 0; i < 3; i++) {
        int32_t va = verts[i];
        int32_t vb = verts[(i + 1) % 3];
        const Vec3& a = mesh.vertex(va).position;
        const Vec3& b = mesh.vertex(vb).position;

        // Project hitPos onto edge segment
        Vec3 ab = b - a;
        float t = (hitPos - a).dot(ab) / (ab.lengthSq() + 1e-10f);
        t = std::max(0.0f, std::min(1.0f, t));
        Vec3 closest = a + ab * t;
        float d = (hitPos - closest).lengthSq();

        if (d < bestDist) {
            bestDist = d;
            bestEdge = EdgeId(va, vb);
        }
    }
    return bestEdge;
}

} // namespace sculpt
