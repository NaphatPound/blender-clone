#include "core/ObjLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>

namespace sculpt {

bool ObjLoader::loadFromString(const std::string& objData, HalfEdgeMesh& mesh) {
    std::vector<Vec3> positions;
    std::vector<std::array<uint32_t, 3>> triangles;

    std::istringstream stream(objData);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        if (prefix == "v") {
            Vec3 pos;
            lineStream >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (prefix == "f") {
            std::vector<uint32_t> faceVerts;
            std::string token;
            while (lineStream >> token) {
                // Handle formats: "1", "1/2", "1/2/3", "1//3"
                uint32_t idx = static_cast<uint32_t>(std::stoi(token.substr(0, token.find('/'))));
                faceVerts.push_back(idx - 1); // OBJ is 1-indexed
            }

            // Triangulate faces with more than 3 vertices (fan triangulation)
            for (size_t i = 1; i + 1 < faceVerts.size(); i++) {
                triangles.push_back({faceVerts[0], faceVerts[i], faceVerts[i + 1]});
            }
        }
    }

    if (positions.empty() || triangles.empty()) return false;

    mesh.buildFromTriangles(positions, triangles);
    return true;
}

bool ObjLoader::load(const std::string& filepath, HalfEdgeMesh& mesh) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open OBJ file: " << filepath << std::endl;
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return loadFromString(content, mesh);
}

bool ObjLoader::save(const std::string& filepath, const HalfEdgeMesh& mesh) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Exported from SculptApp\n";

    // Write vertices
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        const Vec3& p = mesh.vertex(static_cast<int32_t>(i)).position;
        file << "v " << p.x << " " << p.y << " " << p.z << "\n";
    }

    // Write normals
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        const Vec3& n = mesh.vertex(static_cast<int32_t>(i)).normal;
        file << "vn " << n.x << " " << n.y << " " << n.z << "\n";
    }

    // Write faces
    for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
        int32_t he0 = mesh.face(static_cast<int32_t>(fi)).halfEdge;
        int32_t he1 = mesh.halfEdge(he0).next;
        int32_t he2 = mesh.halfEdge(he1).next;

        int32_t v0 = mesh.halfEdge(he2).vertex;
        int32_t v1 = mesh.halfEdge(he0).vertex;
        int32_t v2 = mesh.halfEdge(he1).vertex;

        // OBJ is 1-indexed
        file << "f " << (v0 + 1) << "//" << (v0 + 1)
             << " " << (v1 + 1) << "//" << (v1 + 1)
             << " " << (v2 + 1) << "//" << (v2 + 1) << "\n";
    }

    return true;
}

} // namespace sculpt
