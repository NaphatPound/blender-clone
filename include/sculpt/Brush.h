#pragma once

#include "core/HalfEdgeMesh.h"
#include <vector>

namespace sculpt {

enum class BrushType {
    Draw,
    Smooth,
    Grab,
    Clay,
    Flatten,
    Crease,
    Pinch,
    Inflate,
    Scrape,
    Mask
};

enum class FalloffType {
    Smooth,
    Linear,
    Constant
};

struct BrushSettings {
    BrushType type = BrushType::Draw;
    FalloffType falloff = FalloffType::Smooth;
    float radius = 1.0f;
    float strength = 0.5f;
    bool invert = false;
    bool symmetryX = false;
    bool symmetryY = false;
    bool symmetryZ = false;
};

class Brush {
public:
    Brush() = default;
    explicit Brush(const BrushSettings& settings) : m_settings(settings) {}

    float calculateFalloff(float distance) const;

    // Existing brushes
    void applyDraw(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                   const Vec3& center, const Vec3& direction) const;
    void applySmooth(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                     const Vec3& center) const;
    void applyGrab(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                   const Vec3& center, const Vec3& delta) const;

    // New pro brushes
    void applyClay(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                   const Vec3& center, const Vec3& normal) const;
    void applyFlatten(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                      const Vec3& center) const;
    void applyCrease(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                     const Vec3& center) const;
    void applyPinch(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                    const Vec3& center) const;
    void applyInflate(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                      const Vec3& center) const;
    void applyScrape(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                     const Vec3& center, const Vec3& normal) const;
    void applyMask(HalfEdgeMesh& mesh, const std::vector<int32_t>& verts,
                   const Vec3& center) const;

    const BrushSettings& settings() const { return m_settings; }
    BrushSettings& settings() { return m_settings; }

private:
    BrushSettings m_settings;

    // Helper: get mask-adjusted weight
    float maskedWeight(const HalfEdgeMesh& mesh, int32_t vi, float weight) const {
        return weight * (1.0f - mesh.vertex(vi).mask);
    }
};

} // namespace sculpt
