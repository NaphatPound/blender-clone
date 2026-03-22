#include "test_framework.h"
#include "sculpt/Brush.h"
#include "sculpt/SculptEngine.h"
#include "core/MeshGenerator.h"

using namespace sculpt;

void testBrushFalloff() {
    std::cout << "\n--- Brush Falloff Tests ---" << std::endl;

    BrushSettings settings;
    settings.radius = 1.0f;

    // Smooth falloff
    settings.falloff = FalloffType::Smooth;
    Brush smoothBrush(settings);
    test::checkFloat(smoothBrush.calculateFalloff(0.0f), 1.0f, "Smooth falloff at center");
    test::checkFloat(smoothBrush.calculateFalloff(1.0f), 0.0f, "Smooth falloff at edge");
    test::check(smoothBrush.calculateFalloff(0.5f) > 0.0f &&
                smoothBrush.calculateFalloff(0.5f) < 1.0f, "Smooth falloff midpoint in range");

    // Linear falloff
    settings.falloff = FalloffType::Linear;
    Brush linearBrush(settings);
    test::checkFloat(linearBrush.calculateFalloff(0.0f), 1.0f, "Linear falloff at center");
    test::checkFloat(linearBrush.calculateFalloff(0.5f), 0.5f, "Linear falloff at half radius");
    test::checkFloat(linearBrush.calculateFalloff(1.0f), 0.0f, "Linear falloff at edge");

    // Constant falloff
    settings.falloff = FalloffType::Constant;
    Brush constBrush(settings);
    test::checkFloat(constBrush.calculateFalloff(0.0f), 1.0f, "Constant falloff at center");
    test::checkFloat(constBrush.calculateFalloff(0.5f), 1.0f, "Constant falloff at half");
    test::checkFloat(constBrush.calculateFalloff(1.0f), 0.0f, "Constant falloff at edge");

    // Beyond radius
    test::checkFloat(smoothBrush.calculateFalloff(2.0f), 0.0f, "Falloff beyond radius is 0");
}

void testDrawBrush() {
    std::cout << "\n--- Draw Brush Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);

    // Record original position of vertex 0
    Vec3 origPos = sphere.vertex(0).position;

    BrushSettings settings;
    settings.type = BrushType::Draw;
    settings.radius = 0.5f;
    settings.strength = 0.1f;
    settings.falloff = FalloffType::Smooth;
    Brush brush(settings);

    // Find vertices near the top pole
    std::vector<int32_t> verts;
    Vec3 center = sphere.vertex(0).position;
    for (size_t i = 0; i < sphere.vertexCount(); i++) {
        float dist = (sphere.vertex(static_cast<int32_t>(i)).position - center).length();
        if (dist < settings.radius) {
            verts.push_back(static_cast<int32_t>(i));
        }
    }

    brush.applyDraw(sphere, verts, center, Vec3(0, 1, 0));

    // Vertex should have moved outward along its normal
    Vec3 newPos = sphere.vertex(0).position;
    float displacement = (newPos - origPos).length();
    test::check(displacement > 0.0f, "Draw brush displaces vertices");
}

void testSmoothBrush() {
    std::cout << "\n--- Smooth Brush Tests ---" << std::endl;

    // Create a plane and spike one vertex
    HalfEdgeMesh plane = MeshGenerator::createPlane(2.0f, 4);

    // Spike center vertex upward
    int32_t centerVert = 12; // center of 5x5 grid
    plane.vertex(centerVert).position.y = 1.0f;
    plane.recomputeNormals();

    float origY = plane.vertex(centerVert).position.y;

    BrushSettings settings;
    settings.type = BrushType::Smooth;
    settings.radius = 2.0f;
    settings.strength = 0.5f;
    settings.falloff = FalloffType::Smooth;
    Brush brush(settings);

    std::vector<int32_t> verts;
    Vec3 center = plane.vertex(centerVert).position;
    for (size_t i = 0; i < plane.vertexCount(); i++) {
        float dist = (plane.vertex(static_cast<int32_t>(i)).position - center).length();
        if (dist < settings.radius) {
            verts.push_back(static_cast<int32_t>(i));
        }
    }

    brush.applySmooth(plane, verts, center);

    float newY = plane.vertex(centerVert).position.y;
    test::check(newY < origY, "Smooth brush reduces spike (smoothed vertex moved toward neighbors)");
}

void testGrabBrush() {
    std::cout << "\n--- Grab Brush Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);
    Vec3 origPos = sphere.vertex(0).position;

    BrushSettings settings;
    settings.type = BrushType::Grab;
    settings.radius = 0.5f;
    Brush brush(settings);

    std::vector<int32_t> verts = {0};
    Vec3 delta(0.5f, 0, 0);
    brush.applyGrab(sphere, verts, sphere.vertex(0).position, delta);

    Vec3 newPos = sphere.vertex(0).position;
    test::check(newPos.x > origPos.x, "Grab brush moves vertex in delta direction");
}

void testSculptEngine() {
    std::cout << "\n--- Sculpt Engine Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);
    Vec3 origTopPos = sphere.vertex(0).position;

    SculptEngine engine;
    engine.setMesh(&sphere);

    test::check(engine.bvh().isBuilt(), "Engine builds BVH on setMesh");

    // Perform a draw stroke near the top
    engine.brush().settings().type = BrushType::Draw;
    engine.brush().settings().radius = 0.3f;
    engine.brush().settings().strength = 0.1f;

    bool result = engine.stroke(Vec3(0, 1, 0), Vec3(0, 1, 0));
    test::check(result, "Engine stroke returns true when hitting vertices");

    // Verify vertices moved
    Vec3 newTopPos = sphere.vertex(0).position;
    test::check((newTopPos - origTopPos).length() > 0, "Engine stroke modifies mesh");

    // Raycast test
    Ray ray(Vec3(0, 0, 5), Vec3(0, 0, -1));
    RayHit hit = engine.raycast(ray);
    test::check(hit.hit, "Engine raycast hits mesh");
}

void testInvertBrush() {
    std::cout << "\n--- Invert Brush Tests ---" << std::endl;

    HalfEdgeMesh sphere = MeshGenerator::createSphere(1.0f, 16, 8);
    Vec3 origPos = sphere.vertex(0).position;
    float origDist = origPos.length();

    BrushSettings settings;
    settings.type = BrushType::Draw;
    settings.radius = 0.5f;
    settings.strength = 0.1f;
    settings.invert = true;
    Brush brush(settings);

    std::vector<int32_t> verts = {0};
    brush.applyDraw(sphere, verts, sphere.vertex(0).position, Vec3(0, 1, 0));

    float newDist = sphere.vertex(0).position.length();
    test::check(newDist < origDist, "Inverted draw brush pushes vertex inward");
}

int main() {
    std::cout << "=== Sculpt Tests ===" << std::endl;

    testBrushFalloff();
    testDrawBrush();
    testSmoothBrush();
    testGrabBrush();
    testSculptEngine();
    testInvertBrush();

    return test::report("Sculpt");
}
