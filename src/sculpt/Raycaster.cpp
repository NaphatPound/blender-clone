#include "sculpt/Raycaster.h"
#include <cmath>
#include <algorithm>

namespace sculpt {

Ray Raycaster::screenToWorldRay(float screenX, float screenY,
                                  float screenWidth, float screenHeight,
                                  const float* viewMatrix,
                                  const float* projMatrix) {
    // Convert screen coords to NDC (-1 to 1)
    float ndcX = (2.0f * screenX / screenWidth) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY / screenHeight);

    // Invert projection matrix to get view-space ray
    // For a perspective projection, we can extract the ray direction directly
    float tanHalfFov = 1.0f / projMatrix[5]; // 1/proj[1][1]
    float aspect = projMatrix[5] / projMatrix[0]; // proj[1][1]/proj[0][0]

    Vec3 rayDirView = {
        ndcX * tanHalfFov * aspect,
        ndcY * tanHalfFov,
        -1.0f
    };

    // Invert view matrix (assuming orthonormal rotation + translation)
    // View matrix is column-major: M[col*4+row]
    // Column 0 = (right.x, up.x, -fwd.x), Column 1 = (right.y, up.y, -fwd.y), etc.
    // To go view->world, multiply direction by R^T (transpose of rotation part)
    // R^T * v: row i of result = column i of stored matrix dotted with v
    float tx = viewMatrix[12], ty = viewMatrix[13], tz = viewMatrix[14];

    // R^T * v = for each output component, read down a COLUMN of the view matrix
    Vec3 rayDirWorld = {
        viewMatrix[0] * rayDirView.x + viewMatrix[1] * rayDirView.y + viewMatrix[2] * rayDirView.z,
        viewMatrix[4] * rayDirView.x + viewMatrix[5] * rayDirView.y + viewMatrix[6] * rayDirView.z,
        viewMatrix[8] * rayDirView.x + viewMatrix[9] * rayDirView.y + viewMatrix[10] * rayDirView.z
    };

    // Camera position = -R^T * t (same column-reading pattern)
    Vec3 camPos = {
        -(viewMatrix[0] * tx + viewMatrix[1] * ty + viewMatrix[2] * tz),
        -(viewMatrix[4] * tx + viewMatrix[5] * ty + viewMatrix[6] * tz),
        -(viewMatrix[8] * tx + viewMatrix[9] * ty + viewMatrix[10] * tz)
    };

    return Ray(camPos, rayDirWorld.normalized());
}

bool Raycaster::raySphereIntersect(const Ray& ray, const Vec3& center,
                                     float radius, float& t) {
    Vec3 oc = ray.origin - center;
    float a = ray.direction.dot(ray.direction);
    float b = 2.0f * oc.dot(ray.direction);
    float c = oc.dot(oc) - radius * radius;
    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0) return false;

    float sqrtD = std::sqrt(discriminant);
    float t0 = (-b - sqrtD) / (2.0f * a);
    float t1 = (-b + sqrtD) / (2.0f * a);

    if (t0 > 0) { t = t0; return true; }
    if (t1 > 0) { t = t1; return true; }
    return false;
}

bool Raycaster::rayAABBIntersect(const Ray& ray, const AABB& box, float& tMin) {
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

} // namespace sculpt
