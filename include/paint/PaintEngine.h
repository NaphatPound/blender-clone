#pragma once

#include "core/HalfEdgeMesh.h"
#include "core/BVH.h"
#include <vector>

namespace sculpt {

struct PaintBrushSettings {
    Vec3 color = {1, 0, 0};
    float radius = 0.3f;
    float strength = 1.0f;
    bool faceFill = false;
};

class PaintEngine {
public:
    PaintEngine();

    void setMesh(HalfEdgeMesh* mesh) { m_mesh = mesh; }
    void setBVH(BVH* bvh) { m_bvh = bvh; }

    // Paint a stroke at world position
    bool paintStroke(const Vec3& worldPos);

    // Fill a face with the current color
    bool fillFace(const Ray& ray);

    // Eyedropper: sample color from mesh
    Vec3 sampleColor(const Ray& ray) const;

    PaintBrushSettings& settings() { return m_settings; }

    // Palette
    std::vector<Vec3>& palette() { return m_palette; }

private:
    HalfEdgeMesh* m_mesh = nullptr;
    BVH* m_bvh = nullptr;
    PaintBrushSettings m_settings;
    std::vector<Vec3> m_palette;
};

} // namespace sculpt
