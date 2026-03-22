#pragma once

#include "core/HalfEdgeMesh.h"
#include <string>

namespace sculpt {

class ObjLoader {
public:
    static bool load(const std::string& filepath, HalfEdgeMesh& mesh);
    static bool save(const std::string& filepath, const HalfEdgeMesh& mesh);

    // Load from string data (for testing)
    static bool loadFromString(const std::string& objData, HalfEdgeMesh& mesh);
};

} // namespace sculpt
