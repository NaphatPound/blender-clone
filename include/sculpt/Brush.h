#pragma once

#include "core/HalfEdgeMesh.h"
#include <vector>

namespace sculpt {

enum class BrushType {
    Draw,
    Smooth,
    Grab
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
    bool invert = false;       // invert direction (e.g., push in instead of out)
    bool symmetryX = false;    // mirror across X axis
};

class Brush {
public:
    Brush() = default;
    explicit Brush(const BrushSettings& settings) : m_settings(settings) {}

    // Calculate falloff weight based on distance from center
    float calculateFalloff(float distance) const;

    // Apply brush to vertices
    void applyDraw(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                   const Vec3& center, const Vec3& direction) const;

    void applySmooth(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                     const Vec3& center) const;

    void applyGrab(HalfEdgeMesh& mesh, const std::vector<int32_t>& vertexIndices,
                   const Vec3& center, const Vec3& delta) const;

    const BrushSettings& settings() const { return m_settings; }
    BrushSettings& settings() { return m_settings; }

private:
    BrushSettings m_settings;
};

} // namespace sculpt
