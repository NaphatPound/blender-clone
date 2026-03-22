#pragma once

#include "anim/Armature.h"
#include "render/Shader.h"
#include "render/Camera.h"
#include <cstdint>

namespace sculpt {

class BoneRenderer {
public:
    BoneRenderer() = default;
    ~BoneRenderer();

    bool init();
    void shutdown();

    void update(const Armature& armature, int32_t selectedBone = -1);
    void render(const Camera& camera, float aspectRatio);

private:
    Shader m_flatShader;
    uint32_t m_lineVAO = 0, m_lineVBO = 0;
    int m_lineVertCount = 0;
    uint32_t m_jointVAO = 0, m_jointVBO = 0;
    int m_jointCount = 0;
    uint32_t m_selVAO = 0, m_selVBO = 0;
    int m_selVertCount = 0;
    bool m_initialized = false;
};

} // namespace sculpt
