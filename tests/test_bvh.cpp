#include "test_framework.h"
#include "core/BVH.h"
#include "core/MeshGenerator.h"

using namespace sculpt;

void testBVHBuild() {
    std::cout << "\n--- BVH Build Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);
    BVH bvh;
    bvh.build(sphere);

    test::check(bvh.isBuilt(), "BVH builds successfully on sphere");
}

void testBVHRaycast() {
    std::cout << "\n--- BVH Raycast Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 32, 16);
    BVH bvh;
    bvh.build(sphere);

    // Ray from front, should hit sphere
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
    RayHit hit = bvh.raycast(ray, sphere);

    test::check(hit.hit, "Ray hits sphere from front");
    test::check(hit.distance > 0, "Hit distance is positive");
    test::checkFloat(hit.position.z, 1.0f, "Hit position near sphere surface z", 0.15f);

    // Ray from behind, opposite direction - should miss
    Ray missRay(Vec3(0, 0, 5), Vec3(0, 0, 1));
    RayHit missHit = bvh.raycast(missRay, sphere);
    test::check(!missHit.hit, "Ray misses sphere (wrong direction)");

    // Ray from side
    Ray sideRay(Vec3(5, 0, 0), Vec3(-1, 0, 0));
    RayHit sideHit = bvh.raycast(sideRay, sphere);
    test::check(sideHit.hit, "Ray hits sphere from side");
    test::checkFloat(sideHit.position.x, 1.0f, "Side hit near sphere surface x", 0.15f);

    // Ray that misses completely
    Ray farRay(Vec3(5, 5, 5), Vec3(1, 1, 1));
    RayHit farHit = bvh.raycast(farRay, sphere);
    test::check(!farHit.hit, "Ray misses sphere (pointing away)");
}

void testBVHSphereQuery() {
    std::cout << "\n--- BVH Sphere Query Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);
    BVH bvh;
    bvh.build(sphere);

    // Query near the top pole - should find some vertices
    auto verts = bvh.findVerticesInSphere(Vec3(0, 1, 0), 0.5f, sphere);
    test::check(!verts.empty(), "Found vertices near top pole");

    // Query far away - should find nothing
    auto farVerts = bvh.findVerticesInSphere(Vec3(10, 10, 10), 0.5f, sphere);
    test::check(farVerts.empty(), "No vertices found far from mesh");

    // Query with large radius - should find all vertices
    auto allVerts = bvh.findVerticesInSphere(Vec3(0, 0, 0), 2.0f, sphere);
    test::check(allVerts.size() == sphere.vertexCount(),
                "Large sphere query finds all vertices");
}

void testBVHCube() {
    std::cout << "\n--- BVH Cube Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    BVH bvh;
    bvh.build(cube);
    test::check(bvh.isBuilt(), "BVH builds on cube");

    // Ray straight at front face
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
    RayHit hit = bvh.raycast(ray, cube);
    test::check(hit.hit, "Ray hits cube front face");
    test::checkFloat(hit.position.z, 1.0f, "Cube hit at z=1", 0.01f);
}

void testBVHRebuild() {
    std::cout << "\n--- BVH Rebuild Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 8, 4);
    BVH bvh;
    bvh.build(sphere);

    // Modify mesh
    sphere.vertex(0).position = Vec3(0, 2, 0);
    sphere.recomputeNormals();

    // Rebuild
    bvh.rebuild(sphere);
    test::check(bvh.isBuilt(), "BVH rebuilds after mesh modification");

    // Should still be able to raycast
    Ray ray(Vec3(0, 5, 0), Vec3(0, -1, 0));
    RayHit hit = bvh.raycast(ray, sphere);
    test::check(hit.hit, "Raycast works after rebuild");
}

int main() {
    std::cout << "=== BVH Tests ===" << std::endl;

    testBVHBuild();
    testBVHRaycast();
    testBVHSphereQuery();
    testBVHCube();
    testBVHRebuild();

    return test::report("BVH");
}
