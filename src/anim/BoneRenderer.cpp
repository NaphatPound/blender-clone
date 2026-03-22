#include "anim/BoneRenderer.h"

#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

namespace sculpt {

static const char* BONE_VERT = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
    gl_PointSize = 10.0;
}
)";

static const char* BONE_FRAG = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main() {
    FragColor = uColor;
}
)";

BoneRenderer::~BoneRenderer() { shutdown(); }

bool BoneRenderer::init() {
    if (!m_flatShader.compile(BONE_VERT, BONE_FRAG)) return false;

    auto setup = [](uint32_t& vao, uint32_t& vbo) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    };

    setup(m_lineVAO, m_lineVBO);
    setup(m_jointVAO, m_jointVBO);
    setup(m_selVAO, m_selVBO);

    m_initialized = true;
    return true;
}

void BoneRenderer::shutdown() {
    auto cleanup = [](uint32_t& vao, uint32_t& vbo) {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
    };
    cleanup(m_lineVAO, m_lineVBO);
    cleanup(m_jointVAO, m_jointVBO);
    cleanup(m_selVAO, m_selVBO);
    m_flatShader.destroy();
    m_initialized = false;
}

void BoneRenderer::update(const Armature& armature, int32_t selectedBone) {
    std::vector<float> lineData, jointData, selData;

    for (int32_t i = 0; i < armature.boneCount(); i++) {
        const Bone& b = armature.bone(i);
        // Use posed head/tail positions
        Vec3 head = Mat4::transformPoint(b.poseWorld, Vec3(0, 0, 0));
        Vec3 dir = (b.tail - b.head);
        Vec3 tail = Mat4::transformPoint(b.poseWorld,
            Vec3(0, 0, 0) + Vec3(0, 0, dir.length())); // tail along local Z

        // Actually simpler: just use the pose matrix to transform the bind-space positions
        // Bind-space head is at origin of the bone, tail is along the bone direction
        float boneLen = (b.tail - b.head).length();
        Vec3 localTail(0, 0, boneLen); // bone points along local Z in our fromLookDir convention
        // Wait - fromLookDir puts dir along Z (column 2), so:
        // head in bone-local space = (0,0,0), tail = (0,0,boneLen)
        // But we need to use the actual bind world matrix properly

        // Simpler approach: transform original head/tail by skinMatrix
        // skinMatrix = poseWorld * inverseBindWorld
        float skinMat[16];
        Mat4::multiply(skinMat, b.poseWorld, b.inverseBindWorld);
        Vec3 posedHead = Mat4::transformPoint(skinMat, b.head);
        Vec3 posedTail = Mat4::transformPoint(skinMat, b.tail);

        // Bone line
        lineData.push_back(posedHead.x); lineData.push_back(posedHead.y); lineData.push_back(posedHead.z);
        lineData.push_back(posedTail.x); lineData.push_back(posedTail.y); lineData.push_back(posedTail.z);

        // Joint point
        jointData.push_back(posedHead.x); jointData.push_back(posedHead.y); jointData.push_back(posedHead.z);

        // Selected bone highlight
        if (i == selectedBone) {
            selData.push_back(posedHead.x); selData.push_back(posedHead.y); selData.push_back(posedHead.z);
            selData.push_back(posedTail.x); selData.push_back(posedTail.y); selData.push_back(posedTail.z);
        }
    }

    m_lineVertCount = static_cast<int>(lineData.size() / 3);
    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(lineData.size() * sizeof(float)),
                 lineData.empty() ? nullptr : lineData.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    m_jointCount = static_cast<int>(jointData.size() / 3);
    glBindVertexArray(m_jointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_jointVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(jointData.size() * sizeof(float)),
                 jointData.empty() ? nullptr : jointData.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);

    m_selVertCount = static_cast<int>(selData.size() / 3);
    glBindVertexArray(m_selVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_selVBO);
    glBufferData(GL_ARRAY_BUFFER, static_cast<long>(selData.size() * sizeof(float)),
                 selData.empty() ? nullptr : selData.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(0);
}

void BoneRenderer::render(const Camera& camera, float aspectRatio) {
    if (!m_initialized) return;

    auto view = camera.getViewMatrix();
    auto proj = camera.getProjectionMatrix(aspectRatio);

    float mvp[16] = {};
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            float sum = 0;
            for (int k = 0; k < 4; k++)
                sum += proj[k * 4 + r] * view[c * 4 + k];
            mvp[c * 4 + r] = sum;
        }

    m_flatShader.use();
    m_flatShader.setMat4("uMVP", mvp);

    glDisable(GL_DEPTH_TEST);

    // All bones (cyan lines)
    if (m_lineVertCount > 0) {
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    0.0f, 0.85f, 0.95f, 1.0f);
        glLineWidth(3.0f);
        glBindVertexArray(m_lineVAO);
        glDrawArrays(GL_LINES, 0, m_lineVertCount);
    }

    // Joints (yellow dots)
    if (m_jointCount > 0) {
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    1.0f, 0.9f, 0.2f, 1.0f);
        glBindVertexArray(m_jointVAO);
        glDrawArrays(GL_POINTS, 0, m_jointCount);
    }

    // Selected bone (bright green, thick)
    if (m_selVertCount > 0) {
        glUniform4f(glGetUniformLocation(m_flatShader.getProgram(), "uColor"),
                    0.3f, 1.0f, 0.3f, 1.0f);
        glLineWidth(5.0f);
        glBindVertexArray(m_selVAO);
        glDrawArrays(GL_LINES, 0, m_selVertCount);
    }

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
}

} // namespace sculpt
