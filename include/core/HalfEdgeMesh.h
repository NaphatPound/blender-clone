#pragma once

#include <vector>
#include <cstdint>
#include <array>
#include <cmath>

namespace sculpt {

struct Vec3 {
    float x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator/(float s) const { return {x / s, y / s, z / s}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }

    float dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    float length() const { return std::sqrt(x * x + y * y + z * z); }
    float lengthSq() const { return x * x + y * y + z * z; }
    Vec3 normalized() const {
        float len = length();
        if (len < 1e-8f) return {0, 0, 0};
        return *this / len;
    }

    bool operator==(const Vec3& o) const {
        return std::abs(x - o.x) < 1e-6f &&
               std::abs(y - o.y) < 1e-6f &&
               std::abs(z - o.z) < 1e-6f;
    }
};

struct Vertex {
    Vec3 position;
    Vec3 normal;
    Vec3 color = Vec3(1.0f, 1.0f, 1.0f); // vertex color (default white)
    int32_t halfEdge = -1; // index to one outgoing half-edge
};

struct HalfEdge {
    int32_t vertex = -1;   // vertex this half-edge points to
    int32_t next = -1;     // next half-edge in the face loop
    int32_t prev = -1;     // previous half-edge in the face loop
    int32_t twin = -1;     // opposite half-edge
    int32_t face = -1;     // face this half-edge belongs to
};

struct Face {
    int32_t halfEdge = -1; // one half-edge of this face
    Vec3 normal;
};

struct AABB {
    Vec3 min;
    Vec3 max;

    AABB() : min(1e30f, 1e30f, 1e30f), max(-1e30f, -1e30f, -1e30f) {}
    AABB(const Vec3& mn, const Vec3& mx) : min(mn), max(mx) {}

    void expand(const Vec3& p) {
        if (p.x < min.x) min.x = p.x;
        if (p.y < min.y) min.y = p.y;
        if (p.z < min.z) min.z = p.z;
        if (p.x > max.x) max.x = p.x;
        if (p.y > max.y) max.y = p.y;
        if (p.z > max.z) max.z = p.z;
    }

    void expand(const AABB& other) {
        expand(other.min);
        expand(other.max);
    }

    Vec3 center() const { return (min + max) * 0.5f; }
    Vec3 size() const { return max - min; }

    bool contains(const Vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }

    bool intersects(const AABB& o) const {
        return min.x <= o.max.x && max.x >= o.min.x &&
               min.y <= o.max.y && max.y >= o.min.y &&
               min.z <= o.max.z && max.z >= o.min.z;
    }
};

class HalfEdgeMesh {
public:
    HalfEdgeMesh() = default;

    // Build mesh from indexed triangle data
    void buildFromTriangles(const std::vector<Vec3>& positions,
                            const std::vector<std::array<uint32_t, 3>>& triangles);

    // Accessors
    size_t vertexCount() const { return m_vertices.size(); }
    size_t faceCount() const { return m_faces.size(); }
    size_t halfEdgeCount() const { return m_halfEdges.size(); }

    const Vertex& vertex(int32_t idx) const { return m_vertices[idx]; }
    Vertex& vertex(int32_t idx) { return m_vertices[idx]; }

    const Face& face(int32_t idx) const { return m_faces[idx]; }
    Face& face(int32_t idx) { return m_faces[idx]; }

    const HalfEdge& halfEdge(int32_t idx) const { return m_halfEdges[idx]; }

    const std::vector<Vertex>& vertices() const { return m_vertices; }
    std::vector<Vertex>& vertices() { return m_vertices; }

    const std::vector<Face>& faces() const { return m_faces; }
    const std::vector<HalfEdge>& halfEdges() const { return m_halfEdges; }

    // Topology queries
    std::vector<int32_t> getVertexNeighbors(int32_t vertIdx) const;
    std::vector<int32_t> getVertexFaces(int32_t vertIdx) const;

    // Normal computation
    void computeFaceNormals();
    void computeVertexNormals();
    void recomputeNormals();

    // Get flat vertex/index arrays for rendering
    void getTriangleData(std::vector<float>& vertexData,
                         std::vector<uint32_t>& indices) const;

    // Bounding box
    AABB computeBounds() const;

    // --- Mutation methods for edit tools ---

    // Add a new vertex, return its index
    int32_t addVertex(const Vec3& position);

    // Add a triangular face from 3 vertex indices, returns face index
    int32_t addFace(int32_t v0, int32_t v1, int32_t v2);

    // Mark a face as deleted (call compact() after batch operations)
    void removeFace(int32_t faceIdx);

    // Check if a face is deleted
    bool isFaceDeleted(int32_t faceIdx) const { return m_faces[faceIdx].halfEdge < 0; }

    // Get origin vertex of a half-edge
    int32_t heOrigin(int32_t heIdx) const {
        return m_halfEdges[m_halfEdges[heIdx].prev].vertex;
    }

    // Get the two vertex indices of an edge from a half-edge
    std::pair<int32_t, int32_t> edgeVertices(int32_t heIdx) const;

    // Find the half-edge from v0 to v1, or -1
    int32_t findHalfEdge(int32_t v0, int32_t v1) const;

    // Get the 3 vertex indices of a face
    std::array<int32_t, 3> faceVertices(int32_t faceIdx) const;

    // Mutable half-edge access for topology surgery
    HalfEdge& halfEdgeMut(int32_t idx) { return m_halfEdges[idx]; }

    // Remove deleted faces and re-index everything
    void compact();

    // Rebuild twin links (useful after batch addFace calls)
    void relinkTwins();

private:
    std::vector<Vertex> m_vertices;
    std::vector<HalfEdge> m_halfEdges;
    std::vector<Face> m_faces;
};

} // namespace sculpt
