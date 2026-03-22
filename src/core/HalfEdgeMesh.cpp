#include "core/HalfEdgeMesh.h"
#include <unordered_map>
#include <algorithm>

namespace sculpt {

// Hash for edge pairs
struct EdgeHash {
    size_t operator()(const std::pair<int32_t, int32_t>& p) const {
        return std::hash<int64_t>()(
            (static_cast<int64_t>(p.first) << 32) | static_cast<int64_t>(p.second));
    }
};

void HalfEdgeMesh::buildFromTriangles(const std::vector<Vec3>& positions,
                                       const std::vector<std::array<uint32_t, 3>>& triangles) {
    m_vertices.clear();
    m_halfEdges.clear();
    m_faces.clear();

    // Create vertices
    m_vertices.resize(positions.size());
    for (size_t i = 0; i < positions.size(); i++) {
        m_vertices[i].position = positions[i];
        m_vertices[i].halfEdge = -1;
    }

    // Map from directed edge (v0,v1) to half-edge index
    std::unordered_map<std::pair<int32_t, int32_t>, int32_t, EdgeHash> edgeMap;

    // Create faces and half-edges
    m_faces.resize(triangles.size());
    m_halfEdges.resize(triangles.size() * 3);

    for (size_t fi = 0; fi < triangles.size(); fi++) {
        const auto& tri = triangles[fi];
        int32_t baseHE = static_cast<int32_t>(fi * 3);

        m_faces[fi].halfEdge = baseHE;

        for (int j = 0; j < 3; j++) {
            int32_t heIdx = baseHE + j;
            int32_t nextIdx = baseHE + (j + 1) % 3;
            int32_t prevIdx = baseHE + (j + 2) % 3;

            int32_t v0 = static_cast<int32_t>(tri[j]);
            int32_t v1 = static_cast<int32_t>(tri[(j + 1) % 3]);

            m_halfEdges[heIdx].vertex = v1;
            m_halfEdges[heIdx].next = nextIdx;
            m_halfEdges[heIdx].prev = prevIdx;
            m_halfEdges[heIdx].face = static_cast<int32_t>(fi);
            m_halfEdges[heIdx].twin = -1;

            // Set vertex half-edge if not set
            if (m_vertices[v0].halfEdge == -1) {
                m_vertices[v0].halfEdge = heIdx;
            }

            // Register edge for twin finding
            edgeMap[{v0, v1}] = heIdx;
        }
    }

    // Link twins
    for (auto& [edge, heIdx] : edgeMap) {
        auto twinIt = edgeMap.find({edge.second, edge.first});
        if (twinIt != edgeMap.end()) {
            m_halfEdges[heIdx].twin = twinIt->second;
        }
    }

    // Compute normals
    recomputeNormals();
}

std::vector<int32_t> HalfEdgeMesh::getVertexNeighbors(int32_t vertIdx) const {
    std::vector<int32_t> neighbors;
    int32_t startHE = m_vertices[vertIdx].halfEdge;
    if (startHE < 0) return neighbors;

    // Safety limit to prevent infinite loops with corrupted topology
    int maxIter = static_cast<int>(m_halfEdges.size());
    int32_t he = startHE;
    int iter = 0;
    do {
        if (m_halfEdges[he].face >= 0) {
            int32_t targetVert = m_halfEdges[he].vertex;
            neighbors.push_back(targetVert);
        }

        int32_t prevHE = m_halfEdges[he].prev;
        if (prevHE < 0) break;
        he = m_halfEdges[prevHE].twin;
        if (he < 0) break;
    } while (he != startHE && ++iter < maxIter);

    return neighbors;
}

std::vector<int32_t> HalfEdgeMesh::getVertexFaces(int32_t vertIdx) const {
    std::vector<int32_t> faces;
    int32_t startHE = m_vertices[vertIdx].halfEdge;
    if (startHE < 0) return faces;

    // If the start half-edge is from a deleted face, try to find a live one
    if (m_halfEdges[startHE].face < 0) {
        // Search for any live half-edge from this vertex
        bool found = false;
        for (size_t fi = 0; fi < m_faces.size(); fi++) {
            if (m_faces[fi].halfEdge < 0) continue;
            auto he0 = m_faces[fi].halfEdge;
            for (int j = 0; j < 3; j++) {
                int32_t he = he0 + j;
                if (he < static_cast<int32_t>(m_halfEdges.size()) &&
                    heOrigin(he) == vertIdx) {
                    startHE = he;
                    found = true;
                    break;
                }
            }
            if (found) break;
        }
        if (!found) return faces;
    }

    int maxIter = static_cast<int>(m_halfEdges.size());
    int32_t he = startHE;
    int iter = 0;
    do {
        if (m_halfEdges[he].face >= 0) {
            faces.push_back(m_halfEdges[he].face);
        }

        int32_t prevHE = m_halfEdges[he].prev;
        if (prevHE < 0) break;
        he = m_halfEdges[prevHE].twin;
        if (he < 0) break;
    } while (he != startHE && ++iter < maxIter);

    return faces;
}

void HalfEdgeMesh::computeFaceNormals() {
    for (size_t fi = 0; fi < m_faces.size(); fi++) {
        int32_t he0 = m_faces[fi].halfEdge;
        int32_t he1 = m_halfEdges[he0].next;
        int32_t he2 = m_halfEdges[he1].next;

        // Get the three vertices of the face
        // he0 points to v1, so we need the origin of he0 which is vertex of prev
        int32_t v0idx = m_halfEdges[m_halfEdges[he0].prev].vertex;
        int32_t v1idx = m_halfEdges[he0].vertex;
        int32_t v2idx = m_halfEdges[he1].vertex;

        // Wait - in our structure, he points TO vertex.
        // For face with half-edges he0->he1->he2:
        // he0.vertex = where he0 points to
        // The origin of he0 is he0.prev.vertex (which is he2.vertex since prev of he0 is he2)
        // Actually let's just use the triangle vertex indices directly

        const Vec3& p0 = m_vertices[v0idx].position;
        const Vec3& p1 = m_vertices[v1idx].position;
        const Vec3& p2 = m_vertices[v2idx].position;

        Vec3 edge1 = p1 - p0;
        Vec3 edge2 = p2 - p0;
        m_faces[fi].normal = edge1.cross(edge2).normalized();
    }
}

void HalfEdgeMesh::computeVertexNormals() {
    for (size_t vi = 0; vi < m_vertices.size(); vi++) {
        Vec3 normal = {0, 0, 0};
        auto adjFaces = getVertexFaces(static_cast<int32_t>(vi));
        for (int32_t fi : adjFaces) {
            normal += m_faces[fi].normal;
        }
        if (adjFaces.size() > 0) {
            m_vertices[vi].normal = normal.normalized();
        } else {
            m_vertices[vi].normal = {0, 1, 0};
        }
    }
}

void HalfEdgeMesh::recomputeNormals() {
    computeFaceNormals();
    computeVertexNormals();
}

void HalfEdgeMesh::getTriangleData(std::vector<float>& vertexData,
                                     std::vector<uint32_t>& indices) const {
    // Vertex data: position(3) + normal(3) + color(3) per vertex
    vertexData.resize(m_vertices.size() * 9);
    for (size_t i = 0; i < m_vertices.size(); i++) {
        vertexData[i * 9 + 0] = m_vertices[i].position.x;
        vertexData[i * 9 + 1] = m_vertices[i].position.y;
        vertexData[i * 9 + 2] = m_vertices[i].position.z;
        vertexData[i * 9 + 3] = m_vertices[i].normal.x;
        vertexData[i * 9 + 4] = m_vertices[i].normal.y;
        vertexData[i * 9 + 5] = m_vertices[i].normal.z;
        vertexData[i * 9 + 6] = m_vertices[i].color.x;
        vertexData[i * 9 + 7] = m_vertices[i].color.y;
        vertexData[i * 9 + 8] = m_vertices[i].color.z;
    }

    // Indices from faces
    indices.clear();
    indices.reserve(m_faces.size() * 3);
    for (size_t fi = 0; fi < m_faces.size(); fi++) {
        int32_t he0 = m_faces[fi].halfEdge;
        int32_t he1 = m_halfEdges[he0].next;
        int32_t he2 = m_halfEdges[he1].next;

        // Origin of he0 = he2.vertex (since prev of he0 is he2)
        int32_t v0 = m_halfEdges[he2].vertex;
        int32_t v1 = m_halfEdges[he0].vertex;
        int32_t v2 = m_halfEdges[he1].vertex;

        indices.push_back(static_cast<uint32_t>(v0));
        indices.push_back(static_cast<uint32_t>(v1));
        indices.push_back(static_cast<uint32_t>(v2));
    }
}

AABB HalfEdgeMesh::computeBounds() const {
    AABB bounds;
    for (const auto& v : m_vertices) {
        bounds.expand(v.position);
    }
    return bounds;
}

// --- Mutation methods ---

int32_t HalfEdgeMesh::addVertex(const Vec3& position) {
    Vertex v;
    v.position = position;
    v.normal = {0, 1, 0};
    v.color = {1, 1, 1};
    v.halfEdge = -1;
    m_vertices.push_back(v);
    return static_cast<int32_t>(m_vertices.size() - 1);
}

int32_t HalfEdgeMesh::addFace(int32_t v0, int32_t v1, int32_t v2) {
    int32_t fi = static_cast<int32_t>(m_faces.size());
    int32_t baseHE = static_cast<int32_t>(m_halfEdges.size());

    // Create 3 half-edges
    HalfEdge he0, he1, he2;
    he0.vertex = v1; he0.next = baseHE + 1; he0.prev = baseHE + 2; he0.face = fi; he0.twin = -1;
    he1.vertex = v2; he1.next = baseHE + 2; he1.prev = baseHE;     he1.face = fi; he1.twin = -1;
    he2.vertex = v0; he2.next = baseHE;     he2.prev = baseHE + 1; he2.face = fi; he2.twin = -1;

    m_halfEdges.push_back(he0);
    m_halfEdges.push_back(he1);
    m_halfEdges.push_back(he2);

    Face f;
    f.halfEdge = baseHE;
    m_faces.push_back(f);

    // Update vertex half-edge references if not set
    if (m_vertices[v0].halfEdge == -1) m_vertices[v0].halfEdge = baseHE;
    if (m_vertices[v1].halfEdge == -1) m_vertices[v1].halfEdge = baseHE + 1;
    if (m_vertices[v2].halfEdge == -1) m_vertices[v2].halfEdge = baseHE + 2;

    return fi;
}

void HalfEdgeMesh::removeFace(int32_t faceIdx) {
    if (faceIdx < 0 || faceIdx >= static_cast<int32_t>(m_faces.size())) return;

    int32_t he0 = m_faces[faceIdx].halfEdge;
    if (he0 < 0) return; // already deleted

    int32_t he1 = m_halfEdges[he0].next;
    int32_t he2 = m_halfEdges[he1].next;

    // Unlink twins
    auto unlinkTwin = [this](int32_t heIdx) {
        int32_t tw = m_halfEdges[heIdx].twin;
        if (tw >= 0) {
            m_halfEdges[tw].twin = -1;
        }
    };
    unlinkTwin(he0);
    unlinkTwin(he1);
    unlinkTwin(he2);

    // Mark half-edges as deleted
    m_halfEdges[he0].face = -1;
    m_halfEdges[he1].face = -1;
    m_halfEdges[he2].face = -1;

    // Mark face as deleted
    m_faces[faceIdx].halfEdge = -1;
}

std::pair<int32_t, int32_t> HalfEdgeMesh::edgeVertices(int32_t heIdx) const {
    int32_t dest = m_halfEdges[heIdx].vertex;
    int32_t orig = heOrigin(heIdx);
    return {orig, dest};
}

int32_t HalfEdgeMesh::findHalfEdge(int32_t v0, int32_t v1) const {
    // Walk half-edges around v0 to find one pointing to v1
    int32_t startHE = m_vertices[v0].halfEdge;
    if (startHE < 0) return -1;

    int32_t he = startHE;
    do {
        if (m_halfEdges[he].vertex == v1) return he;
        int32_t prevHE = m_halfEdges[he].prev;
        if (prevHE < 0) break;
        he = m_halfEdges[prevHE].twin;
        if (he < 0) break;
    } while (he != startHE);

    return -1;
}

std::array<int32_t, 3> HalfEdgeMesh::faceVertices(int32_t faceIdx) const {
    if (faceIdx < 0 || faceIdx >= static_cast<int32_t>(m_faces.size()) ||
        m_faces[faceIdx].halfEdge < 0) {
        return {-1, -1, -1}; // deleted or invalid face
    }
    int32_t he0 = m_faces[faceIdx].halfEdge;
    if (he0 < 0 || he0 >= static_cast<int32_t>(m_halfEdges.size())) return {-1, -1, -1};
    int32_t he1 = m_halfEdges[he0].next;
    if (he1 < 0 || he1 >= static_cast<int32_t>(m_halfEdges.size())) return {-1, -1, -1};
    int32_t he2 = m_halfEdges[he1].next;
    if (he2 < 0 || he2 >= static_cast<int32_t>(m_halfEdges.size())) return {-1, -1, -1};
    return {m_halfEdges[he2].vertex, m_halfEdges[he0].vertex, m_halfEdges[he1].vertex};
}

void HalfEdgeMesh::relinkTwins() {
    // Build edge map and link twins
    std::unordered_map<std::pair<int32_t, int32_t>, int32_t, EdgeHash> edgeMap;

    for (int32_t i = 0; i < static_cast<int32_t>(m_halfEdges.size()); i++) {
        if (m_halfEdges[i].face < 0) continue; // deleted
        int32_t v0 = heOrigin(i);
        int32_t v1 = m_halfEdges[i].vertex;
        m_halfEdges[i].twin = -1; // reset
        edgeMap[{v0, v1}] = i;
    }

    for (auto& [edge, heIdx] : edgeMap) {
        auto twinIt = edgeMap.find({edge.second, edge.first});
        if (twinIt != edgeMap.end()) {
            m_halfEdges[heIdx].twin = twinIt->second;
        }
    }
}

void HalfEdgeMesh::compact() {
    // Build mapping from old indices to new
    std::vector<int32_t> vertMap(m_vertices.size(), -1);
    std::vector<int32_t> faceMap(m_faces.size(), -1);
    std::vector<int32_t> heMap(m_halfEdges.size(), -1);

    // Find which vertices are actually used by live faces
    std::vector<bool> vertUsed(m_vertices.size(), false);
    for (size_t fi = 0; fi < m_faces.size(); fi++) {
        if (m_faces[fi].halfEdge < 0) continue;
        int32_t he0 = m_faces[fi].halfEdge;
        for (int j = 0; j < 3; j++) {
            int32_t he = he0 + j;
            if (he < static_cast<int32_t>(m_halfEdges.size()) && m_halfEdges[he].vertex >= 0)
                vertUsed[m_halfEdges[he].vertex] = true;
        }
    }

    // Compact vertices - only keep used ones
    std::vector<Vertex> newVerts;
    for (size_t i = 0; i < m_vertices.size(); i++) {
        if (vertUsed[i]) {
            vertMap[i] = static_cast<int32_t>(newVerts.size());
            newVerts.push_back(m_vertices[i]);
        }
    }

    // Compact faces
    std::vector<Face> newFaces;
    std::vector<HalfEdge> newHEs;

    for (size_t fi = 0; fi < m_faces.size(); fi++) {
        if (m_faces[fi].halfEdge < 0) continue; // deleted

        int32_t oldHE0 = m_faces[fi].halfEdge;
        int32_t oldHE1 = m_halfEdges[oldHE0].next;
        int32_t oldHE2 = m_halfEdges[oldHE1].next;

        int32_t newFI = static_cast<int32_t>(newFaces.size());
        int32_t newBase = static_cast<int32_t>(newHEs.size());

        faceMap[fi] = newFI;
        heMap[oldHE0] = newBase;
        heMap[oldHE1] = newBase + 1;
        heMap[oldHE2] = newBase + 2;

        Face nf;
        nf.halfEdge = newBase;
        nf.normal = m_faces[fi].normal;
        newFaces.push_back(nf);

        // Copy half-edges (remap vertex indices)
        for (int j = 0; j < 3; j++) {
            int32_t oldHE = (j == 0) ? oldHE0 : (j == 1) ? oldHE1 : oldHE2;
            HalfEdge nhe;
            nhe.vertex = vertMap[m_halfEdges[oldHE].vertex];
            nhe.next = newBase + (j + 1) % 3;
            nhe.prev = newBase + (j + 2) % 3;
            nhe.face = newFI;
            nhe.twin = -1; // will be relinked
            newHEs.push_back(nhe);
        }
    }

    // Update vertex half-edge references
    for (auto& v : newVerts) {
        if (v.halfEdge >= 0 && v.halfEdge < static_cast<int32_t>(heMap.size())) {
            v.halfEdge = heMap[v.halfEdge];
        } else {
            v.halfEdge = -1;
        }
    }

    m_vertices = std::move(newVerts);
    m_faces = std::move(newFaces);
    m_halfEdges = std::move(newHEs);

    // Relink twins
    relinkTwins();

    // Fix vertex half-edge references that may now be -1
    for (size_t fi = 0; fi < m_faces.size(); fi++) {
        int32_t he0 = m_faces[fi].halfEdge;
        for (int j = 0; j < 3; j++) {
            int32_t he = he0 + j;
            int32_t orig = heOrigin(he);
            if (orig >= 0 && m_vertices[orig].halfEdge < 0) {
                m_vertices[orig].halfEdge = he;
            }
        }
    }
}

} // namespace sculpt
