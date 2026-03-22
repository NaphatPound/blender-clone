#include "test_framework.h"
#include "sculpt/Raycaster.h"

using namespace sculpt;

void testRaySphereIntersection() {
    std::cout << "\n--- Ray-Sphere Intersection Tests ---" << std::endl;

    // Ray hitting sphere
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
    float t;
    bool hit = Raycaster::raySphereIntersect(ray, Vec3(0, 0, 0), 1.0f, t);
    test::check(hit, "Ray hits sphere");
    test::checkFloat(t, 4.0f, "Hit distance is correct", 0.01f);

    // Ray missing sphere
    Ray missRay(Vec3(5, 5, 5), Vec3(1, 0, 0));
    bool miss = Raycaster::raySphereIntersect(missRay, Vec3(0, 0, 0), 1.0f, t);
    test::check(!miss, "Ray misses sphere");

    // Ray origin inside sphere
    Ray insideRay(Vec3(0, 0, 0), Vec3(0, 0, 1));
    bool insideHit = Raycaster::raySphereIntersect(insideRay, Vec3(0, 0, 0), 1.0f, t);
    test::check(insideHit, "Ray from inside sphere hits");
    test::checkFloat(t, 1.0f, "Inside ray distance is radius", 0.01f);
}

void testRayAABBIntersection() {
    std::cout << "\n--- Ray-AABB Intersection Tests ---" << std::endl;

    AABB box(Vec3(-1, -1, -1), Vec3(1, 1, 1));

    // Direct hit
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
    float tMin;
    bool hit = Raycaster::rayAABBIntersect(ray, box, tMin);
    test::check(hit, "Ray hits AABB");
    test::checkFloat(tMin, 4.0f, "AABB hit distance", 0.01f);

    // Miss
    Ray missRay(Vec3(5, 5, 5), Vec3(1, 1, 1));
    bool miss = Raycaster::rayAABBIntersect(missRay, box, tMin);
    test::check(!miss, "Ray misses AABB");

    // Edge-aligned ray
    Ray edgeRay(Vec3(1, 0, 5), Vec3(0, 0, -1));
    bool edgeHit = Raycaster::rayAABBIntersect(edgeRay, box, tMin);
    test::check(edgeHit, "Edge-aligned ray hits AABB");
}

void testScreenToWorldRay() {
    std::cout << "\n--- Screen-to-World Ray Tests ---" << std::endl;

    // Simple identity-like matrices for testing
    // View matrix: looking down -Z
    float view[16] = {
        1, 0,  0, 0,
        0, 1,  0, 0,
        0, 0, -1, 0,
        0, 0, -5, 1  // camera at z=5
    };

    // Projection matrix: simple perspective
    float proj[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, -1, -1,
        0, 0, -0.2f, 0
    };

    // Center of screen
    Ray ray = Raycaster::screenToWorldRay(400, 300, 800, 600, view, proj);

    // Ray should originate from camera and point roughly forward
    test::check(ray.direction.length() > 0.9f, "Screen ray has unit direction");
    // Center of screen should produce a ray mostly along Z axis
    test::check(std::abs(ray.direction.x) < 0.1f, "Center ray X component near 0");
    test::check(std::abs(ray.direction.y) < 0.1f, "Center ray Y component near 0");
}

int main() {
    std::cout << "=== Raycaster Tests ===" << std::endl;

    testRaySphereIntersection();
    testRayAABBIntersection();
    testScreenToWorldRay();

    return test::report("Raycaster");
}
