#pragma once

#include "core/HalfEdgeMesh.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include <cstdint>

namespace sculpt {

enum class ShadingMode {
    MatCap,
    FlatColor,
    VertexColor,
    GamePreview
};

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    bool init();
    void shutdown();

    void uploadMesh(const HalfEdgeMesh& mesh);
    void updateMeshVertices(const HalfEdgeMesh& mesh); // fast: only update pos+normal, no realloc
    void render(const Camera& camera, float aspectRatio);

    void setWireframe(bool enabled) { m_wireframe = enabled; }
    bool isWireframe() const { return m_wireframe; }

    void setFlatShading(bool enabled) { m_flatShading = enabled; }
    bool isFlatShading() const { return m_flatShading; }

    void setShadingMode(ShadingMode mode) { m_shadingMode = mode; }
    ShadingMode getShadingMode() const { return m_shadingMode; }

private:
    Shader m_matcapShader;
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    uint32_t m_indexCount = 0;
    uint32_t m_vertexCount = 0;
    bool m_wireframe = false;
    bool m_flatShading = false;
    ShadingMode m_shadingMode = ShadingMode::MatCap;
    bool m_initialized = false;
};

} // namespace sculpt
