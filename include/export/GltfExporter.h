#pragma once

#include "core/HalfEdgeMesh.h"
#include <string>

namespace sculpt {

struct ExportPreset {
    std::string name = "Default";
    bool mergeVertices = true;
    bool removeDegenerates = true;
    bool includeVertexColors = true;
    bool binary = false; // .glb vs .gltf+.bin
};

class GltfExporter {
public:
    static bool exportGltf(const std::string& filepath, const HalfEdgeMesh& mesh,
                            const ExportPreset& preset = ExportPreset());

    static ExportPreset presetDefault();
    static ExportPreset presetUnity();
    static ExportPreset presetGodot();
    static ExportPreset presetWeb();
};

} // namespace sculpt
