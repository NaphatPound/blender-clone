#pragma once

#include "core/HalfEdgeMesh.h"
#include "edit/EditSelection.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include <cstdint>

namespace sculpt {

class SelectionRenderer {
public:
    SelectionRenderer() = default;
    ~SelectionRenderer();

    bool init();
    void shutdown();

    void updateSelection(const HalfEdgeMesh& mesh, const EditSelection& sel);
    void render(const Camera& camera, float aspectRatio);

    // Render all mesh edges faintly (edit mode wireframe overlay)
    void updateMeshWireframe(const HalfEdgeMesh& mesh);

private:
    Shader m_flatShader;

    // Vertex dots
    uint32_t m_pointVAO = 0, m_pointVBO = 0;
    int m_pointCount = 0;

    // Edge lines
    uint32_t m_lineVAO = 0, m_lineVBO = 0;
    int m_lineVertCount = 0;

    // Face overlay
    uint32_t m_faceVAO = 0, m_faceVBO = 0;
    int m_faceVertCount = 0;

    // Mesh wireframe
    uint32_t m_wireVAO = 0, m_wireVBO = 0;
    int m_wireVertCount = 0;

    bool m_initialized = false;
};

} // namespace sculpt
