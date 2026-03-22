#include "tools/LowpolyTools.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <unordered_map>
#include <queue>

namespace sculpt {

void LowpolyTools::snapToGrid(HalfEdgeMesh& mesh, float gridSize) {
    if (gridSize <= 0) return;
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        Vec3& p = mesh.vertex(static_cast<int32_t>(i)).position;
        p.x = std::round(p.x / gridSize) * gridSize;
        p.y = std::round(p.y / gridSize) * gridSize;
        p.z = std::round(p.z / gridSize) * gridSize;
    }
    mesh.recomputeNormals();
}

void LowpolyTools::decimate(HalfEdgeMesh& mesh, int targetFaces) {
    if (targetFaces <= 0) return;

    // Count live faces
    auto countLive = [&]() -> int {
        int count = 0;
        for (size_t fi = 0; fi < mesh.faceCount(); fi++)
            if (!mesh.isFaceDeleted(static_cast<int32_t>(fi))) count++;
        return count;
    };

    if (countLive() <= targetFaces) return;

    int maxIterations = static_cast<int>(mesh.faceCount());
    int iter = 0;

    // Simple edge-length based decimation
    while (countLive() > targetFaces && iter++ < maxIterations) {
        float bestLen = 1e30f;
        int32_t bestHE = -1;

        // Find shortest edge
        for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
            if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
            int32_t he0 = mesh.face(static_cast<int32_t>(fi)).halfEdge;
            for (int j = 0; j < 3; j++) {
                int32_t he = he0 + j;
                if (he >= static_cast<int32_t>(mesh.halfEdgeCount())) continue;
                auto [v0, v1] = mesh.edgeVertices(he);
                float len = (mesh.vertex(v0).position - mesh.vertex(v1).position).lengthSq();
                if (len < bestLen) {
                    bestLen = len;
                    bestHE = he;
                }
            }
        }

        if (bestHE < 0) break;

        // Collapse edge: merge v1 into v0
        auto [v0, v1] = mesh.edgeVertices(bestHE);
        Vec3 midPos = (mesh.vertex(v0).position + mesh.vertex(v1).position) * 0.5f;
        Vec3 midCol = (mesh.vertex(v0).color + mesh.vertex(v1).color) * 0.5f;
        mesh.vertex(v0).position = midPos;
        mesh.vertex(v0).color = midCol;

        // Find all faces using v1 and remove degenerate ones / remap
        std::vector<int32_t> facesToRemove;
        for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
            if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
            auto fv = mesh.faceVertices(static_cast<int32_t>(fi));
            bool hasV0 = (fv[0] == v0 || fv[1] == v0 || fv[2] == v0);
            bool hasV1 = (fv[0] == v1 || fv[1] == v1 || fv[2] == v1);
            if (hasV0 && hasV1) {
                // Degenerate face (both endpoints) - remove
                facesToRemove.push_back(static_cast<int32_t>(fi));
            }
        }

        for (int32_t fi : facesToRemove) {
            mesh.removeFace(fi);
        }

        // Remap all references from v1 to v0 in remaining half-edges
        for (size_t i = 0; i < mesh.halfEdgeCount(); i++) {
            if (mesh.halfEdge(static_cast<int32_t>(i)).face < 0) continue;
            if (mesh.halfEdge(static_cast<int32_t>(i)).vertex == v1) {
                mesh.halfEdgeMut(static_cast<int32_t>(i)).vertex = v0;
            }
        }

        // Also check if more faces became degenerate after remapping
        for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
            if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
            auto fv = mesh.faceVertices(static_cast<int32_t>(fi));
            if (fv[0] == fv[1] || fv[1] == fv[2] || fv[0] == fv[2]) {
                mesh.removeFace(static_cast<int32_t>(fi));
            }
        }
    }

    mesh.compact();
    mesh.recomputeNormals();
}

void LowpolyTools::mirror(HalfEdgeMesh& mesh, int axis) {
    // Collect all current geometry
    std::vector<Vec3> positions;
    std::vector<Vec3> colors;
    std::vector<std::array<uint32_t, 3>> triangles;

    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        positions.push_back(mesh.vertex(static_cast<int32_t>(i)).position);
        colors.push_back(mesh.vertex(static_cast<int32_t>(i)).color);
    }
    for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
        if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
        auto fv = mesh.faceVertices(static_cast<int32_t>(fi));
        if (fv[0] < 0 || fv[1] < 0 || fv[2] < 0) continue;
        triangles.push_back({static_cast<uint32_t>(fv[0]), static_cast<uint32_t>(fv[1]), static_cast<uint32_t>(fv[2])});
    }

    // Remove vertices on the negative side of the axis
    // (keep positive side, mirror it)
    // Instead, just duplicate and mirror all geometry for simplicity
    uint32_t vOffset = static_cast<uint32_t>(positions.size());
    for (size_t i = 0; i < vOffset; i++) {
        Vec3 p = positions[i];
        if (axis == 0) p.x = -p.x;
        else if (axis == 1) p.y = -p.y;
        else p.z = -p.z;
        positions.push_back(p);
        colors.push_back(colors[i]);
    }

    // Mirror triangles (flip winding)
    size_t origTriCount = triangles.size();
    for (size_t i = 0; i < origTriCount; i++) {
        auto& t = triangles[i];
        triangles.push_back({t[0] + vOffset, t[2] + vOffset, t[1] + vOffset});
    }

    // Rebuild mesh
    mesh.buildFromTriangles(positions, triangles);

    // Restore colors
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        if (i < colors.size()) mesh.vertex(static_cast<int32_t>(i)).color = colors[i];
    }
}

HalfEdgeMesh LowpolyTools::makeFlatShaded(const HalfEdgeMesh& mesh) {
    std::vector<Vec3> positions;
    std::vector<Vec3> colors;
    std::vector<std::array<uint32_t, 3>> triangles;

    // Each face gets its own vertices (no sharing)
    for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
        if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
        auto fv = mesh.faceVertices(static_cast<int32_t>(fi));
        uint32_t base = static_cast<uint32_t>(positions.size());
        for (int i = 0; i < 3; i++) {
            positions.push_back(mesh.vertex(fv[i]).position);
            colors.push_back(mesh.vertex(fv[i]).color);
        }
        triangles.push_back({base, base + 1, base + 2});
    }

    HalfEdgeMesh result;
    result.buildFromTriangles(positions, triangles);
    for (size_t i = 0; i < result.vertexCount() && i < colors.size(); i++) {
        result.vertex(static_cast<int32_t>(i)).color = colors[i];
    }
    return result;
}

} // namespace sculpt
