#include "anim/Animation.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace sculpt {

static float lerp(float a, float b, float t) { return a + (b - a) * t; }

void Animation::evaluate(float time, Armature& armature) const {
    for (const auto& track : tracks) {
        if (track.boneIndex < 0 || track.boneIndex >= armature.boneCount()) continue;
        if (track.keyframes.empty()) continue;

        // Clamp or wrap time
        float t = time;
        if (t <= track.keyframes.front().time) {
            auto& kf = track.keyframes.front();
            armature.bone(track.boneIndex).poseRotation[0] = kf.rotation[0];
            armature.bone(track.boneIndex).poseRotation[1] = kf.rotation[1];
            armature.bone(track.boneIndex).poseRotation[2] = kf.rotation[2];
            continue;
        }
        if (t >= track.keyframes.back().time) {
            auto& kf = track.keyframes.back();
            armature.bone(track.boneIndex).poseRotation[0] = kf.rotation[0];
            armature.bone(track.boneIndex).poseRotation[1] = kf.rotation[1];
            armature.bone(track.boneIndex).poseRotation[2] = kf.rotation[2];
            continue;
        }

        // Find surrounding keyframes
        for (size_t i = 0; i + 1 < track.keyframes.size(); i++) {
            if (t >= track.keyframes[i].time && t <= track.keyframes[i + 1].time) {
                float seg = track.keyframes[i + 1].time - track.keyframes[i].time;
                float frac = (seg > 1e-6f) ? (t - track.keyframes[i].time) / seg : 0.0f;
                // Smooth interpolation
                frac = frac * frac * (3.0f - 2.0f * frac);

                const auto& k0 = track.keyframes[i];
                const auto& k1 = track.keyframes[i + 1];
                armature.bone(track.boneIndex).poseRotation[0] = lerp(k0.rotation[0], k1.rotation[0], frac);
                armature.bone(track.boneIndex).poseRotation[1] = lerp(k0.rotation[1], k1.rotation[1], frac);
                armature.bone(track.boneIndex).poseRotation[2] = lerp(k0.rotation[2], k1.rotation[2], frac);
                break;
            }
        }
    }
}

void Animation::setKeyframe(int32_t boneIndex, float time, float rx, float ry, float rz) {
    // Find or create track
    BoneTrack* track = nullptr;
    for (auto& t : tracks) {
        if (t.boneIndex == boneIndex) { track = &t; break; }
    }
    if (!track) {
        tracks.push_back({boneIndex, {}});
        track = &tracks.back();
    }

    // Insert keyframe sorted by time
    BoneKeyframe kf;
    kf.time = time;
    kf.rotation[0] = rx;
    kf.rotation[1] = ry;
    kf.rotation[2] = rz;

    auto it = std::lower_bound(track->keyframes.begin(), track->keyframes.end(), kf,
        [](const BoneKeyframe& a, const BoneKeyframe& b) { return a.time < b.time; });

    // Replace if same time
    if (it != track->keyframes.end() && std::abs(it->time - time) < 1e-4f) {
        *it = kf;
    } else {
        track->keyframes.insert(it, kf);
    }

    // Update duration
    duration = std::max(duration, time);
}

// =====================================================
// PRESET: IDLE (breathing)
// =====================================================
Animation Animation::createIdle(const Armature& armature) {
    Animation anim;
    anim.name = "Idle";
    anim.duration = 2.0f;

    float breathAmt = 0.03f;

    int32_t spine = armature.findBone("Spine");
    int32_t chest = armature.findBone("Chest");
    int32_t head  = armature.findBone("Head");

    if (spine >= 0) {
        anim.setKeyframe(spine, 0.0f,  breathAmt, 0, 0);
        anim.setKeyframe(spine, 1.0f, -breathAmt, 0, 0);
        anim.setKeyframe(spine, 2.0f,  breathAmt, 0, 0);
    }
    if (chest >= 0) {
        anim.setKeyframe(chest, 0.0f,  breathAmt * 0.5f, 0, 0);
        anim.setKeyframe(chest, 1.0f, -breathAmt * 0.5f, 0, 0);
        anim.setKeyframe(chest, 2.0f,  breathAmt * 0.5f, 0, 0);
    }
    if (head >= 0) {
        anim.setKeyframe(head, 0.0f, 0, 0, 0);
        anim.setKeyframe(head, 1.0f, 0.02f, 0.01f, 0);
        anim.setKeyframe(head, 2.0f, 0, 0, 0);
    }

    return anim;
}

// =====================================================
// PRESET: WALK CYCLE
// =====================================================
Animation Animation::createWalk(const Armature& armature) {
    Animation anim;
    anim.name = "Walk";
    anim.duration = 1.0f;

    float pi = static_cast<float>(M_PI);
    float legSwing = 0.5f;   // ~30 degrees
    float armSwing = 0.35f;
    float hipSway = 0.03f;

    int32_t hips = armature.findBone("Hips");
    int32_t lThigh = armature.findBone("L_Thigh");
    int32_t rThigh = armature.findBone("R_Thigh");
    int32_t lShin = armature.findBone("L_Shin");
    int32_t rShin = armature.findBone("R_Shin");
    int32_t lArm = armature.findBone("L_UpperArm");
    int32_t rArm = armature.findBone("R_UpperArm");
    int32_t spine = armature.findBone("Spine");

    // Hips sway
    if (hips >= 0) {
        anim.setKeyframe(hips, 0.0f,  0, 0, hipSway);
        anim.setKeyframe(hips, 0.25f, 0, 0, 0);
        anim.setKeyframe(hips, 0.5f,  0, 0, -hipSway);
        anim.setKeyframe(hips, 0.75f, 0, 0, 0);
        anim.setKeyframe(hips, 1.0f,  0, 0, hipSway);
    }

    // Left leg: forward at 0, back at 0.5
    if (lThigh >= 0) {
        anim.setKeyframe(lThigh, 0.0f,  legSwing, 0, 0);
        anim.setKeyframe(lThigh, 0.25f, 0, 0, 0);
        anim.setKeyframe(lThigh, 0.5f, -legSwing, 0, 0);
        anim.setKeyframe(lThigh, 0.75f, 0, 0, 0);
        anim.setKeyframe(lThigh, 1.0f,  legSwing, 0, 0);
    }
    if (lShin >= 0) {
        anim.setKeyframe(lShin, 0.0f,  0, 0, 0);
        anim.setKeyframe(lShin, 0.25f, -legSwing * 0.8f, 0, 0);
        anim.setKeyframe(lShin, 0.5f,  0, 0, 0);
        anim.setKeyframe(lShin, 0.75f, -legSwing * 0.3f, 0, 0);
        anim.setKeyframe(lShin, 1.0f,  0, 0, 0);
    }

    // Right leg: opposite phase
    if (rThigh >= 0) {
        anim.setKeyframe(rThigh, 0.0f, -legSwing, 0, 0);
        anim.setKeyframe(rThigh, 0.25f, 0, 0, 0);
        anim.setKeyframe(rThigh, 0.5f,  legSwing, 0, 0);
        anim.setKeyframe(rThigh, 0.75f, 0, 0, 0);
        anim.setKeyframe(rThigh, 1.0f, -legSwing, 0, 0);
    }
    if (rShin >= 0) {
        anim.setKeyframe(rShin, 0.0f,  0, 0, 0);
        anim.setKeyframe(rShin, 0.25f, -legSwing * 0.3f, 0, 0);
        anim.setKeyframe(rShin, 0.5f,  0, 0, 0);
        anim.setKeyframe(rShin, 0.75f, -legSwing * 0.8f, 0, 0);
        anim.setKeyframe(rShin, 1.0f,  0, 0, 0);
    }

    // Arms swing opposite to legs
    if (lArm >= 0) {
        anim.setKeyframe(lArm, 0.0f, -armSwing, 0, 0);
        anim.setKeyframe(lArm, 0.5f,  armSwing, 0, 0);
        anim.setKeyframe(lArm, 1.0f, -armSwing, 0, 0);
    }
    if (rArm >= 0) {
        anim.setKeyframe(rArm, 0.0f,  armSwing, 0, 0);
        anim.setKeyframe(rArm, 0.5f, -armSwing, 0, 0);
        anim.setKeyframe(rArm, 1.0f,  armSwing, 0, 0);
    }

    // Spine counter-rotation
    if (spine >= 0) {
        anim.setKeyframe(spine, 0.0f,  0.02f, 0, 0);
        anim.setKeyframe(spine, 0.5f, -0.02f, 0, 0);
        anim.setKeyframe(spine, 1.0f,  0.02f, 0, 0);
    }

    return anim;
}

// =====================================================
// PRESET: WAVE
// =====================================================
Animation Animation::createWave(const Armature& armature) {
    Animation anim;
    anim.name = "Wave";
    anim.duration = 1.5f;

    int32_t rArm = armature.findBone("R_UpperArm");
    int32_t rFore = armature.findBone("R_Forearm");

    if (rArm >= 0) {
        anim.setKeyframe(rArm, 0.0f, 0, 0, -2.0f);
        anim.setKeyframe(rArm, 1.5f, 0, 0, -2.0f);
    }
    if (rFore >= 0) {
        anim.setKeyframe(rFore, 0.0f,   0, 0, 0.4f);
        anim.setKeyframe(rFore, 0.375f,  0, 0, -0.4f);
        anim.setKeyframe(rFore, 0.75f,   0, 0, 0.4f);
        anim.setKeyframe(rFore, 1.125f,  0, 0, -0.4f);
        anim.setKeyframe(rFore, 1.5f,    0, 0, 0.4f);
    }

    return anim;
}

// =====================================================
// PRESET: JUMP
// =====================================================
Animation Animation::createJump(const Armature& armature) {
    Animation anim;
    anim.name = "Jump";
    anim.duration = 1.2f;

    int32_t hips = armature.findBone("Hips");
    int32_t lThigh = armature.findBone("L_Thigh");
    int32_t rThigh = armature.findBone("R_Thigh");
    int32_t lShin = armature.findBone("L_Shin");
    int32_t rShin = armature.findBone("R_Shin");
    int32_t lArm = armature.findBone("L_UpperArm");
    int32_t rArm = armature.findBone("R_UpperArm");

    // Crouch
    float crouch = 0.5f;
    if (lThigh >= 0) { anim.setKeyframe(lThigh, 0.0f, 0,0,0); anim.setKeyframe(lThigh, 0.3f, crouch,0,0); anim.setKeyframe(lThigh, 0.5f, -0.2f,0,0); anim.setKeyframe(lThigh, 0.9f, -0.2f,0,0); anim.setKeyframe(lThigh, 1.2f, 0,0,0); }
    if (rThigh >= 0) { anim.setKeyframe(rThigh, 0.0f, 0,0,0); anim.setKeyframe(rThigh, 0.3f, crouch,0,0); anim.setKeyframe(rThigh, 0.5f, -0.2f,0,0); anim.setKeyframe(rThigh, 0.9f, -0.2f,0,0); anim.setKeyframe(rThigh, 1.2f, 0,0,0); }
    if (lShin >= 0)  { anim.setKeyframe(lShin, 0.0f, 0,0,0); anim.setKeyframe(lShin, 0.3f, -crouch*1.2f,0,0); anim.setKeyframe(lShin, 0.5f, 0,0,0); anim.setKeyframe(lShin, 0.9f, -0.3f,0,0); anim.setKeyframe(lShin, 1.2f, 0,0,0); }
    if (rShin >= 0)  { anim.setKeyframe(rShin, 0.0f, 0,0,0); anim.setKeyframe(rShin, 0.3f, -crouch*1.2f,0,0); anim.setKeyframe(rShin, 0.5f, 0,0,0); anim.setKeyframe(rShin, 0.9f, -0.3f,0,0); anim.setKeyframe(rShin, 1.2f, 0,0,0); }
    // Arms up during jump
    if (lArm >= 0) { anim.setKeyframe(lArm, 0.0f, 0,0,0); anim.setKeyframe(lArm, 0.4f, 0,0,0.8f); anim.setKeyframe(lArm, 0.9f, 0,0,0.8f); anim.setKeyframe(lArm, 1.2f, 0,0,0); }
    if (rArm >= 0) { anim.setKeyframe(rArm, 0.0f, 0,0,0); anim.setKeyframe(rArm, 0.4f, 0,0,-0.8f); anim.setKeyframe(rArm, 0.9f, 0,0,-0.8f); anim.setKeyframe(rArm, 1.2f, 0,0,0); }

    return anim;
}

// =====================================================
// AnimationPlayer
// =====================================================
void AnimationPlayer::update(float dt, Armature& armature) {
    if (!playing || !current) return;

    time += dt * speed;

    if (loop && current->duration > 0) {
        while (time >= current->duration) time -= current->duration;
        while (time < 0) time += current->duration;
    } else {
        time = std::max(0.0f, std::min(time, current->duration));
        if (time >= current->duration) playing = false;
    }

    current->evaluate(time, armature);
    armature.updatePoseMatrices();
}

} // namespace sculpt
