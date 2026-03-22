#pragma once

#include "core/HalfEdgeMesh.h"
#include "anim/Armature.h"

namespace sculpt {

class AutoRig {
public:
    // Place a humanoid skeleton based on mesh bounding box
    static void rigHumanoid(const HalfEdgeMesh& mesh, Armature& armature);

    // Compute bone weights for all vertices (distance-based)
    static void computeWeights(const HalfEdgeMesh& mesh, Armature& armature);

    // Distance from point to line segment
    static float pointToSegmentDist(const Vec3& p, const Vec3& a, const Vec3& b);
};

} // namespace sculpt
