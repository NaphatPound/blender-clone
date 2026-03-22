#pragma once

#include "anim/Armature.h"
#include <vector>
#include <string>

namespace sculpt {

struct BoneKeyframe {
    float time;
    float rotation[3]; // euler XYZ
};

struct BoneTrack {
    int32_t boneIndex;
    std::vector<BoneKeyframe> keyframes; // sorted by time
};

class Animation {
public:
    std::string name;
    float duration = 0.0f;
    std::vector<BoneTrack> tracks;

    // Evaluate animation at time t, write bone rotations to armature
    void evaluate(float time, Armature& armature) const;

    // Add a keyframe for a bone
    void setKeyframe(int32_t boneIndex, float time, float rx, float ry, float rz);

    // Preset animations
    static Animation createIdle(const Armature& armature);
    static Animation createWalk(const Armature& armature);
    static Animation createWave(const Armature& armature);
    static Animation createJump(const Armature& armature);
};

struct AnimationPlayer {
    Animation* current = nullptr;
    float time = 0.0f;
    bool playing = false;
    bool loop = true;
    float speed = 1.0f;

    void update(float dt, Armature& armature);
    void play() { playing = true; }
    void pause() { playing = false; }
    void stop() { playing = false; time = 0.0f; }
};

} // namespace sculpt
