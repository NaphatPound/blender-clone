#pragma once

#include "core/HalfEdgeMesh.h"
#include "core/BVH.h"

namespace sculpt {

class Raycaster {
public:
    // Create a ray from screen coordinates and camera matrices
    static Ray screenToWorldRay(float screenX, float screenY,
                                 float screenWidth, float screenHeight,
                                 const float* viewMatrix,
                                 const float* projMatrix);

    // Simple ray-sphere intersection test
    static bool raySphereIntersect(const Ray& ray, const Vec3& center,
                                    float radius, float& t);

    // Ray-AABB intersection
    static bool rayAABBIntersect(const Ray& ray, const AABB& box, float& tMin);
};

} // namespace sculpt
