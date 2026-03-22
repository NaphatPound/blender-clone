#pragma once

#include "core/HalfEdgeMesh.h"
#include "core/BVH.h"
#include "sculpt/Brush.h"

namespace sculpt {

class SculptEngine {
public:
    SculptEngine() = default;

    void setMesh(HalfEdgeMesh* mesh);
    void rebuildBVH();

    // Perform a brush stroke at a world position
    bool stroke(const Vec3& worldPos, const Vec3& direction);

    // Perform a grab stroke
    bool grabStroke(const Vec3& worldPos, const Vec3& delta);

    // Raycast from screen position
    RayHit raycast(const Ray& ray) const;

    Brush& brush() { return m_brush; }
    const Brush& brush() const { return m_brush; }

    const BVH& bvh() const { return m_bvh; }

private:
    HalfEdgeMesh* m_mesh = nullptr;
    BVH m_bvh;
    Brush m_brush;
};

} // namespace sculpt
