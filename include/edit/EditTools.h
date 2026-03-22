#pragma once

#include "core/HalfEdgeMesh.h"
#include "edit/EditSelection.h"

namespace sculpt {

class EditTools {
public:
    // Extrude selected faces along their normals by 'offset' distance
    static bool extrude(HalfEdgeMesh& mesh, EditSelection& sel, float offset);

    // Loop cut: insert an edge loop through the edge at halfEdgeIdx
    static bool loopCut(HalfEdgeMesh& mesh, int32_t halfEdgeIdx, int segments = 1);

    // Bevel selected edges by 'amount'
    static bool bevel(HalfEdgeMesh& mesh, EditSelection& sel, float amount);

    // Inset selected faces by 'amount' (0..1)
    static bool insetFace(HalfEdgeMesh& mesh, EditSelection& sel, float amount);
};

} // namespace sculpt
