#include "test_framework.h"
#include "core/MeshGenerator.h"
#include "edit/EditTools.h"
#include "edit/EditSelection.h"
#include "edit/EditEngine.h"
#include "core/BVH.h"

using namespace sculpt;

void testInsetFace() {
    std::cout << "\n--- Inset Face Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    size_t origVerts = cube.vertexCount();
    size_t origFaces = cube.faceCount();

    EditSelection sel;
    sel.mode = SelectionMode::Face;
    sel.selectedFaces.insert(0); // select first face

    bool result = EditTools::insetFace(cube, sel, 0.3f);
    test::check(result, "Inset face succeeds");

    // Inset one triangle: removes 1 face, adds 1 inner + 6 side = 7 new
    // Net after compact: origFaces - 1 + 7 = origFaces + 6
    test::check(cube.faceCount() == origFaces + 6, "Inset adds 6 net faces (1 inner + 6 side - 1 removed)");
    test::check(cube.vertexCount() >= origVerts + 3, "Inset adds at least 3 new vertices");
}

void testExtrude() {
    std::cout << "\n--- Extrude Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    size_t origVerts = cube.vertexCount();
    size_t origFaces = cube.faceCount();

    EditSelection sel;
    sel.mode = SelectionMode::Face;
    sel.selectedFaces.insert(0); // select one face

    bool result = EditTools::extrude(cube, sel, 0.3f);
    test::check(result, "Extrude succeeds");
    test::check(cube.vertexCount() > origVerts, "Extrude adds vertices");
    test::check(cube.faceCount() > origFaces, "Extrude adds faces");

    // Extruded face should be in selection
    test::check(!sel.selectedFaces.empty(), "Extruded face is selected");
}

void testLoopCut() {
    std::cout << "\n--- Loop Cut Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    size_t origVerts = cube.vertexCount();
    size_t origFaces = cube.faceCount();

    // Find a valid half-edge
    int32_t he = cube.face(0).halfEdge;
    test::check(he >= 0, "Found a valid half-edge for loop cut");

    bool result = EditTools::loopCut(cube, he, 1);
    test::check(result, "Loop cut succeeds");
    test::check(cube.vertexCount() > origVerts, "Loop cut adds vertices");
    test::check(cube.faceCount() > origFaces, "Loop cut adds faces");
}

void testBevel() {
    std::cout << "\n--- Bevel Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    size_t origVerts = cube.vertexCount();

    // Select an edge
    auto fv = cube.faceVertices(0);
    EdgeId edge(fv[0], fv[1]);

    EditSelection sel;
    sel.mode = SelectionMode::Edge;
    sel.selectedEdges.insert(edge);

    bool result = EditTools::bevel(cube, sel, 0.1f);
    test::check(result, "Bevel succeeds");
    test::check(cube.vertexCount() > origVerts, "Bevel adds vertices");
}

void testMeshMutations() {
    std::cout << "\n--- Mesh Mutation Tests ---" << std::endl;

    HalfEdgeMesh mesh;
    std::vector<Vec3> pos = {{0,0,0}, {1,0,0}, {0,1,0}};
    std::vector<std::array<uint32_t, 3>> tris = {{0, 1, 2}};
    mesh.buildFromTriangles(pos, tris);

    test::check(mesh.vertexCount() == 3, "Initial mesh 3 verts");
    test::check(mesh.faceCount() == 1, "Initial mesh 1 face");

    // Add vertex
    int32_t nv = mesh.addVertex(Vec3(0.5f, 0.5f, 0));
    test::check(nv == 3, "addVertex returns correct index");
    test::check(mesh.vertexCount() == 4, "addVertex increases count");

    // Add face
    int32_t nf = mesh.addFace(0, 1, 3);
    test::check(nf >= 0, "addFace returns valid index");
    test::check(mesh.faceCount() == 2, "addFace increases count");

    // Face vertices
    auto fv = mesh.faceVertices(0);
    test::check(fv[0] >= 0 && fv[1] >= 0 && fv[2] >= 0, "faceVertices returns valid indices");

    // Remove face
    mesh.removeFace(0);
    test::check(mesh.isFaceDeleted(0), "removeFace marks face as deleted");

    // Compact
    size_t beforeCompact = mesh.faceCount();
    mesh.compact();
    test::check(mesh.faceCount() < beforeCompact, "compact reduces face count after deletion");
}

void testEditSelection() {
    std::cout << "\n--- Edit Selection Tests ---" << std::endl;

    EditSelection sel;
    sel.mode = SelectionMode::Vertex;

    sel.toggleVertex(5);
    test::check(sel.selectedVerts.count(5) == 1, "Toggle adds vertex");
    sel.toggleVertex(5);
    test::check(sel.selectedVerts.count(5) == 0, "Toggle removes vertex");

    sel.setVertex(10);
    test::check(sel.selectedVerts.size() == 1, "Set replaces selection");
    test::check(sel.selectedVerts.count(10) == 1, "Set selects correct vertex");

    sel.mode = SelectionMode::Edge;
    sel.setEdge(EdgeId(1, 5));
    test::check(sel.selectedEdges.size() == 1, "Edge selection works");
    // EdgeId should normalize order
    test::check(sel.selectedEdges.begin()->v0 == 1, "EdgeId normalizes v0 < v1");

    sel.clear();
    test::check(!sel.hasSelection(), "Clear empties selection");
}

void testPickVertex() {
    std::cout << "\n--- Pick Vertex Tests ---" << std::endl;

    HalfEdgeMesh cube = MeshGenerator::createCube(2.0f);
    EditSelection sel;
    sel.mode = SelectionMode::Vertex;

    // Pick vertex closest to a point near vertex 0
    Vec3 nearV0 = cube.vertex(0).position + Vec3(0.01f, 0, 0);
    int32_t picked = sel.pickVertex(cube, 0, nearV0);
    test::check(picked >= 0, "Pick vertex returns valid index");
}

int main() {
    std::cout << "=== Edit Tools Tests ===" << std::endl;

    testMeshMutations();
    testEditSelection();
    testPickVertex();
    testInsetFace();
    testExtrude();
    testLoopCut();
    testBevel();

    return test::report("Edit Tools");
}
