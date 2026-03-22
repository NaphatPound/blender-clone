#include "render/Renderer.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <iostream>

namespace sculpt {

static const char* MATCAP_VERT = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec3 vViewPos;
out vec3 vViewNormal;
out vec3 vColor;
out vec3 vWorldPos;

void main() {
    mat4 mv = uView * uModel;
    vec4 viewPos = mv * vec4(aPos, 1.0);
    vViewPos = viewPos.xyz;
    vWorldPos = (uModel * vec4(aPos, 1.0)).xyz;

    // Use mat3(mv) directly — correct for uniform-scale transforms
    vViewNormal = normalize(mat3(mv) * aNormal);
    vNormal = aNormal;
    vColor = aColor;

    gl_Position = uProjection * viewPos;
}
)";

static const char* MATCAP_FRAG = R"(
#version 330 core
in vec3 vNormal;
in vec3 vViewPos;
in vec3 vViewNormal;
in vec3 vColor;
in vec3 vWorldPos;

uniform int uWireframe;
uniform int uFlatShading;
uniform int uShadingMode; // 0=MatCap, 1=FlatColor, 2=VertexColor, 3=GamePreview

out vec4 FragColor;

void main() {
    if (uWireframe == 1) {
        FragColor = vec4(0.8, 0.8, 0.8, 1.0);
        return;
    }

    vec3 n = normalize(vViewNormal);

    // Flat shading: reconstruct face normal from screen derivatives
    if (uFlatShading == 1) {
        vec3 fdx = dFdx(vViewPos);
        vec3 fdy = dFdy(vViewPos);
        n = normalize(cross(fdx, fdy));
    }

    // === SHADING MODE: Flat Vertex Color ===
    if (uShadingMode == 1) {
        // Simple flat color with basic lighting
        float light = dot(n, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5;
        FragColor = vec4(vec3(0.7) * light + vec3(0.2), 1.0);
        return;
    }

    if (uShadingMode == 2) {
        // Vertex color with simple directional light
        float light = dot(n, vec3(0.0, 0.0, 1.0)) * 0.5 + 0.5;
        vec3 ambient = vColor * 0.3;
        vec3 diffuse = vColor * light * 0.7;
        FragColor = vec4(ambient + diffuse, 1.0);
        return;
    }

    if (uShadingMode == 3) {
        // Game preview: directional light + ambient
        vec3 worldN = normalize(vNormal);
        if (uFlatShading == 1) {
            vec3 wdx = dFdx(vWorldPos);
            vec3 wdy = dFdy(vWorldPos);
            worldN = normalize(cross(wdx, wdy));
        }
        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.7));
        float ndl = max(dot(worldN, lightDir), 0.0);
        vec3 ambient = vColor * 0.25;
        vec3 diffuse = vColor * ndl * 0.65;
        vec3 sky = vColor * max(dot(worldN, vec3(0,1,0)), 0.0) * 0.1;
        FragColor = vec4(ambient + diffuse + sky, 1.0);
        return;
    }

    // === SHADING MODE: MatCap (default) ===
    float u = n.x * 0.5 + 0.5;
    float v = n.y * 0.5 + 0.5;

    vec3 baseColor = vec3(0.76, 0.55, 0.42);
    vec3 highlight = vec3(1.0, 0.95, 0.9);
    vec3 shadow = vec3(0.25, 0.15, 0.1);

    float light = dot(n, vec3(0.0, 0.0, 1.0));
    light = light * 0.5 + 0.5;

    vec3 viewDir = normalize(-vViewPos);
    float rim = 1.0 - max(dot(viewDir, n), 0.0);
    rim = pow(rim, 3.0) * 0.4;

    vec3 lightDir = normalize(vec3(0.5, 1.0, 1.0));
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(n, halfDir), 0.0), 32.0) * 0.3;

    vec3 color = mix(shadow, baseColor, light);
    color = mix(color, highlight, pow(light, 3.0) * 0.5);
    color += vec3(rim * 0.3, rim * 0.2, rim * 0.15);
    color += vec3(spec);

    // Tint matcap by vertex color
    color *= vColor;

    FragColor = vec4(color, 1.0);
}
)";

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::init() {
    if (!m_matcapShader.compile(MATCAP_VERT, MATCAP_FRAG)) {
        std::cerr << "Failed to compile matcap shader" << std::endl;
        return false;
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_initialized = true;
    return true;
}

void Renderer::shutdown() {
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    m_matcapShader.destroy();
    m_initialized = false;
}

void Renderer::uploadMesh(const HalfEdgeMesh& mesh) {
    std::vector<float> vertexData;
    std::vector<uint32_t> indices;
    mesh.getTriangleData(vertexData, indices);
    m_indexCount = static_cast<uint32_t>(indices.size());

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<long>(vertexData.size() * sizeof(float)),
                 vertexData.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<long>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_DYNAMIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Color (location 2)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float),
                          (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void Renderer::render(const Camera& camera, float aspectRatio) {
    glClearColor(0.18f, 0.20f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_indexCount == 0) return;

    m_matcapShader.use();

    auto view = camera.getViewMatrix();
    auto proj = camera.getProjectionMatrix(aspectRatio);

    float model[16] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

    m_matcapShader.setMat4("uModel", model);
    m_matcapShader.setMat4("uView", view.data());
    m_matcapShader.setMat4("uProjection", proj.data());
    m_matcapShader.setInt("uWireframe", m_wireframe ? 1 : 0);
    m_matcapShader.setInt("uFlatShading", m_flatShading ? 1 : 0);
    m_matcapShader.setInt("uShadingMode", static_cast<int>(m_shadingMode));

    glBindVertexArray(m_vao);

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glDrawElements(GL_TRIANGLES, static_cast<int>(m_indexCount), GL_UNSIGNED_INT, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(0);
}

} // namespace sculpt
