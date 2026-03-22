#pragma once

#include "core/HalfEdgeMesh.h"
#include <vector>
#include <string>
#include <array>

namespace sculpt {

struct BoneWeight {
    int32_t indices[4] = {0, 0, 0, 0};
    float weights[4] = {0, 0, 0, 0};
};

struct Bone {
    std::string name;
    Vec3 head;    // joint start (rest/bind pose, world space)
    Vec3 tail;    // joint end
    int32_t parent = -1;
    std::vector<int32_t> children;

    // Bind pose matrices (column-major 4x4)
    float bindWorld[16] = {};        // world transform in rest pose
    float inverseBindWorld[16] = {}; // inverse of above (for skinning)

    // Current pose
    float poseRotation[3] = {0, 0, 0}; // euler XYZ in radians, relative to rest
    float poseWorld[16] = {};           // computed world matrix for current pose
};

// 4x4 matrix utilities (column-major)
namespace Mat4 {
    void identity(float* m);
    void multiply(float* out, const float* a, const float* b);
    void inverse(float* out, const float* m);
    void fromTranslation(float* out, float x, float y, float z);
    void fromRotationEuler(float* out, float rx, float ry, float rz);
    void fromLookDir(float* out, const Vec3& pos, const Vec3& dir, const Vec3& up);
    Vec3 transformPoint(const float* m, const Vec3& p);
    Vec3 transformDir(const float* m, const Vec3& d);
}

class Armature {
public:
    Armature() = default;

    int32_t addBone(const std::string& name, const Vec3& head, const Vec3& tail, int32_t parent = -1);
    void clear();

    int32_t boneCount() const { return static_cast<int32_t>(m_bones.size()); }
    Bone& bone(int32_t idx) { return m_bones[idx]; }
    const Bone& bone(int32_t idx) const { return m_bones[idx]; }
    const std::vector<Bone>& bones() const { return m_bones; }

    int32_t findBone(const std::string& name) const;

    // Compute bind pose matrices from head/tail positions
    void computeBindMatrices();

    // Update pose matrices from current euler rotations (call every frame in pose mode)
    void updatePoseMatrices();

    // Reset all pose rotations to zero
    void resetPose();

    // Vertex weights
    std::vector<BoneWeight>& weights() { return m_weights; }
    const std::vector<BoneWeight>& weights() const { return m_weights; }
    void resizeWeights(size_t vertexCount) { m_weights.resize(vertexCount); }

    // CPU skinning: deform mesh positions/normals based on current pose
    void applyPose(HalfEdgeMesh& mesh,
                   const std::vector<Vec3>& restPositions,
                   const std::vector<Vec3>& restNormals) const;

    bool hasData() const { return !m_bones.empty(); }

private:
    std::vector<Bone> m_bones;
    std::vector<BoneWeight> m_weights;
};

} // namespace sculpt
