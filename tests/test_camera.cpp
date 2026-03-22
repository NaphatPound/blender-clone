#include "test_framework.h"
#include "render/Camera.h"

using namespace sculpt;

void testCameraDefaults() {
    std::cout << "\n--- Camera Default Tests ---" << std::endl;

    Camera cam;
    Vec3 pos = cam.getPosition();

    test::check(pos.length() > 0, "Camera has non-zero default position");
    test::checkFloat(cam.getFov(), 45.0f, "Camera default FOV");
    test::checkFloat(cam.getNearPlane(), 0.1f, "Camera default near plane");
    test::checkFloat(cam.getFarPlane(), 100.0f, "Camera default far plane");
}

void testCameraOrbit() {
    std::cout << "\n--- Camera Orbit Tests ---" << std::endl;

    Camera cam;
    Vec3 before = cam.getPosition();

    cam.orbit(100.0f, 0);
    Vec3 after = cam.getPosition();

    test::check((after - before).length() > 0.01f, "Orbit changes camera position");

    // Distance should remain the same
    float distBefore = before.length(); // target is at origin
    float distAfter = after.length();
    test::checkFloat(distAfter, distBefore, "Orbit preserves distance", 0.01f);
}

void testCameraZoom() {
    std::cout << "\n--- Camera Zoom Tests ---" << std::endl;

    Camera cam;
    cam.setDistance(5.0f);
    float origDist = cam.getPosition().length();

    cam.zoom(1.0f); // zoom in
    float newDist = cam.getPosition().length();
    test::check(newDist < origDist, "Zoom in reduces distance");

    cam.zoom(-1.0f); // zoom out
    float zoomOutDist = cam.getPosition().length();
    test::check(zoomOutDist > newDist, "Zoom out increases distance");
}

void testCameraPan() {
    std::cout << "\n--- Camera Pan Tests ---" << std::endl;

    Camera cam;
    Vec3 origTarget = cam.getTarget();

    cam.pan(100.0f, 0);
    Vec3 newTarget = cam.getTarget();
    test::check((newTarget - origTarget).length() > 0.01f, "Pan changes target");
}

void testViewMatrix() {
    std::cout << "\n--- View Matrix Tests ---" << std::endl;

    Camera cam;
    auto view = cam.getViewMatrix();

    // Last row should be [0, 0, 0, 1] for an affine transform
    test::checkFloat(view[3], 0.0f, "View matrix [3] = 0");
    test::checkFloat(view[7], 0.0f, "View matrix [7] = 0");
    test::checkFloat(view[11], 0.0f, "View matrix [11] = 0");
    test::checkFloat(view[15], 1.0f, "View matrix [15] = 1");
}

void testProjectionMatrix() {
    std::cout << "\n--- Projection Matrix Tests ---" << std::endl;

    Camera cam;
    auto proj = cam.getProjectionMatrix(16.0f / 9.0f);

    // Perspective projection: [11] should be -1
    test::checkFloat(proj[11], -1.0f, "Projection matrix [11] = -1 (perspective)");
    test::check(proj[0] > 0, "Projection matrix [0] > 0");
    test::check(proj[5] > 0, "Projection matrix [5] > 0");
}

int main() {
    std::cout << "=== Camera Tests ===" << std::endl;

    testCameraDefaults();
    testCameraOrbit();
    testCameraZoom();
    testCameraPan();
    testViewMatrix();
    testProjectionMatrix();

    return test::report("Camera");
}
