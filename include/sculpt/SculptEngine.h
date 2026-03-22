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
    bool grabStroke(const Vec3& worldPos, const Vec3& delta);

    // Raycast from screen position
    RayHit raycast(const Ray& ray) const;

    Brush& brush() { return m_brush; }
    const Brush& brush() const { return m_brush; }
    const BVH& bvh() const { return m_bvh; }

    // Undo/redo
    void pushUndo();
    bool undo();
    bool redo();
    int undoDepth() const { return static_cast<int>(m_undoStack.size()); }
    int redoDepth() const { return static_cast<int>(m_redoStack.size()); }

    // Mask operations
    void clearMask();
    void invertMask();

private:
    // Apply stroke at a single position (used by symmetry system)
    bool strokeAt(const Vec3& worldPos, const Vec3& direction);
    bool grabAt(const Vec3& worldPos, const Vec3& delta);

    // Partial normal update for affected vertices
    void recomputeNormalsPartial(const std::vector<int32_t>& affectedVerts);

    HalfEdgeMesh* m_mesh = nullptr;
    BVH m_bvh;
    Brush m_brush;

    // Undo system
    struct MeshSnapshot {
        std::vector<Vec3> positions;
        std::vector<Vec3> normals;
        std::vector<Vec3> colors;
        std::vector<float> masks;
    };
    std::vector<MeshSnapshot> m_undoStack;
    std::vector<MeshSnapshot> m_redoStack;
    static constexpr int MAX_UNDO = 50;

    MeshSnapshot captureSnapshot() const;
    void restoreSnapshot(const MeshSnapshot& snap);
};

} // namespace sculpt
