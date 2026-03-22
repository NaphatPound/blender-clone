#pragma once

#include "core/HalfEdgeMesh.h"

namespace sculpt {

class MeshGenerator {
public:
    // Basic primitives
    static HalfEdgeMesh createSphere(float radius, int segments, int rings);
    static HalfEdgeMesh createCube(float size);
    static HalfEdgeMesh createPlane(float size, int subdivisions);
    static HalfEdgeMesh createCylinder(float radius, float height, int segments);
    static HalfEdgeMesh createCone(float radius, float height, int segments);
    static HalfEdgeMesh createTorus(float majorRadius, float minorRadius, int majorSegs, int minorSegs);
    static HalfEdgeMesh createCapsule(float radius, float height, int segments, int rings);
    static HalfEdgeMesh createWedge(float width, float height, float depth);
    static HalfEdgeMesh createPyramid(float base, float height);

    // Procedural generators
    struct RockParams {
        float radius = 0.8f; float roughness = 0.3f; int seed = 42; int segments = 12; int rings = 6;
    };
    static HalfEdgeMesh generateRock(const RockParams& p);

    struct TreeParams {
        float trunkRadius = 0.1f; float trunkHeight = 1.0f; float foliageRadius = 0.6f;
        int trunkSegs = 8; int foliageRings = 6; int seed = 42; int style = 0; // 0=round, 1=cone, 2=flat
    };
    static HalfEdgeMesh generateTree(const TreeParams& p);

    struct TerrainParams {
        float size = 10.0f; int subdivisions = 20; float height = 1.5f;
        float noiseScale = 0.3f; int seed = 42; int octaves = 3;
    };
    static HalfEdgeMesh generateTerrain(const TerrainParams& p);

    struct CrystalParams {
        float radius = 0.3f; float height = 1.2f; int sides = 6; float taper = 0.0f; int seed = 42;
    };
    static HalfEdgeMesh generateCrystal(const CrystalParams& p);

    // Game templates (preset combinations)
    static HalfEdgeMesh createGameTree();
    static HalfEdgeMesh createGameRock();
    static HalfEdgeMesh createGameHouse();
    static HalfEdgeMesh createGameSword();
};

} // namespace sculpt
