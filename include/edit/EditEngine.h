#pragma once

#include "core/HalfEdgeMesh.h"
#include "core/BVH.h"
#include "edit/EditSelection.h"
#include "edit/EditTools.h"

namespace sculpt {

class EditEngine {
public:
    EditEngine() = default;

    void setMesh(HalfEdgeMesh* mesh);
    void setBVH(BVH* bvh);

    EditSelection& selection() { return m_selection; }
    const EditSelection& selection() const { return m_selection; }

    // Pick element from a ray, returns true if hit
    bool pick(const Ray& ray, bool addToSelection);

    // Tools
    bool extrude(float offset = 0.2f);
    bool loopCut(int segments = 1);
    bool bevel(float amount = 0.1f);
    bool insetFace(float amount = 0.3f);

    // Select all / deselect
    void selectAll();
    void deselectAll();

    bool isDirty() const { return m_dirty; }
    void clearDirty() { m_dirty = false; }

    // The last hovered edge (for loop cut preview)
    int32_t hoveredEdgeHE() const { return m_hoveredEdgeHE; }
    void updateHover(const Ray& ray);

private:
    HalfEdgeMesh* m_mesh = nullptr;
    BVH* m_bvh = nullptr;
    EditSelection m_selection;
    bool m_dirty = false;
    int32_t m_hoveredEdgeHE = -1;
};

} // namespace sculpt
