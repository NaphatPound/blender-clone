#include "core/BVH.h"
#include <algorithm>
#include <numeric>

namespace sculpt {

void BVH::build(const HalfEdgeMesh& mesh) {
    if (mesh.faceCount() == 0) return;

    m_nodes.clear();
    m_faceIndices.resize(mesh.faceCount());
    std::iota(m_faceIndices.begin(), m_faceIndices.end(), 0);

    // Reserve space for nodes (roughly 2*N for a binary tree)
    m_nodes.reserve(mesh.faceCount() * 2);

    // Create root node
    m_nodes.push_back(BVHNode{});
    buildRecursive(0, m_faceIndices, 0, static_cast<int32_t>(m_faceIndices.size()), mesh);
}

void BVH::rebuild(const HalfEdgeMesh& mesh) {
    build(mesh);
}

void BVH::buildRecursive(int32_t nodeIdx, std::vector<int32_t>& faceIndices,
                          int32_t start, int32_t end, const HalfEdgeMesh& mesh) {
    // Compute bounds for this node (use index, not reference — vector may reallocate later)
    m_nodes[nodeIdx].bounds = AABB();
    for (int32_t i = start; i < end; i++) {
        int32_t fi = faceIndices[i];
        int32_t he0 = mesh.face(fi).halfEdge;
        int32_t he1 = mesh.halfEdge(he0).next;
        int32_t he2 = mesh.halfEdge(he1).next;

        m_nodes[nodeIdx].bounds.expand(mesh.vertex(mesh.halfEdge(he2).vertex).position);
        m_nodes[nodeIdx].bounds.expand(mesh.vertex(mesh.halfEdge(he0).vertex).position);
        m_nodes[nodeIdx].bounds.expand(mesh.vertex(mesh.halfEdge(he1).vertex).position);
    }

    int32_t count = end - start;
    if (count <= MAX_LEAF_FACES) {
        m_nodes[nodeIdx].isLeaf = true;
        m_nodes[nodeIdx].left = start;
        m_nodes[nodeIdx].right = count;
        return;
    }

    // Find longest axis to split
    Vec3 size = m_nodes[nodeIdx].bounds.size();
    int axis = 0;
    if (size.y > size.x) axis = 1;
    if (size.z > (axis == 0 ? size.x : size.y)) axis = 2;

    // Sort faces by centroid along split axis
    std::sort(faceIndices.begin() + start, faceIndices.begin() + end,
              [&](int32_t a, int32_t b) {
                  auto getCentroid = [&](int32_t fi) -> float {
                      int32_t he0 = mesh.face(fi).halfEdge;
                      int32_t he1 = mesh.halfEdge(he0).next;
                      int32_t he2 = mesh.halfEdge(he1).next;
                      Vec3 c = (mesh.vertex(mesh.halfEdge(he2).vertex).position +
                                mesh.vertex(mesh.halfEdge(he0).vertex).position +
                                mesh.vertex(mesh.halfEdge(he1).vertex).position) / 3.0f;
                      if (axis == 0) return c.x;
                      if (axis == 1) return c.y;
                      return c.z;
                  };
                  return getCentroid(a) < getCentroid(b);
              });

    int32_t mid = (start + end) / 2;

    // Create child nodes
    // NOTE: push_back may reallocate, invalidating `node` reference — use nodeIdx after
    int32_t leftIdx = static_cast<int32_t>(m_nodes.size());
    m_nodes.push_back(BVHNode{});
    int32_t rightIdx = static_cast<int32_t>(m_nodes.size());
    m_nodes.push_back(BVHNode{});

    // Re-fetch node reference after push_back (vector may have reallocated)
    m_nodes[nodeIdx].left = leftIdx;
    m_nodes[nodeIdx].right = rightIdx;
    m_nodes[nodeIdx].isLeaf = false;

    buildRecursive(leftIdx, faceIndices, start, mid, mesh);
    buildRecursive(rightIdx, faceIndices, mid, end, mesh);
}

bool BVH::rayIntersectAABB(const Ray& ray, const AABB& box, float& tMin) const {
    float tmax = 1e30f;
    tMin = -1e30f;

    for (int i = 0; i < 3; i++) {
        float d = (i == 0) ? ray.direction.x : (i == 1) ? ray.direction.y : ray.direction.z;
        float o = (i == 0) ? ray.origin.x : (i == 1) ? ray.origin.y : ray.origin.z;
        float bmin = (i == 0) ? box.min.x : (i == 1) ? box.min.y : box.min.z;
        float bmax = (i == 0) ? box.max.x : (i == 1) ? box.max.y : box.max.z;

        if (std::abs(d) < 1e-8f) {
            if (o < bmin || o > bmax) return false;
        } else {
            float invD = 1.0f / d;
            float t1 = (bmin - o) * invD;
            float t2 = (bmax - o) * invD;
            if (t1 > t2) std::swap(t1, t2);
            tMin = std::max(tMin, t1);
            tmax = std::min(tmax, t2);
            if (tMin > tmax) return false;
        }
    }
    return tmax >= 0;
}

bool BVH::rayIntersectTriangle(const Ray& ray, const Vec3& v0, const Vec3& v1,
                                const Vec3& v2, float& t, float& u, float& v) const {
    // Möller–Trumbore intersection algorithm
    Vec3 edge1 = v1 - v0;
    Vec3 edge2 = v2 - v0;
    Vec3 h = ray.direction.cross(edge2);
    float a = edge1.dot(h);

    if (std::abs(a) < 1e-8f) return false;

    float f = 1.0f / a;
    Vec3 s = ray.origin - v0;
    u = f * s.dot(h);
    if (u < 0.0f || u > 1.0f) return false;

    Vec3 q = s.cross(edge1);
    v = f * ray.direction.dot(q);
    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * edge2.dot(q);
    return t > 1e-6f;
}

void BVH::raycastNode(int32_t nodeIdx, const Ray& ray, const HalfEdgeMesh& mesh,
                       RayHit& closest) const {
    const auto& node = m_nodes[nodeIdx];

    float tBox;
    if (!rayIntersectAABB(ray, node.bounds, tBox)) return;
    if (tBox > closest.distance) return;

    if (node.isLeaf) {
        for (int32_t i = node.left; i < node.left + node.right; i++) {
            int32_t fi = m_faceIndices[i];
            int32_t he0 = mesh.face(fi).halfEdge;
            int32_t he1 = mesh.halfEdge(he0).next;
            int32_t he2 = mesh.halfEdge(he1).next;

            const Vec3& p0 = mesh.vertex(mesh.halfEdge(he2).vertex).position;
            const Vec3& p1 = mesh.vertex(mesh.halfEdge(he0).vertex).position;
            const Vec3& p2 = mesh.vertex(mesh.halfEdge(he1).vertex).position;

            float t, u, v;
            if (rayIntersectTriangle(ray, p0, p1, p2, t, u, v)) {
                if (t < closest.distance) {
                    closest.hit = true;
                    closest.distance = t;
                    closest.faceIdx = fi;
                    closest.position = ray.origin + ray.direction * t;
                    closest.normal = mesh.face(fi).normal;
                    closest.u = u;
                    closest.v = v;
                }
            }
        }
        return;
    }

    raycastNode(node.left, ray, mesh, closest);
    raycastNode(node.right, ray, mesh, closest);
}

RayHit BVH::raycast(const Ray& ray, const HalfEdgeMesh& mesh) const {
    RayHit result;
    if (m_nodes.empty()) return result;
    raycastNode(0, ray, mesh, result);
    return result;
}

void BVH::findVerticesInSphereNode(int32_t nodeIdx, const Vec3& center, float radius,
                                     const HalfEdgeMesh& mesh,
                                     std::vector<bool>& visited,
                                     std::vector<int32_t>& result) const {
    const auto& node = m_nodes[nodeIdx];

    // Check if sphere intersects this AABB
    // Find closest point on AABB to sphere center
    Vec3 closest;
    closest.x = std::max(node.bounds.min.x, std::min(center.x, node.bounds.max.x));
    closest.y = std::max(node.bounds.min.y, std::min(center.y, node.bounds.max.y));
    closest.z = std::max(node.bounds.min.z, std::min(center.z, node.bounds.max.z));

    float distSq = (closest - center).lengthSq();
    if (distSq > radius * radius) return;

    if (node.isLeaf) {
        float radiusSq = radius * radius;
        for (int32_t i = node.left; i < node.left + node.right; i++) {
            int32_t fi = m_faceIndices[i];
            int32_t he0 = mesh.face(fi).halfEdge;
            int32_t he1 = mesh.halfEdge(he0).next;
            int32_t he2 = mesh.halfEdge(he1).next;

            int32_t verts[3] = {
                mesh.halfEdge(he2).vertex,
                mesh.halfEdge(he0).vertex,
                mesh.halfEdge(he1).vertex
            };

            for (int j = 0; j < 3; j++) {
                int32_t vi = verts[j];
                if (!visited[vi]) {
                    float d = (mesh.vertex(vi).position - center).lengthSq();
                    if (d <= radiusSq) {
                        visited[vi] = true;
                        result.push_back(vi);
                    }
                }
            }
        }
        return;
    }

    findVerticesInSphereNode(node.left, center, radius, mesh, visited, result);
    findVerticesInSphereNode(node.right, center, radius, mesh, visited, result);
}

std::vector<int32_t> BVH::findVerticesInSphere(const Vec3& center, float radius,
                                                 const HalfEdgeMesh& mesh) const {
    std::vector<int32_t> result;
    if (m_nodes.empty()) return result;

    std::vector<bool> visited(mesh.vertexCount(), false);
    findVerticesInSphereNode(0, center, radius, mesh, visited, result);
    return result;
}

} // namespace sculpt
