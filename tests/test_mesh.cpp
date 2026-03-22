#include "test_framework.h"
#include "core/HalfEdgeMesh.h"
#include "core/MeshGenerator.h"

using namespace sculpt;

void testVec3() {
    std::cout << "\n--- Vec3 Tests ---" << std::endl;

    Vec3 a(1, 2, 3);
    Vec3 b(4, 5, 6);

    Vec3 sum = a + b;
    test::check(sum.x == 5.0f && sum.y == 7.0f && sum.z == 9.0f, "Vec3 addition");

    Vec3 diff = b - a;
    test::check(diff.x == 3.0f && diff.y == 3.0f && diff.z == 3.0f, "Vec3 subtraction");

    Vec3 scaled = a * 2.0f;
    test::check(scaled.x == 2.0f && scaled.y == 4.0f && scaled.z == 6.0f, "Vec3 scalar multiply");

    float dot = a.dot(b);
    test::checkFloat(dot, 32.0f, "Vec3 dot product");

    Vec3 cross = Vec3(1,0,0).cross(Vec3(0,1,0));
    test::check(cross.x == 0.0f && cross.y == 0.0f && cross.z == 1.0f, "Vec3 cross product");

    Vec3 n = Vec3(3, 0, 0).normalized();
    test::checkFloat(n.length(), 1.0f, "Vec3 normalize length");
    test::checkFloat(n.x, 1.0f, "Vec3 normalize direction");

    Vec3 zero(0, 0, 0);
    Vec3 nz = zero.normalized();
    test::checkFloat(nz.length(), 0.0f, "Vec3 normalize zero vector");
}

void testAABB() {
    std::cout << "\n--- AABB Tests ---" << std::endl;

    AABB box;
    box.expand(Vec3(-1, -1, -1));
    box.expand(Vec3(1, 1, 1));

    test::check(box.contains(Vec3(0, 0, 0)), "AABB contains center");
    test::check(box.contains(Vec3(1, 1, 1)), "AABB contains corner");
    test::check(!box.contains(Vec3(2, 0, 0)), "AABB does not contain outside point");

    Vec3 center = box.center();
    test::checkFloat(center.x, 0.0f, "AABB center x");
    test::checkFloat(center.y, 0.0f, "AABB center y");

    AABB other(Vec3(0.5f, 0.5f, 0.5f), Vec3(2, 2, 2));
    test::check(box.intersects(other), "AABB overlap intersection");

    AABB far(Vec3(5, 5, 5), Vec3(6, 6, 6));
    test::check(!box.intersects(far), "AABB non-overlapping");
}

void testTriangleMesh() {
    std::cout << "\n--- Triangle Mesh Tests ---" << std::endl;

    // Create a simple triangle
    std::vector<Vec3> positions = {{0,0,0}, {1,0,0}, {0,1,0}};
    std::vector<std::array<uint32_t, 3>> triangles = {{0, 1, 2}};

    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(positions, triangles);

    test::check(mesh.vertexCount() == 3, "Single triangle vertex count");
    test::check(mesh.faceCount() == 1, "Single triangle face count");
    test::check(mesh.halfEdgeCount() == 3, "Single triangle half-edge count");

    // Check face normal (should point in +Z)
    Vec3 fn = mesh.face(0).normal;
    test::checkFloat(fn.z, 1.0f, "Triangle face normal z");
}

void testCubeGeneration() {
    std::cout << "\n--- Cube Generation Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);

    test::check(cube.vertexCount() == 8, "Cube vertex count");
    test::check(cube.faceCount() == 12, "Cube face count (6 sides * 2 triangles)");

    AABB bounds = cube.computeBounds();
    test::checkFloat(bounds.min.x, -1.0f, "Cube bounds min x");
    test::checkFloat(bounds.max.x, 1.0f, "Cube bounds max x");
}

void testSphereGeneration() {
    std::cout << "\n--- Sphere Generation Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);

    test::check(sphere.vertexCount() > 0, "Sphere has vertices");
    test::check(sphere.faceCount() > 0, "Sphere has faces");

    // Check that all vertices are approximately on the sphere surface
    bool allOnSurface = true;
    for (size_t i = 0; i < sphere.vertexCount(); i++) {
        float dist = sphere.vertex(static_cast<int32_t>(i)).position.length();
        if (std::abs(dist - 1.0f) > 0.01f) {
            allOnSurface = false;
            break;
        }
    }
    test::check(allOnSurface, "Sphere vertices on unit sphere surface");

    AABB bounds = sphere.computeBounds();
    test::check(bounds.min.x >= -1.01f && bounds.max.x <= 1.01f, "Sphere bounds within radius");
}

void testPlaneGeneration() {
    std::cout << "\n--- Plane Generation Tests ---" << std::endl;

    HalfEdgeMesh plane = MeshGenerator::createPlane(2.0f, 4);

    test::check(plane.vertexCount() == 25, "Plane 4x4 subdivision vertex count (5x5=25)");
    test::check(plane.faceCount() == 32, "Plane 4x4 subdivision face count (4*4*2=32)");
}

void testVertexNeighbors() {
    std::cout << "\n--- Vertex Neighbor Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(1.0f);

    // Each vertex of a cube should have at least some neighbors
    auto neighbors = cube.getVertexNeighbors(0);
    test::check(!neighbors.empty(), "Cube vertex has neighbors");

    auto faces = cube.getVertexFaces(0);
    test::check(!faces.empty(), "Cube vertex has adjacent faces");
}

void testTriangleData() {
    std::cout << "\n--- Triangle Data Export Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(1.0f);

    std::vector<float> vertexData;
    std::vector<uint32_t> indices;
    cube.getTriangleData(vertexData, indices);

    test::check(vertexData.size() == cube.vertexCount() * 9, "Vertex data size (pos+normal+color per vertex)");
    test::check(indices.size() == cube.faceCount() * 3, "Index count (3 per triangle)");
}

int main() {
    std::cout << "=== Mesh Tests ===" << std::endl;

    testVec3();
    testAABB();
    testTriangleMesh();
    testCubeGeneration();
    testSphereGeneration();
    testPlaneGeneration();
    testVertexNeighbors();
    testTriangleData();

    return test::report("Mesh");
}
