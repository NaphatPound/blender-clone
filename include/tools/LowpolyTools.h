#pragma once

#include "core/HalfEdgeMesh.h"

namespace sculpt {

class LowpolyTools {
public:
    // Snap all vertices to grid
    static void snapToGrid(HalfEdgeMesh& mesh, float gridSize);

    // Decimate mesh by collapsing shortest edges until reaching targetFaces
    static void decimate(HalfEdgeMesh& mesh, int targetFaces);

    // Mirror mesh across axis (0=X, 1=Y, 2=Z)
    static void mirror(HalfEdgeMesh& mesh, int axis);

    // Make normals flat (split shared vertices so each face has unique verts)
    static HalfEdgeMesh makeFlatShaded(const HalfEdgeMesh& mesh);
};

} // namespace sculpt
