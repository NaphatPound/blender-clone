#include "edit/EditTools.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <cmath>

namespace sculpt {

// =====================================================
// INSET FACE
// =====================================================
bool EditTools::insetFace(HalfEdgeMesh& mesh, EditSelection& sel, float amount) {
    if (sel.selectedFaces.empty()) return false;

    // Collect faces to inset (copy because we'll modify selection)
    std::vector<int32_t> facesToInset(sel.selectedFaces.begin(), sel.selectedFaces.end());
    sel.selectedFaces.clear();

    for (int32_t fi : facesToInset) {
        if (mesh.isFaceDeleted(fi)) continue;

        auto verts = mesh.faceVertices(fi);
        int32_t v0 = verts[0], v1 = verts[1], v2 = verts[2];

        const Vec3& p0 = mesh.vertex(v0).position;
        const Vec3& p1 = mesh.vertex(v1).position;
        const Vec3& p2 = mesh.vertex(v2).position;

        // Compute centroid
        Vec3 centroid = (p0 + p1 + p2) / 3.0f;

        // Create 3 new inset vertices (lerp toward centroid)
        Vec3 np0 = p0 + (centroid - p0) * amount;
        Vec3 np1 = p1 + (centroid - p1) * amount;
        Vec3 np2 = p2 + (centroid - p2) * amount;

        int32_t nv0 = mesh.addVertex(np0);
        int32_t nv1 = mesh.addVertex(np1);
        int32_t nv2 = mesh.addVertex(np2);

        // Remove original face
        mesh.removeFace(fi);

        // Add inner face
        int32_t innerFace = mesh.addFace(nv0, nv1, nv2);
        sel.selectedFaces.insert(innerFace);

        // Add 3 side quads (each as 2 triangles)
        // Side 0: v0-v1 -> nv1-nv0
        mesh.addFace(v0, v1, nv1);
        mesh.addFace(v0, nv1, nv0);

        // Side 1: v1-v2 -> nv2-nv1
        mesh.addFace(v1, v2, nv2);
        mesh.addFace(v1, nv2, nv1);

        // Side 2: v2-v0 -> nv0-nv2
        mesh.addFace(v2, v0, nv0);
        mesh.addFace(v2, nv0, nv2);
    }

    mesh.compact();
    mesh.recomputeNormals();
    return true;
}

// =====================================================
// EXTRUDE
// =====================================================
bool EditTools::extrude(HalfEdgeMesh& mesh, EditSelection& sel, float offset) {
    if (sel.selectedFaces.empty()) return false;

    std::vector<int32_t> facesToExtrude(sel.selectedFaces.begin(), sel.selectedFaces.end());
    std::unordered_set<int32_t> selFaceSet(sel.selectedFaces.begin(), sel.selectedFaces.end());

    // Compute average normal of selected faces
    Vec3 avgNormal = {0, 0, 0};
    for (int32_t fi : facesToExtrude) {
        avgNormal += mesh.face(fi).normal;
    }
    avgNormal = avgNormal.normalized();

    // Find all vertices used by selected faces
    std::unordered_set<int32_t> usedVerts;
    for (int32_t fi : facesToExtrude) {
        auto fv = mesh.faceVertices(fi);
        usedVerts.insert(fv[0]);
        usedVerts.insert(fv[1]);
        usedVerts.insert(fv[2]);
    }

    // Find boundary edges: edges where one face is selected and the other is not
    struct DirEdge {
        int32_t v0, v1;
        int32_t face; // the selected face this edge belongs to
    };
    std::vector<DirEdge> boundaryEdges;

    for (int32_t fi : facesToExtrude) {
        int32_t he0 = mesh.face(fi).halfEdge;
        for (int j = 0; j < 3; j++) {
            int32_t he = he0 + j;
            int32_t twin = mesh.halfEdge(he).twin;
            bool isBoundary = true;
            if (twin >= 0) {
                int32_t adjFace = mesh.halfEdge(twin).face;
                if (adjFace >= 0 && selFaceSet.count(adjFace)) {
                    isBoundary = false; // both faces selected = interior edge
                }
            }
            if (isBoundary) {
                auto [v0, v1] = mesh.edgeVertices(he);
                boundaryEdges.push_back({v0, v1, fi});
            }
        }
    }

    // Save face vertex data BEFORE any removal (faceVertices crashes on deleted faces)
    std::vector<std::array<int32_t, 3>> faceVertData;
    faceVertData.reserve(facesToExtrude.size());
    for (int32_t fi : facesToExtrude) {
        faceVertData.push_back(mesh.faceVertices(fi));
    }

    // Duplicate all used vertices
    std::unordered_map<int32_t, int32_t> vertDup; // old -> new
    for (int32_t vi : usedVerts) {
        Vec3 newPos = mesh.vertex(vi).position + avgNormal * offset;
        int32_t nvi = mesh.addVertex(newPos);
        vertDup[vi] = nvi;
    }

    // Remove original selected faces
    for (int32_t fi : facesToExtrude) {
        mesh.removeFace(fi);
    }

    // Re-add faces with duplicated vertices (the "top" of the extrusion)
    sel.selectedFaces.clear();
    for (const auto& fv : faceVertData) {
        int32_t newFi = mesh.addFace(vertDup[fv[0]], vertDup[fv[1]], vertDup[fv[2]]);
        sel.selectedFaces.insert(newFi);
    }

    // Create side walls for each boundary edge
    for (const auto& be : boundaryEdges) {
        int32_t ov0 = be.v0;
        int32_t ov1 = be.v1;
        int32_t nv0 = vertDup[ov0];
        int32_t nv1 = vertDup[ov1];

        // Two triangles forming a quad: ov0-ov1-nv1 and ov0-nv1-nv0
        mesh.addFace(ov0, ov1, nv1);
        mesh.addFace(ov0, nv1, nv0);
    }

    mesh.compact();
    mesh.recomputeNormals();
    return true;
}

// =====================================================
// LOOP CUT
// =====================================================
bool EditTools::loopCut(HalfEdgeMesh& mesh, int32_t startHE, int segments) {
    if (startHE < 0 || startHE >= static_cast<int32_t>(mesh.halfEdgeCount())) return false;
    if (mesh.halfEdge(startHE).face < 0) return false;

    // Walk the edge loop collecting edges to split
    // For triangle meshes: from he, go he.next.twin.next to cross one triangle
    // and get to the "opposite" edge of the quad-like triangle pair
    struct LoopEdge {
        int32_t he;       // half-edge to split
        int32_t faceA;    // face on one side
        int32_t faceB;    // face on the other side (from twin's side)
    };

    std::vector<LoopEdge> loopEdges;
    std::unordered_set<int32_t> visitedFaces;

    int32_t curHE = startHE;
    int maxIter = static_cast<int>(mesh.faceCount()); // safety limit

    for (int iter = 0; iter < maxIter; iter++) {
        int32_t face = mesh.halfEdge(curHE).face;
        if (face < 0) break;
        if (visitedFaces.count(face)) break; // loop closed or revisited

        visitedFaces.insert(face);

        LoopEdge le;
        le.he = curHE;
        le.faceA = face;

        int32_t tw = mesh.halfEdge(curHE).twin;
        le.faceB = (tw >= 0) ? mesh.halfEdge(tw).face : -1;
        if (le.faceB >= 0) visitedFaces.insert(le.faceB);

        loopEdges.push_back(le);

        // Walk to next edge in the loop: cross the current triangle via next,
        // then cross to the adjacent triangle via twin, then next again
        int32_t nextHE = mesh.halfEdge(curHE).next;
        int32_t crossTwin = mesh.halfEdge(nextHE).twin;
        if (crossTwin < 0) break;

        int32_t nextInLoop = mesh.halfEdge(crossTwin).next;
        curHE = nextInLoop;

        // Check if we've looped back
        if (curHE == startHE) break;
    }

    if (loopEdges.empty()) return false;

    // For each edge in the loop, split it by adding a midpoint vertex
    // Then retriangulate the affected faces
    struct SplitInfo {
        int32_t origV0, origV1;
        int32_t midVert;
        int32_t faceA, faceB;
    };
    std::vector<SplitInfo> splits;

    for (const auto& le : loopEdges) {
        auto [v0, v1] = mesh.edgeVertices(le.he);
        Vec3 mid = (mesh.vertex(v0).position + mesh.vertex(v1).position) * 0.5f;
        int32_t midV = mesh.addVertex(mid);
        splits.push_back({v0, v1, midV, le.faceA, le.faceB});
    }

    // Remove old faces and create new split faces
    std::unordered_set<int32_t> removedFaces;
    for (const auto& sp : splits) {
        // Remove faceA if not already removed
        if (sp.faceA >= 0 && !removedFaces.count(sp.faceA)) {
            auto fv = mesh.faceVertices(sp.faceA);
            mesh.removeFace(sp.faceA);
            removedFaces.insert(sp.faceA);

            // Split faceA: replace the edge (v0,v1) with two edges via midpoint
            // faceA has vertices including v0 and v1; the third vertex is "other"
            int32_t other = -1;
            for (int i = 0; i < 3; i++) {
                if (fv[i] != sp.origV0 && fv[i] != sp.origV1) { other = fv[i]; break; }
            }
            if (other >= 0) {
                mesh.addFace(sp.origV0, sp.midVert, other);
                mesh.addFace(sp.midVert, sp.origV1, other);
            }
        }

        // Remove faceB if not already removed
        if (sp.faceB >= 0 && !removedFaces.count(sp.faceB)) {
            auto fv = mesh.faceVertices(sp.faceB);
            mesh.removeFace(sp.faceB);
            removedFaces.insert(sp.faceB);

            int32_t other = -1;
            for (int i = 0; i < 3; i++) {
                if (fv[i] != sp.origV0 && fv[i] != sp.origV1) { other = fv[i]; break; }
            }
            if (other >= 0) {
                mesh.addFace(sp.origV1, sp.midVert, other);
                mesh.addFace(sp.midVert, sp.origV0, other);
            }
        }
    }

    mesh.compact();
    mesh.recomputeNormals();
    return true;
}

// =====================================================
// BEVEL
// =====================================================
bool EditTools::bevel(HalfEdgeMesh& mesh, EditSelection& sel, float amount) {
    if (sel.selectedEdges.empty()) return false;

    std::vector<EdgeId> edgesToBevel(sel.selectedEdges.begin(), sel.selectedEdges.end());
    sel.selectedEdges.clear();

    for (const auto& eid : edgesToBevel) {
        int32_t heA = mesh.findHalfEdge(eid.v0, eid.v1);
        if (heA < 0) heA = mesh.findHalfEdge(eid.v1, eid.v0);
        if (heA < 0) continue;

        int32_t heB = mesh.halfEdge(heA).twin;

        auto [v0, v1] = mesh.edgeVertices(heA);
        const Vec3& p0 = mesh.vertex(v0).position;
        const Vec3& p1 = mesh.vertex(v1).position;

        Vec3 edgeDir = (p1 - p0).normalized();

        // Create 4 new vertices: 2 offset from v0, 2 offset from v1
        // Offset perpendicular to the edge within the face planes
        Vec3 mid = (p0 + p1) * 0.5f;

        // For face A
        int32_t faceA = mesh.halfEdge(heA).face;
        Vec3 nA = (faceA >= 0) ? mesh.face(faceA).normal : Vec3(0, 1, 0);
        Vec3 perpA = nA.cross(edgeDir).normalized() * amount;

        // New verts on face A side
        int32_t nv0a = mesh.addVertex(p0 + perpA);
        int32_t nv1a = mesh.addVertex(p1 + perpA);

        // For face B
        Vec3 perpB = perpA * -1.0f;
        if (heB >= 0) {
            int32_t faceB = mesh.halfEdge(heB).face;
            if (faceB >= 0) {
                Vec3 nB = mesh.face(faceB).normal;
                perpB = nB.cross(edgeDir).normalized() * amount;
            }
        }
        int32_t nv0b = mesh.addVertex(p0 + perpB);
        int32_t nv1b = mesh.addVertex(p1 + perpB);

        // Remove faces adjacent to the edge
        if (faceA >= 0) {
            auto fv = mesh.faceVertices(faceA);
            int32_t other = -1;
            for (int i = 0; i < 3; i++) {
                if (fv[i] != v0 && fv[i] != v1) { other = fv[i]; break; }
            }
            mesh.removeFace(faceA);
            if (other >= 0) {
                mesh.addFace(nv0a, nv1a, other);
            }
        }

        if (heB >= 0) {
            int32_t faceB = mesh.halfEdge(heB).face;
            if (faceB >= 0) {
                auto fv = mesh.faceVertices(faceB);
                int32_t other = -1;
                for (int i = 0; i < 3; i++) {
                    if (fv[i] != v0 && fv[i] != v1) { other = fv[i]; break; }
                }
                mesh.removeFace(faceB);
                if (other >= 0) {
                    mesh.addFace(nv1b, nv0b, other);
                }
            }
        }

        // Create bevel face (quad as 2 triangles)
        mesh.addFace(nv0a, nv1a, nv1b);
        mesh.addFace(nv0a, nv1b, nv0b);

        sel.selectedEdges.insert(EdgeId(nv0a, nv1a));
        sel.selectedEdges.insert(EdgeId(nv0b, nv1b));
    }

    mesh.compact();
    mesh.recomputeNormals();
    return true;
}

} // namespace sculpt
