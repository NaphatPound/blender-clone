#pragma once

#include "core/HalfEdgeMesh.h"
#include <unordered_set>
#include <vector>

namespace sculpt {

enum class SelectionMode { Vertex, Edge, Face };

struct EdgeId {
    int32_t v0, v1; // always v0 < v1
    EdgeId() : v0(-1), v1(-1) {}
    EdgeId(int32_t a, int32_t b) {
        if (a < b) { v0 = a; v1 = b; }
        else       { v0 = b; v1 = a; }
    }
    bool operator==(const EdgeId& o) const { return v0 == o.v0 && v1 == o.v1; }
};

struct EdgeIdHash {
    size_t operator()(const EdgeId& e) const {
        return std::hash<int64_t>()(
            (static_cast<int64_t>(e.v0) << 32) | static_cast<int64_t>(e.v1));
    }
};

class EditSelection {
public:
    SelectionMode mode = SelectionMode::Vertex;

    std::unordered_set<int32_t> selectedVerts;
    std::unordered_set<EdgeId, EdgeIdHash> selectedEdges;
    std::unordered_set<int32_t> selectedFaces;

    void clear();
    void selectAll(const HalfEdgeMesh& mesh);

    void toggleVertex(int32_t idx);
    void toggleEdge(EdgeId e);
    void toggleFace(int32_t idx);

    void setVertex(int32_t idx);
    void setEdge(EdgeId e);
    void setFace(int32_t idx);

    bool hasSelection() const;

    // Get all vertices affected by current selection (for tools)
    std::vector<int32_t> getAffectedVertices(const HalfEdgeMesh& mesh) const;

    // Pick from a raycast hit
    int32_t pickVertex(const HalfEdgeMesh& mesh, int32_t faceIdx, const Vec3& hitPos) const;
    EdgeId pickEdge(const HalfEdgeMesh& mesh, int32_t faceIdx, const Vec3& hitPos) const;
};

} // namespace sculpt
