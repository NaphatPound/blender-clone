#include "anim/Armature.h"
#include <cmath>
#include <cstring>
#include <algorithm>

namespace sculpt {

// =====================================================
// 4x4 Matrix Utilities (column-major)
// =====================================================
namespace Mat4 {

void identity(float* m) {
    std::memset(m, 0, 16 * sizeof(float));
    m[0] = m[5] = m[10] = m[15] = 1.0f;
}

void multiply(float* out, const float* a, const float* b) {
    float tmp[16];
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            float sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += a[k * 4 + r] * b[c * 4 + k];
            }
            tmp[c * 4 + r] = sum;
        }
    }
    std::memcpy(out, tmp, 16 * sizeof(float));
}

void inverse(float* out, const float* m) {
    // For an affine transform (rotation + translation), transpose rotation and negate translation
    // This is faster and more stable than full 4x4 inverse
    float r00 = m[0], r01 = m[4], r02 = m[8];
    float r10 = m[1], r11 = m[5], r12 = m[9];
    float r20 = m[2], r21 = m[6], r22 = m[10];
    float tx = m[12], ty = m[13], tz = m[14];

    identity(out);
    out[0] = r00; out[4] = r10; out[8]  = r20;
    out[1] = r01; out[5] = r11; out[9]  = r21;
    out[2] = r02; out[6] = r12; out[10] = r22;
    out[12] = -(r00*tx + r01*ty + r02*tz);
    out[13] = -(r10*tx + r11*ty + r12*tz);
    out[14] = -(r20*tx + r21*ty + r22*tz);
}

void fromTranslation(float* out, float x, float y, float z) {
    identity(out);
    out[12] = x; out[13] = y; out[14] = z;
}

void fromRotationEuler(float* out, float rx, float ry, float rz) {
    float cx = std::cos(rx), sx = std::sin(rx);
    float cy = std::cos(ry), sy = std::sin(ry);
    float cz = std::cos(rz), sz = std::sin(rz);

    identity(out);
    // Rotation order: Z * Y * X (standard)
    out[0]  = cy * cz;
    out[1]  = cy * sz;
    out[2]  = -sy;
    out[4]  = sx * sy * cz - cx * sz;
    out[5]  = sx * sy * sz + cx * cz;
    out[6]  = sx * cy;
    out[8]  = cx * sy * cz + sx * sz;
    out[9]  = cx * sy * sz - sx * cz;
    out[10] = cx * cy;
}

void fromLookDir(float* out, const Vec3& pos, const Vec3& dir, const Vec3& worldUp) {
    Vec3 fwd = dir.normalized();
    Vec3 right = fwd.cross(worldUp).normalized();
    if (right.lengthSq() < 1e-8f) {
        // dir is parallel to up, pick arbitrary right
        right = fwd.cross(Vec3(1, 0, 0)).normalized();
        if (right.lengthSq() < 1e-8f)
            right = fwd.cross(Vec3(0, 0, 1)).normalized();
    }
    Vec3 up = right.cross(fwd).normalized();

    identity(out);
    out[0] = right.x; out[1] = right.y; out[2] = right.z;
    out[4] = up.x;    out[5] = up.y;    out[6] = up.z;
    out[8] = fwd.x;   out[9] = fwd.y;   out[10] = fwd.z;
    out[12] = pos.x;  out[13] = pos.y;  out[14] = pos.z;
}

Vec3 transformPoint(const float* m, const Vec3& p) {
    return {
        m[0]*p.x + m[4]*p.y + m[8]*p.z + m[12],
        m[1]*p.x + m[5]*p.y + m[9]*p.z + m[13],
        m[2]*p.x + m[6]*p.y + m[10]*p.z + m[14]
    };
}

Vec3 transformDir(const float* m, const Vec3& d) {
    return {
        m[0]*d.x + m[4]*d.y + m[8]*d.z,
        m[1]*d.x + m[5]*d.y + m[9]*d.z,
        m[2]*d.x + m[6]*d.y + m[10]*d.z
    };
}

} // namespace Mat4

// =====================================================
// Armature
// =====================================================

int32_t Armature::addBone(const std::string& name, const Vec3& head, const Vec3& tail, int32_t parent) {
    int32_t idx = static_cast<int32_t>(m_bones.size());
    Bone b;
    b.name = name;
    b.head = head;
    b.tail = tail;
    b.parent = parent;
    Mat4::identity(b.bindWorld);
    Mat4::identity(b.inverseBindWorld);
    Mat4::identity(b.poseWorld);
    m_bones.push_back(b);

    if (parent >= 0 && parent < static_cast<int32_t>(m_bones.size())) {
        m_bones[parent].children.push_back(idx);
    }
    return idx;
}

void Armature::clear() {
    m_bones.clear();
    m_weights.clear();
}

int32_t Armature::findBone(const std::string& name) const {
    for (size_t i = 0; i < m_bones.size(); i++) {
        if (m_bones[i].name == name) return static_cast<int32_t>(i);
    }
    return -1;
}

void Armature::computeBindMatrices() {
    for (auto& bone : m_bones) {
        Vec3 dir = (bone.tail - bone.head).normalized();
        Vec3 up(0, 1, 0);
        if (std::abs(dir.dot(up)) > 0.99f) up = Vec3(0, 0, 1);
        Mat4::fromLookDir(bone.bindWorld, bone.head, dir, up);
        Mat4::inverse(bone.inverseBindWorld, bone.bindWorld);
    }
}

void Armature::updatePoseMatrices() {
    // Process bones in topological order (parents before children)
    // Build processing order: roots first, then BFS through children
    std::vector<int32_t> order;
    order.reserve(m_bones.size());
    for (size_t i = 0; i < m_bones.size(); i++) {
        if (m_bones[i].parent < 0) order.push_back(static_cast<int32_t>(i));
    }
    for (size_t idx = 0; idx < order.size(); idx++) {
        int32_t bi = order[idx];
        for (int32_t child : m_bones[bi].children) {
            order.push_back(child);
        }
    }

    for (int32_t i : order) {
        auto& bone = m_bones[i];

        // Local rotation from euler angles
        float localRot[16];
        Mat4::fromRotationEuler(localRot, bone.poseRotation[0], bone.poseRotation[1], bone.poseRotation[2]);

        // Posed local matrix = bindWorld * localRotation
        float posedLocal[16];
        Mat4::multiply(posedLocal, bone.bindWorld, localRot);

        if (bone.parent < 0) {
            // Root bone: posed world = posed local
            std::memcpy(bone.poseWorld, posedLocal, 16 * sizeof(float));
        } else {
            // Child: transform relative to parent
            // poseWorld = parent.poseWorld * inv(parent.bindWorld) * posedLocal
            float parentInvBind[16];
            Mat4::inverse(parentInvBind, m_bones[bone.parent].bindWorld);
            float temp[16];
            Mat4::multiply(temp, m_bones[bone.parent].poseWorld, parentInvBind);
            Mat4::multiply(bone.poseWorld, temp, posedLocal);
        }
    }
}

void Armature::resetPose() {
    for (auto& bone : m_bones) {
        bone.poseRotation[0] = bone.poseRotation[1] = bone.poseRotation[2] = 0;
    }
    updatePoseMatrices();
}

void Armature::applyPose(HalfEdgeMesh& mesh,
                          const std::vector<Vec3>& restPositions,
                          const std::vector<Vec3>& restNormals) const {
    if (m_weights.empty() || m_bones.empty()) return;

    size_t count = std::min({mesh.vertexCount(), restPositions.size(),
                             restNormals.size(), m_weights.size()});

    for (size_t i = 0; i < count; i++) {
        const BoneWeight& bw = m_weights[i];
        Vec3 pos = {0, 0, 0};
        Vec3 nrm = {0, 0, 0};

        for (int j = 0; j < 4; j++) {
            float w = bw.weights[j];
            if (w < 1e-6f) continue;

            int32_t bi = bw.indices[j];
            if (bi < 0 || bi >= static_cast<int32_t>(m_bones.size())) continue;

            const Bone& bone = m_bones[bi];

            // skinMatrix = bone.poseWorld * bone.inverseBindWorld
            float skinMat[16];
            Mat4::multiply(skinMat, bone.poseWorld, bone.inverseBindWorld);

            pos += Mat4::transformPoint(skinMat, restPositions[i]) * w;
            nrm += Mat4::transformDir(skinMat, restNormals[i]) * w;
        }

        mesh.vertex(static_cast<int32_t>(i)).position = pos;
        mesh.vertex(static_cast<int32_t>(i)).normal = nrm.normalized();
    }
}

} // namespace sculpt
