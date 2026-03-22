#include "export/GltfExporter.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <vector>
#include <unordered_map>

namespace sculpt {

ExportPreset GltfExporter::presetDefault() {
    return {"Default", true, true, true, false};
}

ExportPreset GltfExporter::presetUnity() {
    return {"Unity", true, true, true, false};
}

ExportPreset GltfExporter::presetGodot() {
    return {"Godot", true, true, true, false};
}

ExportPreset GltfExporter::presetWeb() {
    return {"Web (glb)", true, true, true, true};
}

// Helper: write a float as 4 bytes (little-endian)
static void writeFloat(std::vector<uint8_t>& buf, float v) {
    uint8_t bytes[4];
    std::memcpy(bytes, &v, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
}

static void writeUint32(std::vector<uint8_t>& buf, uint32_t v) {
    uint8_t bytes[4];
    std::memcpy(bytes, &v, 4);
    buf.insert(buf.end(), bytes, bytes + 4);
}

static void writeUint16(std::vector<uint8_t>& buf, uint16_t v) {
    uint8_t bytes[2];
    std::memcpy(bytes, &v, 2);
    buf.insert(buf.end(), bytes, bytes + 2);
}

// Pad buffer to alignment
static void padTo(std::vector<uint8_t>& buf, size_t alignment, uint8_t padByte = 0) {
    while (buf.size() % alignment != 0) buf.push_back(padByte);
}

bool GltfExporter::exportGltf(const std::string& filepath, const HalfEdgeMesh& mesh,
                                const ExportPreset& preset) {
    // Gather vertex and index data
    std::vector<float> positions, normals, colors;
    std::vector<uint32_t> indices;

    // Compute bounds
    Vec3 minPos(1e30f, 1e30f, 1e30f), maxPos(-1e30f, -1e30f, -1e30f);

    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        const auto& v = mesh.vertex(static_cast<int32_t>(i));
        positions.push_back(v.position.x);
        positions.push_back(v.position.y);
        positions.push_back(v.position.z);
        normals.push_back(v.normal.x);
        normals.push_back(v.normal.y);
        normals.push_back(v.normal.z);
        if (preset.includeVertexColors) {
            colors.push_back(v.color.x);
            colors.push_back(v.color.y);
            colors.push_back(v.color.z);
        }
        if (v.position.x < minPos.x) minPos.x = v.position.x;
        if (v.position.y < minPos.y) minPos.y = v.position.y;
        if (v.position.z < minPos.z) minPos.z = v.position.z;
        if (v.position.x > maxPos.x) maxPos.x = v.position.x;
        if (v.position.y > maxPos.y) maxPos.y = v.position.y;
        if (v.position.z > maxPos.z) maxPos.z = v.position.z;
    }

    for (size_t fi = 0; fi < mesh.faceCount(); fi++) {
        if (mesh.isFaceDeleted(static_cast<int32_t>(fi))) continue;
        auto fv = mesh.faceVertices(static_cast<int32_t>(fi));
        if (fv[0] < 0) continue;
        if (preset.removeDegenerates) {
            if (fv[0] == fv[1] || fv[1] == fv[2] || fv[0] == fv[2]) continue;
        }
        indices.push_back(static_cast<uint32_t>(fv[0]));
        indices.push_back(static_cast<uint32_t>(fv[1]));
        indices.push_back(static_cast<uint32_t>(fv[2]));
    }

    uint32_t vertCount = static_cast<uint32_t>(mesh.vertexCount());
    uint32_t idxCount = static_cast<uint32_t>(indices.size());

    // Build binary buffer
    std::vector<uint8_t> binBuf;

    // Indices
    size_t idxOffset = 0;
    size_t idxSize = idxCount * sizeof(uint32_t);
    for (uint32_t idx : indices) writeUint32(binBuf, idx);
    padTo(binBuf, 4);

    // Positions
    size_t posOffset = binBuf.size();
    size_t posSize = vertCount * 3 * sizeof(float);
    for (float f : positions) writeFloat(binBuf, f);
    padTo(binBuf, 4);

    // Normals
    size_t nrmOffset = binBuf.size();
    size_t nrmSize = vertCount * 3 * sizeof(float);
    for (float f : normals) writeFloat(binBuf, f);
    padTo(binBuf, 4);

    // Colors (optional)
    size_t colOffset = 0, colSize = 0;
    if (preset.includeVertexColors) {
        colOffset = binBuf.size();
        colSize = vertCount * 3 * sizeof(float);
        for (float f : colors) writeFloat(binBuf, f);
        padTo(binBuf, 4);
    }

    size_t totalBufSize = binBuf.size();

    // Build JSON
    std::ostringstream json;
    json << "{\n";
    json << "  \"asset\": { \"version\": \"2.0\", \"generator\": \"SculptApp\" },\n";
    json << "  \"scene\": 0,\n";
    json << "  \"scenes\": [{ \"nodes\": [0] }],\n";
    json << "  \"nodes\": [{ \"mesh\": 0 }],\n";

    // Mesh
    json << "  \"meshes\": [{ \"primitives\": [{ \"attributes\": { ";
    json << "\"POSITION\": 1, \"NORMAL\": 2";
    if (preset.includeVertexColors) json << ", \"COLOR_0\": 3";
    json << " }, \"indices\": 0 }] }],\n";

    // Accessors
    int accIdx = 0;
    json << "  \"accessors\": [\n";
    // 0: indices
    json << "    { \"bufferView\": 0, \"componentType\": 5125, \"count\": " << idxCount
         << ", \"type\": \"SCALAR\" },\n";
    // 1: positions
    json << "    { \"bufferView\": 1, \"componentType\": 5126, \"count\": " << vertCount
         << ", \"type\": \"VEC3\", \"min\": ["
         << minPos.x << "," << minPos.y << "," << minPos.z << "], \"max\": ["
         << maxPos.x << "," << maxPos.y << "," << maxPos.z << "] },\n";
    // 2: normals
    json << "    { \"bufferView\": 2, \"componentType\": 5126, \"count\": " << vertCount
         << ", \"type\": \"VEC3\" }";
    if (preset.includeVertexColors) {
        json << ",\n";
        // 3: colors
        json << "    { \"bufferView\": 3, \"componentType\": 5126, \"count\": " << vertCount
             << ", \"type\": \"VEC3\" }\n";
    } else {
        json << "\n";
    }
    json << "  ],\n";

    // Buffer views
    json << "  \"bufferViews\": [\n";
    json << "    { \"buffer\": 0, \"byteOffset\": " << idxOffset << ", \"byteLength\": " << idxSize
         << ", \"target\": 34963 },\n";
    json << "    { \"buffer\": 0, \"byteOffset\": " << posOffset << ", \"byteLength\": " << posSize
         << ", \"target\": 34962 },\n";
    json << "    { \"buffer\": 0, \"byteOffset\": " << nrmOffset << ", \"byteLength\": " << nrmSize
         << ", \"target\": 34962 }";
    if (preset.includeVertexColors) {
        json << ",\n";
        json << "    { \"buffer\": 0, \"byteOffset\": " << colOffset << ", \"byteLength\": " << colSize
             << ", \"target\": 34962 }\n";
    } else {
        json << "\n";
    }
    json << "  ],\n";

    std::string jsonStr;
    if (preset.binary) {
        // .glb: buffer references internal chunk
        json << "  \"buffers\": [{ \"byteLength\": " << totalBufSize << " }]\n";
        json << "}\n";
        jsonStr = json.str();

        // Write GLB file
        padTo(binBuf, 4);

        // Pad JSON to 4-byte alignment
        while (jsonStr.size() % 4 != 0) jsonStr += ' ';

        uint32_t jsonChunkLen = static_cast<uint32_t>(jsonStr.size());
        uint32_t binChunkLen = static_cast<uint32_t>(binBuf.size());
        uint32_t totalLen = 12 + 8 + jsonChunkLen + 8 + binChunkLen;

        std::ofstream out(filepath, std::ios::binary);
        if (!out) return false;

        // GLB header
        uint32_t magic = 0x46546C67; // "glTF"
        uint32_t version = 2;
        out.write(reinterpret_cast<const char*>(&magic), 4);
        out.write(reinterpret_cast<const char*>(&version), 4);
        out.write(reinterpret_cast<const char*>(&totalLen), 4);

        // JSON chunk
        uint32_t jsonType = 0x4E4F534A; // "JSON"
        out.write(reinterpret_cast<const char*>(&jsonChunkLen), 4);
        out.write(reinterpret_cast<const char*>(&jsonType), 4);
        out.write(jsonStr.data(), jsonChunkLen);

        // BIN chunk
        uint32_t binType = 0x004E4942; // "BIN\0"
        out.write(reinterpret_cast<const char*>(&binChunkLen), 4);
        out.write(reinterpret_cast<const char*>(&binType), 4);
        out.write(reinterpret_cast<const char*>(binBuf.data()), binChunkLen);

        return out.good();
    } else {
        // .gltf + .bin
        std::string binPath = filepath;
        size_t dotPos = binPath.rfind('.');
        if (dotPos != std::string::npos) binPath = binPath.substr(0, dotPos);
        std::string binFilename = binPath.substr(binPath.rfind('/') + 1) + ".bin";
        binPath += ".bin";

        json << "  \"buffers\": [{ \"uri\": \"" << binFilename << "\", \"byteLength\": " << totalBufSize << " }]\n";
        json << "}\n";
        jsonStr = json.str();

        // Write JSON
        std::ofstream jsonOut(filepath);
        if (!jsonOut) return false;
        jsonOut << jsonStr;
        jsonOut.close();

        // Write binary
        std::ofstream binOut(binPath, std::ios::binary);
        if (!binOut) return false;
        binOut.write(reinterpret_cast<const char*>(binBuf.data()), static_cast<long>(binBuf.size()));

        return binOut.good();
    }
}

} // namespace sculpt
