#include "edit/SelectionRenderer.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <vector>

namespace sculpt {

static const char* FLAT_VERT = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = 8.0;
}
)";

static const char* FLAT_FRAG = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
)";

SelectionRenderer::~SelectionRenderer() {
    shutdown();
}

bool SelectionRenderer::init() {
    if (!m_flatShader.compile(FLAT_VERT, FLAT_FRAG)) return false;

    glGenVertexArrays(1, &m_pointVAO);
    glGenBuffers(1, &m_pointVBO);

    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glGenVertexArrays(1, &m_faceVAO);
    glGenBuffers(1, &m_faceVBO);

    glGenVertexArrays(1, &m_wireVAO);
    glGenBuffers(1, &m_wireVBO);

    // Setup VAO attribute layouts
    auto setupVAO = [](uint32_t vao, uint32_t vbo) {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    };

    setupVAO(m_pointVAO, m_pointVBO);
    setupVAO(m_lineVAO, m_lineVBO);
    setupVAO(m_faceVAO, m_faceVBO);
    setupVAO(m_wireVAO, m_wireVBO);

    m_initialized = true;
    return true;
}

void SelectionRenderer::shutdown() {
    if (m_pointVAO) { glDeleteVertexArrays(1, &m_pointVAO); m_pointVAO = 0; }
    if (m_pointVBO) { glDeleteBuffers(1, &m_pointVBO); m_pointVBO = 0; }
    if (m_lineVAO) { glDeleteVertexArrays(1, &m_lineVAO); m_lineVAO = 0; }
    if (m_lineVBO) { glDeleteBuffers(1, &m_lineVBO); m_lineVBO = 0; }
    if (m_faceVAO) { glDeleteVertexArrays(1, &m_faceVAO); m_faceVAO = 0; }
    if (m_faceVBO) { glDeleteBuffers(1, &m_faceVBO); m_faceVBO = 0; }
    if (m_wireVAO) { glDeleteVertexArrays(1, &m_wireVAO); m_wireVAO = 0; }
    if (m_wireVBO) { glDeleteBuffers(1, &m_wireVBO); m_wireVBO = 0; }
    m_flatShader.destroy();
    m_initialized = false;
}

void SelectionRenderer::updateMeshWireframe(const HalfEdgeMesh& mesh) {
    std::vector<float> lineData;
    std::unordered_set<int64_t> seenEdges;

    for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
        if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
        auto verts = mesh.faceVertices(static_cast<int32_t>(fi));
        for (int i = 0; i < 3; i++) {
            int32_t va = verts[i], vb = verts[(i + 1) % 3];
            int64_t key = (static_cast<int64_t>(std::min(va, vb)) << 32) |
                           static_cast<int64_t>(std::max(va, vb));
            if (seenEdges.count(key)) continue;
            seenEdges.insert(key);

            const Vec3& pa = mesh.vertex(va).position;
            const Vec3& pb = mesh.vertex(vb).position;
            lineData.push_back(pa.x); lineData.push_back(pa.y); lineData.push_back(pa.z);
            lineData.push_back(pb.x); lineData.push_back(pb.y); lineData.push_back(pb.z);
        }
    }

    m_wireVertCount = static_cast<int>(lineData.size() / 3);
    glBindVertexArray(m_wireVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_wireVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<long>(lineData.size() * sizeof(float)),
                 lineData.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

void SelectionRenderer::updateSelection(const HalfEdgeMesh& mesh, const EditSelection& sel) {
    // -- Vertex points --
    {
        std::vector<float> data;
        for (int32_t vi : sel.selectedVerts) {
            if (vi < 0 || vi >= static_cast<int32_t>(mesh.vertexCount())) continue;
            const Vec3& p = mesh.vertex(vi).position;
            data.push_back(p.x); data.push_back(p.y); data.push_back(p.z);
        }
        m_pointCount = static_cast<int>(data.size() / 3);
        glBindVertexArray(m_pointVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_pointVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<long>(data.size() * sizeof(float)),
                     data.empty() ? nullptr : data.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(0);
    }

    // -- Edge lines --
    {
        std::vector<float> data;
        for (const auto& e : sel.selectedEdges) {
            if (e.v0 < 0 || e.v0 >= static_cast<int32_t>(mesh.vertexCount())) continue;
            if (e.v1 < 0 || e.v1 >= static_cast<int32_t>(mesh.vertexCount())) continue;
            const Vec3& pa = mesh.vertex(e.v0).position;
            const Vec3& pb = mesh.vertex(e.v1).position;
            data.push_back(pa.x); data.push_back(pa.y); data.push_back(pa.z);
            data.push_back(pb.x); data.push_back(pb.y); data.push_back(pb.z);
        }
        m_lineVertCount = static_cast<int>(data.size() / 3);
        glBindVertexArray(m_lineVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<long>(data.size() * sizeof(float)),
                     data.empty() ? nullptr : data.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(0);
    }

    // -- Face triangles (semi-transparent overlay) --
    {
        std::vector<float> data;
        for (int32_t fi : sel.selectedFaces) {
            if (fi < 0 || fi >= static_cast<int32_t>(mesh.faceCount())) continue;
            if (mesh.isFaceDeleted(fi)) continue;
            auto verts = mesh.faceVertices(fi);
            if (verts[0] < 0 || verts[1] < 0 || verts[2] < 0) continue;
            if (verts[0] >= static_cast<int32_t>(mesh.vertexCount()) ||
                verts[1] >= static_cast<int32_t>(mesh.vertexCount()) ||
                verts[2] >= static_cast<int32_t>(mesh.vertexCount())) continue;
            for (int i = 0; i < 3; i++) {
                const Vec3& p = mesh.vertex(verts[i]).position;
                data.push_back(p.x); data.push_back(p.y); data.push_back(p.z);
            }
        }
        m_faceVertCount = static_cast<int>(data.size() / 3);
        glBindVertexArray(m_faceVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_faceVBO);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<long>(data.size() * sizeof(float)),
                     data.empty() ? nullptr : data.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(0);
    }
}

void SelectionRenderer::render(const Camera& camera, float aspectRatio) {
    if (!m_initialized) return;

    auto view = camera.getViewMatrix();
    auto proj = camera.getProjectionMatrix(aspectRatio);

    // Compute MVP (model = identity)
    // Manual 4x4 multiply: result = proj * view
    float mvp[16] = {};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            float sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += proj[k * 4 + r] * view[c * 4 + k];
            }
            mvp[c * 4 + r] = sum;
        }
    }

    m_flatShader.use();
    m_flatShader.setMat4("uMVP", mvp);

    glEnable(GL_DEPTH_TEST);

    // Depth bias to render on top of the mesh surface
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glEnable(GL_POLYGON_OFFSET_POINT);

    // -- Wireframe overlay (faint) --
    if (m_wireVertCount > 0) {
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    0.35f, 0.35f, 0.4f, 1.0f);
        glBindVertexArray(m_wireVAO);
        glDrawArrays(GL_LINES, 0, m_wireVertCount);
    }

    // -- Selected face overlay (orange, semi-transparent) --
    if (m_faceVertCount > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    1.0f, 0.5f, 0.15f, 0.35f);
        glBindVertexArray(m_faceVAO);
        glDrawArrays(GL_TRIANGLES, 0, m_faceVertCount);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // -- Selected edge lines (bright orange) --
    if (m_lineVertCount > 0) {
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    1.0f, 0.6f, 0.1f, 1.0f);
        glLineWidth(3.0f);
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_LINES, 0, m_lineVertCount);
        glLineWidth(1.0f);
    }

    // -- Selected vertex dots (white/bright) --
    if (m_pointCount > 0) {
#ifndef __APPLE__
        glEnable(GL_PROGRAM_POINT_SIZE);
#endif
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    1.0f, 1.0f, 1.0f, 1.0f);
        glBindVertexArray(m_pointVAO);
        glDrawArrays(GL_POINTS, 0, m_pointCount);
    }

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDisable(GL_POLYGON_OFFSET_LINE);
    glDisable(GL_POLYGON_OFFSET_POINT);
    glBindVertexArray(0);
}

} // namespace sculpt
