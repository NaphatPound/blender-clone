#include "core/MeshGenerator.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple hash-based noise for procedural generation
namespace {
float hashFloat(int x, int y, int z, int seed) {
    int h = seed + x * 374761393 + y * 668265263 + z * 1274126177;
    h = (h ^ (h >> 13)) * 1274126177;
    h = h ^ (h >> 16);
    return static_cast<float>(h & 0x7FFFFFFF) / 2147483647.0f;
}

float noise3D(float x, float y, float z, int seed) {
    int ix = static_cast<int>(std::floor(x));
    int iy = static_cast<int>(std::floor(y));
    int iz = static_cast<int>(std::floor(z));
    float fx = x - std::floor(x);
    float fy = y - std::floor(y);
    float fz = z - std::floor(z);
    // Smoothstep
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    fz = fz * fz * (3.0f - 2.0f * fz);
    // Trilinear interpolation
    float c000 = hashFloat(ix, iy, iz, seed);
    float c100 = hashFloat(ix+1, iy, iz, seed);
    float c010 = hashFloat(ix, iy+1, iz, seed);
    float c110 = hashFloat(ix+1, iy+1, iz, seed);
    float c001 = hashFloat(ix, iy, iz+1, seed);
    float c101 = hashFloat(ix+1, iy, iz+1, seed);
    float c011 = hashFloat(ix, iy+1, iz+1, seed);
    float c111 = hashFloat(ix+1, iy+1, iz+1, seed);
    float a = c000 + (c100-c000)*fx; float b = c010 + (c110-c010)*fx;
    float c = c001 + (c101-c001)*fx; float d = c011 + (c111-c011)*fx;
    float e = a + (b-a)*fy; float f = c + (d-c)*fy;
    return e + (f-e)*fz;
}

float fbmNoise(float x, float y, float z, int octaves, int seed) {
    float val = 0, amp = 1.0f, freq = 1.0f, total = 0;
    for (int i = 0; i < octaves; i++) {
        val += noise3D(x*freq, y*freq, z*freq, seed+i*31) * amp;
        total += amp;
        amp *= 0.5f; freq *= 2.0f;
    }
    return val / total;
}

// Helper to merge two meshes (appends B's geometry into A's arrays)
void mergeMeshData(std::vector<sculpt::Vec3>& posA, std::vector<std::array<uint32_t,3>>& triA,
                   const std::vector<sculpt::Vec3>& posB, const std::vector<std::array<uint32_t,3>>& triB,
                   sculpt::Vec3 offset = {0,0,0}) {
    uint32_t base = static_cast<uint32_t>(posA.size());
    for (const auto& p : posB) posA.push_back(p + offset);
    for (const auto& t : triB) triA.push_back({t[0]+base, t[1]+base, t[2]+base});
}
} // anonymous namespace

namespace sculpt {

HalfEdgeMesh MeshGenerator::createSphere(float radius, int segments, int rings) {
    std::vector<Vec3> positions;
    std::vector<std::array<uint32_t, 3>> triangles;

    // Top pole
    positions.push_back({0, radius, 0});

    // Ring vertices
    for (int ring = 1; ring < rings; ring++) {
        float phi = static_cast<float>(M_PI) * static_cast<float>(ring) / static_cast<float>(rings);
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int seg = 0; seg < segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            float x = radius * sinPhi * std::cos(theta);
            float y = radius * cosPhi;
            float z = radius * sinPhi * std::sin(theta);
            positions.push_back({x, y, z});
        }
    }

    // Bottom pole
    positions.push_back({0, -radius, 0});

    uint32_t topPole = 0;
    uint32_t bottomPole = static_cast<uint32_t>(positions.size() - 1);

    // Top cap triangles (CCW when viewed from outside = above)
    for (int seg = 0; seg < segments; seg++) {
        uint32_t next = static_cast<uint32_t>((seg + 1) % segments + 1);
        uint32_t curr = static_cast<uint32_t>(seg + 1);
        triangles.push_back({topPole, next, curr});
    }

    // Middle quads (as triangles, CCW when viewed from outside)
    for (int ring = 0; ring < rings - 2; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            uint32_t curr = static_cast<uint32_t>(ring * segments + seg + 1);
            uint32_t next = static_cast<uint32_t>(ring * segments + (seg + 1) % segments + 1);
            uint32_t currBelow = static_cast<uint32_t>((ring + 1) * segments + seg + 1);
            uint32_t nextBelow = static_cast<uint32_t>((ring + 1) * segments + (seg + 1) % segments + 1);

            triangles.push_back({curr, next, currBelow});
            triangles.push_back({next, nextBelow, currBelow});
        }
    }

    // Bottom cap triangles (CCW when viewed from outside = below)
    uint32_t lastRingStart = static_cast<uint32_t>((rings - 2) * segments + 1);
    for (int seg = 0; seg < segments; seg++) {
        uint32_t curr = lastRingStart + static_cast<uint32_t>(seg);
        uint32_t next = lastRingStart + static_cast<uint32_t>((seg + 1) % segments);
        triangles.push_back({curr, next, bottomPole});
    }

    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(positions, triangles);
    return mesh;
}

HalfEdgeMesh MeshGenerator::createCube(float size) {
    float h = size * 0.5f;
    std::vector<Vec3> positions = {
        {-h, -h, -h}, { h, -h, -h}, { h,  h, -h}, {-h,  h, -h},  // back
        {-h, -h,  h}, { h, -h,  h}, { h,  h,  h}, {-h,  h,  h},  // front
    };

    std::vector<std::array<uint32_t, 3>> triangles = {
        // Front
        {4, 5, 6}, {4, 6, 7},
        // Back
        {1, 0, 3}, {1, 3, 2},
        // Left
        {0, 4, 7}, {0, 7, 3},
        // Right
        {5, 1, 2}, {5, 2, 6},
        // Top
        {7, 6, 2}, {7, 2, 3},
        // Bottom
        {0, 1, 5}, {0, 5, 4},
    };

    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(positions, triangles);
    return mesh;
}

HalfEdgeMesh MeshGenerator::createPlane(float size, int subdivisions) {
    std::vector<Vec3> positions;
    std::vector<std::array<uint32_t, 3>> triangles;

    float h = size * 0.5f;
    int verts = subdivisions + 1;

    for (int y = 0; y <= subdivisions; y++) {
        for (int x = 0; x <= subdivisions; x++) {
            float px = -h + size * static_cast<float>(x) / static_cast<float>(subdivisions);
            float pz = -h + size * static_cast<float>(y) / static_cast<float>(subdivisions);
            positions.push_back({px, 0, pz});
        }
    }

    for (int y = 0; y < subdivisions; y++) {
        for (int x = 0; x < subdivisions; x++) {
            uint32_t tl = static_cast<uint32_t>(y * verts + x);
            uint32_t tr = tl + 1;
            uint32_t bl = static_cast<uint32_t>((y + 1) * verts + x);
            uint32_t br = bl + 1;

            triangles.push_back({tl, bl, tr});
            triangles.push_back({tr, bl, br});
        }
    }

    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(positions, triangles);
    return mesh;
}

// =====================================================
// CYLINDER
// =====================================================
HalfEdgeMesh MeshGenerator::createCylinder(float radius, float height, int segments) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    float h2 = height * 0.5f;
    // Bottom center + top center
    uint32_t botCenter = 0;
    pos.push_back({0, -h2, 0});
    uint32_t topCenter = 1;
    pos.push_back({0, h2, 0});
    // Bottom ring (starting at index 2)
    for (int i = 0; i < segments; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        pos.push_back({radius * std::cos(a), -h2, radius * std::sin(a)});
    }
    // Top ring
    uint32_t topStart = static_cast<uint32_t>(pos.size());
    for (int i = 0; i < segments; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        pos.push_back({radius * std::cos(a), h2, radius * std::sin(a)});
    }
    uint32_t botStart = 2;
    for (int i = 0; i < segments; i++) {
        uint32_t next = static_cast<uint32_t>((i + 1) % segments);
        // Bottom cap
        tris.push_back({botCenter, botStart + next, botStart + static_cast<uint32_t>(i)});
        // Top cap
        tris.push_back({topCenter, topStart + static_cast<uint32_t>(i), topStart + next});
        // Side quads
        uint32_t b0 = botStart + static_cast<uint32_t>(i), b1 = botStart + next;
        uint32_t t0 = topStart + static_cast<uint32_t>(i), t1 = topStart + next;
        tris.push_back({b0, t0, b1});
        tris.push_back({b1, t0, t1});
    }
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// CONE
// =====================================================
HalfEdgeMesh MeshGenerator::createCone(float radius, float height, int segments) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    float h2 = height * 0.5f;
    uint32_t apex = 0;
    pos.push_back({0, h2, 0});
    uint32_t botCenter = 1;
    pos.push_back({0, -h2, 0});
    uint32_t ringStart = 2;
    for (int i = 0; i < segments; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(segments);
        pos.push_back({radius * std::cos(a), -h2, radius * std::sin(a)});
    }
    for (int i = 0; i < segments; i++) {
        uint32_t next = static_cast<uint32_t>((i + 1) % segments);
        tris.push_back({apex, ringStart + static_cast<uint32_t>(i), ringStart + next});
        tris.push_back({botCenter, ringStart + next, ringStart + static_cast<uint32_t>(i)});
    }
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// TORUS
// =====================================================
HalfEdgeMesh MeshGenerator::createTorus(float majorR, float minorR, int majorSegs, int minorSegs) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    for (int i = 0; i < majorSegs; i++) {
        float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(majorSegs);
        float ct = std::cos(theta), st = std::sin(theta);
        for (int j = 0; j < minorSegs; j++) {
            float phi = 2.0f * static_cast<float>(M_PI) * static_cast<float>(j) / static_cast<float>(minorSegs);
            float cp = std::cos(phi), sp = std::sin(phi);
            float r = majorR + minorR * cp;
            pos.push_back({r * ct, minorR * sp, r * st});
        }
    }
    for (int i = 0; i < majorSegs; i++) {
        int ni = (i + 1) % majorSegs;
        for (int j = 0; j < minorSegs; j++) {
            int nj = (j + 1) % minorSegs;
            uint32_t a = static_cast<uint32_t>(i * minorSegs + j);
            uint32_t b = static_cast<uint32_t>(ni * minorSegs + j);
            uint32_t c = static_cast<uint32_t>(ni * minorSegs + nj);
            uint32_t d = static_cast<uint32_t>(i * minorSegs + nj);
            tris.push_back({a, b, c});
            tris.push_back({a, c, d});
        }
    }
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// CAPSULE
// =====================================================
HalfEdgeMesh MeshGenerator::createCapsule(float radius, float height, int segments, int rings) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    float bodyH = height * 0.5f;
    int halfRings = std::max(rings / 2, 2);
    // Top hemisphere
    pos.push_back({0, bodyH + radius, 0}); // top pole
    for (int ring = 1; ring <= halfRings; ring++) {
        float phi = static_cast<float>(M_PI) * 0.5f * static_cast<float>(ring) / static_cast<float>(halfRings);
        for (int seg = 0; seg < segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            pos.push_back({radius * std::sin(phi) * std::cos(theta),
                           bodyH + radius * std::cos(phi),
                           radius * std::sin(phi) * std::sin(theta)});
        }
    }
    // Bottom hemisphere
    for (int ring = 0; ring < halfRings; ring++) {
        float phi = static_cast<float>(M_PI) * 0.5f + static_cast<float>(M_PI) * 0.5f * static_cast<float>(ring) / static_cast<float>(halfRings);
        for (int seg = 0; seg < segments; seg++) {
            float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(seg) / static_cast<float>(segments);
            pos.push_back({radius * std::sin(phi) * std::cos(theta),
                           -bodyH + radius * std::cos(phi),
                           radius * std::sin(phi) * std::sin(theta)});
        }
    }
    pos.push_back({0, -bodyH - radius, 0}); // bottom pole
    uint32_t topPole = 0;
    uint32_t botPole = static_cast<uint32_t>(pos.size() - 1);
    int totalRings = halfRings * 2;
    // Top cap
    for (int s = 0; s < segments; s++) {
        uint32_t next = static_cast<uint32_t>((s + 1) % segments + 1);
        uint32_t curr = static_cast<uint32_t>(s + 1);
        tris.push_back({topPole, next, curr});
    }
    // Body rings
    for (int r = 0; r < totalRings - 1; r++) {
        for (int s = 0; s < segments; s++) {
            uint32_t c = static_cast<uint32_t>(r * segments + s + 1);
            uint32_t n = static_cast<uint32_t>(r * segments + (s+1)%segments + 1);
            uint32_t cb = static_cast<uint32_t>((r+1) * segments + s + 1);
            uint32_t nb = static_cast<uint32_t>((r+1) * segments + (s+1)%segments + 1);
            tris.push_back({c, n, cb});
            tris.push_back({n, nb, cb});
        }
    }
    // Bottom cap
    uint32_t lastRing = static_cast<uint32_t>((totalRings - 1) * segments + 1);
    for (int s = 0; s < segments; s++) {
        uint32_t curr = lastRing + static_cast<uint32_t>(s);
        uint32_t next = lastRing + static_cast<uint32_t>((s+1) % segments);
        tris.push_back({curr, next, botPole});
    }
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// WEDGE
// =====================================================
HalfEdgeMesh MeshGenerator::createWedge(float w, float h, float d) {
    float w2 = w*0.5f, d2 = d*0.5f;
    std::vector<Vec3> pos = {
        {-w2, 0, -d2}, {w2, 0, -d2}, {w2, 0, d2}, {-w2, 0, d2}, // bottom
        {-w2, h, -d2}, {w2, h, -d2}  // top edge
    };
    std::vector<std::array<uint32_t,3>> tris = {
        {0,2,1}, {0,3,2},    // bottom
        {0,1,5}, {0,5,4},    // front
        {2,3,4}, {2,4,5},    // back (slope)
        {0,4,3},              // left triangle
        {1,2,5}               // right triangle
    };
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// PYRAMID
// =====================================================
HalfEdgeMesh MeshGenerator::createPyramid(float base, float height) {
    float h2 = base * 0.5f;
    std::vector<Vec3> pos = {
        {-h2, 0, -h2}, {h2, 0, -h2}, {h2, 0, h2}, {-h2, 0, h2}, // base
        {0, height, 0} // apex
    };
    std::vector<std::array<uint32_t,3>> tris = {
        {0,2,1}, {0,3,2},   // base
        {0,1,4}, {1,2,4}, {2,3,4}, {3,0,4}  // sides
    };
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    return mesh;
}

// =====================================================
// PROCEDURAL: ROCK
// =====================================================
HalfEdgeMesh MeshGenerator::generateRock(const RockParams& p) {
    HalfEdgeMesh mesh = createSphere(p.radius, p.segments, p.rings);
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        Vec3& pos = mesh.vertex(static_cast<int32_t>(i)).position;
        Vec3 n = pos.normalized();
        float nv = fbmNoise(pos.x * 3.0f, pos.y * 3.0f, pos.z * 3.0f, 3, p.seed);
        float displacement = (nv - 0.5f) * 2.0f * p.roughness * p.radius;
        pos += n * displacement;
        // Slightly flatten bottom
        if (pos.y < -p.radius * 0.3f) pos.y *= 0.7f;
    }
    mesh.recomputeNormals();
    // Color: gray-brown tones
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        float nv = hashFloat(static_cast<int>(i), p.seed, 0, 777);
        mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.45f + nv*0.15f, 0.42f + nv*0.12f, 0.38f + nv*0.1f);
    }
    return mesh;
}

// =====================================================
// PROCEDURAL: TREE
// =====================================================
HalfEdgeMesh MeshGenerator::generateTree(const TreeParams& p) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;

    // Trunk (cylinder)
    std::vector<Vec3> trunkPos;
    std::vector<std::array<uint32_t,3>> trunkTris;
    int ts = p.trunkSegs;
    trunkPos.push_back({0, 0, 0}); // bottom center
    trunkPos.push_back({0, p.trunkHeight, 0}); // top center
    for (int i = 0; i < ts; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(ts);
        trunkPos.push_back({p.trunkRadius * std::cos(a), 0, p.trunkRadius * std::sin(a)});
    }
    uint32_t topStart = static_cast<uint32_t>(trunkPos.size());
    for (int i = 0; i < ts; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(ts);
        float r = p.trunkRadius * 0.7f; // taper
        trunkPos.push_back({r * std::cos(a), p.trunkHeight, r * std::sin(a)});
    }
    for (int i = 0; i < ts; i++) {
        uint32_t ni = static_cast<uint32_t>((i+1) % ts);
        trunkTris.push_back({0, 2+ni, 2+static_cast<uint32_t>(i)});
        uint32_t b0=2+static_cast<uint32_t>(i), b1=2+ni, t0=topStart+static_cast<uint32_t>(i), t1=topStart+ni;
        trunkTris.push_back({b0, t0, b1});
        trunkTris.push_back({b1, t0, t1});
    }
    mergeMeshData(pos, tris, trunkPos, trunkTris);

    // Foliage
    std::vector<Vec3> fPos;
    std::vector<std::array<uint32_t,3>> fTris;
    int fs = p.foliageRings;
    float fr = p.foliageRadius;

    if (p.style == 1) {
        // Cone foliage
        fPos.push_back({0, fr * 2.0f, 0}); // apex
        fPos.push_back({0, 0, 0}); // center bottom
        for (int i = 0; i < fs * 2; i++) {
            float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(fs * 2);
            fPos.push_back({fr * std::cos(a), 0, fr * std::sin(a)});
        }
        for (int i = 0; i < fs * 2; i++) {
            uint32_t ni = static_cast<uint32_t>((i+1) % (fs*2));
            fTris.push_back({0, 2+static_cast<uint32_t>(i), 2+ni});
            fTris.push_back({1, 2+ni, 2+static_cast<uint32_t>(i)});
        }
    } else {
        // Sphere foliage (style 0 or 2)
        // Build a simple icosphere-like shape
        int segs = std::max(fs, 6);
        int rings = std::max(fs / 2, 3);
        fPos.push_back({0, fr, 0}); // top
        for (int r = 1; r < rings; r++) {
            float phi = static_cast<float>(M_PI) * static_cast<float>(r) / static_cast<float>(rings);
            for (int s = 0; s < segs; s++) {
                float theta = 2.0f * static_cast<float>(M_PI) * static_cast<float>(s) / static_cast<float>(segs);
                float y = fr * std::cos(phi);
                float xz = fr * std::sin(phi);
                if (p.style == 2) xz *= 1.5f; // flat style: wider
                if (p.style == 2) y *= 0.5f;
                fPos.push_back({xz * std::cos(theta), y, xz * std::sin(theta)});
            }
        }
        fPos.push_back({0, -fr * (p.style == 2 ? 0.5f : 1.0f), 0}); // bottom
        uint32_t tp = 0, bp = static_cast<uint32_t>(fPos.size()-1);
        for (int s = 0; s < segs; s++) {
            uint32_t ns = static_cast<uint32_t>((s+1)%segs+1);
            fTris.push_back({tp, ns, static_cast<uint32_t>(s+1)});
        }
        for (int r = 0; r < rings-2; r++) {
            for (int s = 0; s < segs; s++) {
                uint32_t c = static_cast<uint32_t>(r*segs+s+1);
                uint32_t n = static_cast<uint32_t>(r*segs+(s+1)%segs+1);
                uint32_t cb = static_cast<uint32_t>((r+1)*segs+s+1);
                uint32_t nb = static_cast<uint32_t>((r+1)*segs+(s+1)%segs+1);
                fTris.push_back({c, n, cb});
                fTris.push_back({n, nb, cb});
            }
        }
        uint32_t lr = static_cast<uint32_t>((rings-2)*segs+1);
        for (int s = 0; s < segs; s++) {
            uint32_t curr = lr + static_cast<uint32_t>(s);
            uint32_t next = lr + static_cast<uint32_t>((s+1)%segs);
            fTris.push_back({curr, next, bp});
        }
    }
    Vec3 foliageOffset = {0, p.trunkHeight + fr * 0.3f, 0};
    mergeMeshData(pos, tris, fPos, fTris, foliageOffset);

    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);

    // Color: brown trunk, green foliage
    uint32_t trunkVerts = static_cast<uint32_t>(trunkPos.size());
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        float nv = hashFloat(static_cast<int>(i), p.seed, 0, 999);
        if (i < trunkVerts) {
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.4f+nv*0.1f, 0.28f+nv*0.05f, 0.15f);
        } else {
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.2f+nv*0.15f, 0.5f+nv*0.2f, 0.15f+nv*0.1f);
        }
    }

    // Add noise to foliage vertices
    for (size_t i = trunkVerts; i < mesh.vertexCount(); i++) {
        Vec3& v = mesh.vertex(static_cast<int32_t>(i)).position;
        float n = fbmNoise(v.x*2, v.y*2, v.z*2, 2, p.seed) - 0.5f;
        v += v.normalized() * (n * fr * 0.2f);
    }
    mesh.recomputeNormals();
    return mesh;
}

// =====================================================
// PROCEDURAL: TERRAIN
// =====================================================
HalfEdgeMesh MeshGenerator::generateTerrain(const TerrainParams& p) {
    HalfEdgeMesh mesh = createPlane(p.size, p.subdivisions);
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        Vec3& pos = mesh.vertex(static_cast<int32_t>(i)).position;
        float nv = fbmNoise(pos.x * p.noiseScale, 0, pos.z * p.noiseScale, p.octaves, p.seed);
        pos.y = nv * p.height;
        // Color by height
        float t = (pos.y / p.height) * 0.5f + 0.5f;
        Vec3 low(0.3f, 0.5f, 0.2f);   // green grass
        Vec3 mid(0.5f, 0.45f, 0.3f);  // brown dirt
        Vec3 high(0.6f, 0.6f, 0.55f); // gray rock
        Vec3 col;
        if (t < 0.4f) col = low + (mid - low) * (t / 0.4f);
        else col = mid + (high - mid) * ((t - 0.4f) / 0.6f);
        mesh.vertex(static_cast<int32_t>(i)).color = col;
    }
    mesh.recomputeNormals();
    return mesh;
}

// =====================================================
// PROCEDURAL: CRYSTAL
// =====================================================
HalfEdgeMesh MeshGenerator::generateCrystal(const CrystalParams& p) {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    float h2 = p.height * 0.5f;
    // Top and bottom apex
    pos.push_back({0, h2 + h2 * 0.3f, 0}); // top point
    pos.push_back({0, -h2 * 0.5f, 0}); // bottom point
    // Middle ring
    for (int i = 0; i < p.sides; i++) {
        float a = 2.0f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(p.sides);
        float r = p.radius * (1.0f + (hashFloat(i, p.seed, 0, 42) - 0.5f) * 0.3f);
        pos.push_back({r * std::cos(a), 0, r * std::sin(a)});
    }
    // Connect top and bottom to ring
    for (int i = 0; i < p.sides; i++) {
        uint32_t curr = static_cast<uint32_t>(i + 2);
        uint32_t next = static_cast<uint32_t>((i + 1) % p.sides + 2);
        tris.push_back({0, curr, next}); // top
        tris.push_back({1, next, curr}); // bottom
    }
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    // Crystal colors: purple/blue/cyan
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        float t = hashFloat(static_cast<int>(i), p.seed, 1, 123);
        mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.4f+t*0.3f, 0.3f+t*0.2f, 0.7f+t*0.3f);
    }
    return mesh;
}

// =====================================================
// GAME TEMPLATES
// =====================================================
HalfEdgeMesh MeshGenerator::createGameTree() {
    TreeParams p;
    p.trunkRadius = 0.08f; p.trunkHeight = 0.8f; p.foliageRadius = 0.5f;
    p.trunkSegs = 6; p.foliageRings = 6; p.seed = 42; p.style = 0;
    return generateTree(p);
}

HalfEdgeMesh MeshGenerator::createGameRock() {
    RockParams p;
    p.radius = 0.6f; p.roughness = 0.25f; p.seed = 77; p.segments = 8; p.rings = 5;
    return generateRock(p);
}

HalfEdgeMesh MeshGenerator::createGameHouse() {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    // Box body
    std::vector<Vec3> boxPos;
    std::vector<std::array<uint32_t,3>> boxTris;
    float w = 1.0f, h = 0.8f, d = 1.2f;
    float w2 = w*0.5f, d2 = d*0.5f;
    boxPos = {{-w2,0,-d2},{w2,0,-d2},{w2,0,d2},{-w2,0,d2},{-w2,h,-d2},{w2,h,-d2},{w2,h,d2},{-w2,h,d2}};
    boxTris = {{4,5,6},{4,6,7},{1,0,3},{1,3,2},{0,4,7},{0,7,3},{5,1,2},{5,2,6},{7,6,2},{7,2,3},{0,1,5},{0,5,4}};
    mergeMeshData(pos, tris, boxPos, boxTris);
    // Pyramid roof
    std::vector<Vec3> roofPos = {{-w2-0.1f,h,-d2-0.1f},{w2+0.1f,h,-d2-0.1f},{w2+0.1f,h,d2+0.1f},{-w2-0.1f,h,d2+0.1f},{0,h+0.6f,0}};
    std::vector<std::array<uint32_t,3>> roofTris = {{0,2,1},{0,3,2},{0,1,4},{1,2,4},{2,3,4},{3,0,4}};
    mergeMeshData(pos, tris, roofPos, roofTris);
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    // Colors
    size_t wallVerts = boxPos.size();
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        if (i < wallVerts)
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.85f, 0.80f, 0.70f); // beige walls
        else
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.6f, 0.25f, 0.15f);  // red roof
    }
    return mesh;
}

HalfEdgeMesh MeshGenerator::createGameSword() {
    std::vector<Vec3> pos;
    std::vector<std::array<uint32_t,3>> tris;
    // Blade (thin diamond cross-section)
    float bLen = 1.2f, bW = 0.08f, bH = 0.02f;
    std::vector<Vec3> blade = {{0,0,0},{bW,0,bLen*0.5f},{0,bH,bLen*0.5f},{-bW,0,bLen*0.5f},{0,-bH,bLen*0.5f},{0,0,bLen}};
    std::vector<std::array<uint32_t,3>> bladeTri = {{0,1,2},{0,2,3},{0,3,4},{0,4,1},{5,2,1},{5,3,2},{5,4,3},{5,1,4}};
    mergeMeshData(pos, tris, blade, bladeTri);
    // Guard (cross piece)
    float gW = 0.25f, gH = 0.03f, gD = 0.04f;
    std::vector<Vec3> guard = {
        {-gW,-gH,-gD},{gW,-gH,-gD},{gW,-gH,gD},{-gW,-gH,gD},
        {-gW,gH,-gD},{gW,gH,-gD},{gW,gH,gD},{-gW,gH,gD}
    };
    std::vector<std::array<uint32_t,3>> guardTri = {{4,5,6},{4,6,7},{1,0,3},{1,3,2},{0,4,7},{0,7,3},{5,1,2},{5,2,6},{7,6,2},{7,2,3},{0,1,5},{0,5,4}};
    mergeMeshData(pos, tris, guard, guardTri);
    // Handle (cylinder-ish)
    float hR = 0.025f, hL = 0.3f;
    int hs = 6;
    std::vector<Vec3> hPos;
    std::vector<std::array<uint32_t,3>> hTris;
    hPos.push_back({0,0,0}); hPos.push_back({0,0,-hL});
    for (int i = 0; i < hs; i++) {
        float a = 2.0f*static_cast<float>(M_PI)*static_cast<float>(i)/static_cast<float>(hs);
        hPos.push_back({hR*std::cos(a), hR*std::sin(a), 0});
    }
    uint32_t hts = static_cast<uint32_t>(hPos.size());
    for (int i = 0; i < hs; i++) {
        float a = 2.0f*static_cast<float>(M_PI)*static_cast<float>(i)/static_cast<float>(hs);
        hPos.push_back({hR*std::cos(a), hR*std::sin(a), -hL});
    }
    for (int i = 0; i < hs; i++) {
        uint32_t ni = static_cast<uint32_t>((i+1)%hs);
        hTris.push_back({0, 2+ni, 2+static_cast<uint32_t>(i)});
        hTris.push_back({1, hts+static_cast<uint32_t>(i), hts+ni});
        hTris.push_back({2+static_cast<uint32_t>(i), hts+static_cast<uint32_t>(i), 2+ni});
        hTris.push_back({2+ni, hts+static_cast<uint32_t>(i), hts+ni});
    }
    mergeMeshData(pos, tris, hPos, hTris, Vec3(0, 0, -gD));
    HalfEdgeMesh mesh;
    mesh.buildFromTriangles(pos, tris);
    // Colors
    size_t bladeV = blade.size(), guardV = guard.size();
    for (size_t i = 0; i < mesh.vertexCount(); i++) {
        if (i < bladeV)
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.75f, 0.78f, 0.82f); // steel
        else if (i < bladeV + guardV)
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.55f, 0.45f, 0.2f); // gold guard
        else
            mesh.vertex(static_cast<int32_t>(i)).color = Vec3(0.35f, 0.2f, 0.1f); // brown handle
    }
    return mesh;
}

} // namespace sculpt
