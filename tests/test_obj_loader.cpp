#include "test_framework.h"
#include "core/ObjLoader.h"
#include "core/MeshGenerator.h"

using namespace sculpt;

void testLoadFromString() {
    std::cout << "\n--- OBJ Load From String Tests ---" << std::endl;

    std::string objData =
        "# Simple triangle\n"
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3\n";

    HalfEdgeMesh mesh;
    bool result = ObjLoader::loadFromString(objData, mesh);

    test::check(result, "OBJ loads successfully from string");
    test::check(mesh.vertexCount() == 3, "OBJ loaded 3 vertices");
    test::check(mesh.faceCount() == 1, "OBJ loaded 1 face");
}

void testLoadQuadFace() {
    std::cout << "\n--- OBJ Quad Triangulation Tests ---" << std::endl;

    std::string objData =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 1.0 1.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "f 1 2 3 4\n";

    HalfEdgeMesh mesh;
    bool result = ObjLoader::loadFromString(objData, mesh);

    test::check(result, "OBJ quad loads successfully");
    test::check(mesh.vertexCount() == 4, "Quad has 4 vertices");
    test::check(mesh.faceCount() == 2, "Quad triangulated into 2 faces");
}

void testLoadWithSlashes() {
    std::cout << "\n--- OBJ Vertex/Normal/UV Format Tests ---" << std::endl;

    std::string objData =
        "v 0.0 0.0 0.0\n"
        "v 1.0 0.0 0.0\n"
        "v 0.0 1.0 0.0\n"
        "vn 0.0 0.0 1.0\n"
        "f 1//1 2//1 3//1\n";

    HalfEdgeMesh mesh;
    bool result = ObjLoader::loadFromString(objData, mesh);

    test::check(result, "OBJ with v//vn format loads");
    test::check(mesh.vertexCount() == 3, "Loaded 3 vertices from v//vn format");
}

void testLoadEmpty() {
    std::cout << "\n--- OBJ Empty/Invalid Tests ---" << std::endl;

    std::string emptyData = "";
    HalfEdgeMesh mesh;
    bool result = ObjLoader::loadFromString(emptyData, mesh);
    test::check(!result, "Empty OBJ returns false");

    std::string commentOnly = "# Just a comment\n";
    result = ObjLoader::loadFromString(commentOnly, mesh);
    test::check(!result, "Comment-only OBJ returns false");

    std::string noFaces = "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
    result = ObjLoader::loadFromString(noFaces, mesh);
    test::check(!result, "OBJ without faces returns false");
}

void testSaveAndReload() {
    std::cout << "\n--- OBJ Save/Reload Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(1.0f);

    std::string tmpPath = "/tmp/test_sculpt_cube.obj";
    bool saved = ObjLoader::save(tmpPath, cube);
    test::check(saved, "OBJ save succeeds");

    HalfEdgeMesh loaded;
    bool loadResult = ObjLoader::load(tmpPath, loaded);
    test::check(loadResult, "OBJ reload succeeds");
    test::check(loaded.vertexCount() == cube.vertexCount(), "Reloaded vertex count matches");
    test::check(loaded.faceCount() == cube.faceCount(), "Reloaded face count matches");
}

int main() {
    std::cout << "=== OBJ Loader Tests ===" << std::endl;

    testLoadFromString();
    testLoadQuadFace();
    testLoadWithSlashes();
    testLoadEmpty();
    testSaveAndReload();

    return test::report("OBJ Loader");
}
