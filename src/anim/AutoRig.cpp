#include "anim/AutoRig.h"
#include <cmath>
#include <algorithm>

namespace sculpt {

float AutoRig::pointToSegmentDist(const Vec3& p, const Vec3& a, const Vec3& b) {
    Vec3 ab = b - a;
    float lenSq = ab.lengthSq();
    if (lenSq < 1e-10f) return (p - a).length();
    float t = std::max(0.0f, std::min(1.0f, (p - a).dot(ab) / lenSq));
    Vec3 closest = a + ab * t;
    return (p - closest).length();
}

void AutoRig::rigHumanoid(const HalfEdgeMesh& mesh, Armature& armature) {
    armature.clear();

    AABB bounds = mesh.computeBounds();
    Vec3 size = bounds.size();
    Vec3 center = bounds.center();
    float h = size.y;        // total height
    float w = size.x;        // total width
    float bot = bounds.min.y;

    // Proportional bone placement (Y-up humanoid)
    float cx = center.x, cz = center.z;

    // Core chain
    Vec3 hipsPos    = {cx, bot + h * 0.45f, cz};
    Vec3 spinePos   = {cx, bot + h * 0.55f, cz};
    Vec3 chestPos   = {cx, bot + h * 0.65f, cz};
    Vec3 neckPos    = {cx, bot + h * 0.80f, cz};
    Vec3 headTop    = {cx, bot + h * 0.95f, cz};

    int32_t hips  = armature.addBone("Hips",  hipsPos, spinePos,  -1);
    int32_t spine = armature.addBone("Spine", spinePos, chestPos, hips);
    int32_t chest = armature.addBone("Chest", chestPos, neckPos,  spine);
    int32_t neck  = armature.addBone("Neck",  neckPos, {cx, bot + h * 0.85f, cz}, chest);
    int32_t head  = armature.addBone("Head",  {cx, bot + h * 0.85f, cz}, headTop, neck);

    // Arms
    float shoulderW = w * 0.22f;
    float armLen = w * 0.30f;
    float forearmLen = w * 0.28f;
    float shoulderY = bot + h * 0.73f;

    // Left arm
    Vec3 lShould = {cx - shoulderW, shoulderY, cz};
    Vec3 lElbow  = {cx - shoulderW - armLen, shoulderY - h * 0.02f, cz};
    Vec3 lHand   = {cx - shoulderW - armLen - forearmLen, shoulderY - h * 0.04f, cz};
    int32_t lUpperArm = armature.addBone("L_UpperArm", lShould, lElbow, chest);
    int32_t lForearm  = armature.addBone("L_Forearm",  lElbow, lHand, lUpperArm);
    armature.addBone("L_Hand", lHand, lHand + Vec3(-w * 0.08f, 0, 0), lForearm);

    // Right arm
    Vec3 rShould = {cx + shoulderW, shoulderY, cz};
    Vec3 rElbow  = {cx + shoulderW + armLen, shoulderY - h * 0.02f, cz};
    Vec3 rHand   = {cx + shoulderW + armLen + forearmLen, shoulderY - h * 0.04f, cz};
    int32_t rUpperArm = armature.addBone("R_UpperArm", rShould, rElbow, chest);
    int32_t rForearm  = armature.addBone("R_Forearm",  rElbow, rHand, rUpperArm);
    armature.addBone("R_Hand", rHand, rHand + Vec3(w * 0.08f, 0, 0), rForearm);

    // Legs
    float hipW = w * 0.10f;
    float thighLen = h * 0.22f;
    float shinLen = h * 0.20f;

    // Left leg
    Vec3 lHip   = {cx - hipW, hipsPos.y, cz};
    Vec3 lKnee  = {cx - hipW, hipsPos.y - thighLen, cz + h * 0.01f};
    Vec3 lAnkle = {cx - hipW, hipsPos.y - thighLen - shinLen, cz};
    Vec3 lToe   = {cx - hipW, bot, cz + h * 0.06f};
    int32_t lThigh = armature.addBone("L_Thigh", lHip, lKnee, hips);
    int32_t lShin  = armature.addBone("L_Shin",  lKnee, lAnkle, lThigh);
    armature.addBone("L_Foot", lAnkle, lToe, lShin);

    // Right leg
    Vec3 rHip   = {cx + hipW, hipsPos.y, cz};
    Vec3 rKnee  = {cx + hipW, hipsPos.y - thighLen, cz + h * 0.01f};
    Vec3 rAnkle = {cx + hipW, hipsPos.y - thighLen - shinLen, cz};
    Vec3 rToe   = {cx + hipW, bot, cz + h * 0.06f};
    int32_t rThigh = armature.addBone("R_Thigh", rHip, rKnee, hips);
    int32_t rShin  = armature.addBone("R_Shin",  rKnee, rAnkle, rThigh);
    armature.addBone("R_Foot", rAnkle, rToe, rShin);

    armature.computeBindMatrices();
    armature.resetPose();
}

void AutoRig::computeWeights(const HalfEdgeMesh& mesh, Armature& armature) {
    armature.resizeWeights(mesh.vertexCount());

    int bc = armature.boneCount();
    if (bc == 0) return;

    for (size_t vi = 0; vi < mesh.vertexCount(); vi++) {
        const Vec3& p = mesh.vertex(static_cast<int32_t>(vi)).position;

        // Compute distance to each bone segment
        struct BoneDist { int32_t idx; float dist; };
        std::vector<BoneDist> dists(bc);
        for (int32_t bi = 0; bi < bc; bi++) {
            dists[bi] = {bi, pointToSegmentDist(p, armature.bone(bi).head, armature.bone(bi).tail)};
        }

        // Sort by distance, take closest 4
        std::sort(dists.begin(), dists.end(), [](const BoneDist& a, const BoneDist& b) {
            return a.dist < b.dist;
        });

        BoneWeight& bw = armature.weights()[vi];
        float totalWeight = 0;
        int count = std::min(4, bc);

        for (int j = 0; j < 4; j++) {
            if (j < count && dists[j].dist < 1e10f) {
                bw.indices[j] = dists[j].idx;
                // Inverse distance weighting (with falloff)
                float d = std::max(dists[j].dist, 0.001f);
                bw.weights[j] = 1.0f / (d * d);
                totalWeight += bw.weights[j];
            } else {
                bw.indices[j] = 0;
                bw.weights[j] = 0;
            }
        }

        // Normalize weights
        if (totalWeight > 0) {
            for (int j = 0; j < 4; j++) {
                bw.weights[j] /= totalWeight;
            }
        } else {
            bw.indices[0] = 0;
            bw.weights[0] = 1.0f;
        }
    }
}

} // namespace sculpt
