#pragma once

#include "core/HalfEdgeMesh.h"
#include <vector>
#include <functional>

namespace sculpt {

struct Ray {
    Vec3 origin;
    Vec3 direction;

    Ray() = default;
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalized()) {}
};

struct RayHit {
    bool hit = false;
    float distance = 1e30f;
    int32_t faceIdx = -1;
    Vec3 position;
    Vec3 normal;
    float u = 0, v = 0; // barycentric coords
};

class BVH {
public:
    BVH() = default;

    void build(const HalfEdgeMesh& mesh);
    void rebuild(const HalfEdgeMesh& mesh);

    // Ray intersection
    RayHit raycast(const Ray& ray, const HalfEdgeMesh& mesh) const;

    // Find all vertices within a sphere
    std::vector<int32_t> findVerticesInSphere(const Vec3& center, float radius,
                                               const HalfEdgeMesh& mesh) const;

    bool isBuilt() const { return !m_nodes.empty(); }

private:
    struct BVHNode {
        AABB bounds;
        int32_t left = -1;   // left child or first face index
        int32_t right = -1;  // right child or face count
        bool isLeaf = false;
    };

    void buildRecursive(int32_t nodeIdx, std::vector<int32_t>& faceIndices,
                        int32_t start, int32_t end, const HalfEdgeMesh& mesh);

    bool rayIntersectAABB(const Ray& ray, const AABB& box, float& tMin) const;
    bool rayIntersectTriangle(const Ray& ray, const Vec3& v0, const Vec3& v1,
                              const Vec3& v2, float& t, float& u, float& v) const;

    void raycastNode(int32_t nodeIdx, const Ray& ray, const HalfEdgeMesh& mesh,
                     RayHit& closest) const;

    void findVerticesInSphereNode(int32_t nodeIdx, const Vec3& center, float radius,
                                   const HalfEdgeMesh& mesh,
                                   std::vector<bool>& visited,
                                   std::vector<int32_t>& result) const;

    std::vector<BVHNode> m_nodes;
    std::vector<int32_t> m_faceIndices;
    static constexpr int MAX_LEAF_FACES = 4;
};

} // namespace sculpt
