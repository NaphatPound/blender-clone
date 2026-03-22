# SculptApp Pro — ZBrush-Level Sculpting Ideas

## Current State
We have 3 basic brushes (Draw, Smooth, Grab) with simple falloff. This document outlines the roadmap to reach professional sculpting capability like ZBrush, Blender Sculpt, and Mudbox.

---

## 1. Professional Brush Library (15+ Brushes)

### Currently Have: Draw, Smooth, Grab

### Essential Brushes to Add:

| Brush | What it Does | How it Works |
|-------|-------------|--------------|
| **Clay** | Adds material in flat strips (like real clay) | Displace toward a plane above the surface, not along normals |
| **Clay Strips** | Directional clay with stroke direction | Clay brush oriented along mouse drag direction |
| **Flatten** | Flattens bumps to an average plane | Find average height in radius, push vertices toward it |
| **Crease** | Creates sharp valleys/ridges | Push vertices inward along normals + pinch neighbors together |
| **Pinch** | Pulls vertices toward the brush center line | Move vertices toward the stroke center axis |
| **Inflate** | Expands vertices along their normals uniformly | Like Draw but affects all directions equally (balloon effect) |
| **Layer** | Adds a uniform height layer (won't go above limit) | Draw with a height cap — stops displacing once it reaches a threshold |
| **Nudge** | Slides surface sideways | Translate vertices in screen-space direction of mouse drag |
| **Snake Hook** | Pulls geometry in long strands | Like Grab but affects a trail of vertices along the stroke |
| **Thumb** | Smears surface like pushing clay | Averages vertex positions along drag direction with momentum |
| **Elastic Deform** | Soft body-like deformation | Use Kelvinlet (elastic brush) math for physically-based deformation |
| **Pose** | Rotates a limb from a joint | Detect limb axis, rotate vertices on one side |
| **Mask** | Paint a mask to protect areas | Store per-vertex mask (0-1), multiply all brush effects by (1-mask) |
| **Trim** | Cuts flat along a plane | Boolean-like flatten that removes geometry above a cutting plane |
| **Scrape** | Removes peaks only (like sandpaper) | Flatten but only affects vertices above the average plane |

### Brush System Enhancements:
- **Custom falloff curves** — ImGui spline editor, save/load presets
- **Brush texture/alpha** — Apply a 2D texture pattern to the brush tip (stamp mode)
- **Lazy mouse** — Smooth out shaky strokes by trailing behind the cursor
- **Accumulate mode** — Keep adding displacement on same spot vs. height limit
- **Front faces only** — Only affect vertices whose normals face the camera
- **Autosmooth** — Automatic smoothing pass after each brush stroke

---

## 2. Dynamic Topology (DynTopo)

**The #1 feature that separates pro sculpting from amateur.**

### What it Does:
While you sculpt, the mesh automatically **adds more triangles** where you need detail and **removes them** where you don't. Like infinite clay — you never run out of resolution.

### How to Implement:

#### A. Subdivide Under Brush
When a brush stroke hits triangles that are **too large** (edge length > threshold):
1. Split long edges at their midpoint
2. Retriangulate the affected faces
3. Interpolate new vertex positions/normals from neighbors
4. Update the BVH for the affected region

#### B. Collapse Under Brush
When triangles are **too small** (edge length < threshold):
1. Collapse the shortest edge (merge two vertices)
2. Remove degenerate triangles
3. Clean up topology

#### C. Detail Flood Fill
One-click operation that subdivides the entire mesh to a target edge length.

#### D. Parameters:
- **Detail size** — Target edge length (slider)
- **Detail mode**: Relative (based on brush size) vs Constant (fixed world-space)
- **Subdivide only** / **Collapse only** / **Both**

### Architecture:
```
class DynamicTopo {
    float detailSize = 0.05f;
    bool subdivide = true;
    bool collapse = true;

    void processStroke(HalfEdgeMesh& mesh, const Vec3& center, float radius);
    void splitLongEdges(HalfEdgeMesh& mesh, float maxEdgeLen, const Vec3& center, float radius);
    void collapseShortEdges(HalfEdgeMesh& mesh, float minEdgeLen, const Vec3& center, float radius);
    void floodFill(HalfEdgeMesh& mesh, float targetEdgeLen);
};
```

---

## 3. Remeshing (ZRemesher-like)

### What it Does:
Takes any messy sculpted mesh and generates a **clean, uniform quad mesh** while preserving the shape. Essential for game-ready models.

### Types of Remesh:

#### A. Voxel Remesh (Simple, Fast)
1. Voxelize the mesh at a target resolution
2. Run Marching Cubes to extract an isosurface
3. Result: uniform triangle mesh, no topology from original

**Parameters:** Voxel size, Smooth iterations

#### B. Quad Remesh (Advanced)
1. Compute a cross-field on the surface (direction field for quad alignment)
2. Trace quad strips along the field
3. Result: clean quad topology following surface flow

**This is very complex — start with Voxel Remesh.**

### Instant Meshes Algorithm (Simpler Alternative):
1. Compute target vertex positions via Poisson disk sampling
2. Connect neighbors via Delaunay triangulation on the surface
3. Relax to improve quality

---

## 4. Multi-Resolution Sculpting

### What it Does:
Work at different **detail levels** on the same mesh. Low-level for big shapes, high-level for fine wrinkles.

### How it Works:
1. Start with a base mesh (low poly)
2. **Subdivide** (Loop subdivision or Catmull-Clark) to add resolution
3. Store **displacement offsets** at each level
4. Switch between levels freely — edits propagate up and down
5. Export any level or bake displacements as a normal/displacement map

### Subdivision Schemes:
- **Loop Subdivision** (for triangle meshes): Split each triangle into 4, smooth positions
- **Catmull-Clark** (for quad meshes): Split each face into N quads, smooth positions

### Architecture:
```
class MultiResMesh {
    std::vector<HalfEdgeMesh> levels;    // level 0 = base, higher = more detail
    std::vector<std::vector<Vec3>> displacements; // per-level offsets
    int currentLevel = 0;

    void subdivide();      // add a level
    void setLevel(int lv); // switch working level
    void propagateUp();    // apply changes to higher levels
    void propagateDown();  // apply changes to lower levels
};
```

---

## 5. Sculpt Masking System

### What it Does:
Paint a **mask** on the mesh to **protect areas** from brush strokes. Like masking tape for 3D sculpting.

### Features:
- **Mask Brush** — Paint mask intensity (0 = unmasked, 1 = fully protected)
- **Invert Mask** — Swap masked/unmasked areas (Ctrl+I)
- **Clear Mask** — Remove all masking (Alt+M)
- **Mask by Cavity** — Auto-mask based on surface concavity
- **Mask by Normal** — Mask based on face direction (e.g., mask top-facing)
- **Sharpen Mask** — Make mask edges crisp
- **Grow/Shrink Mask** — Expand or contract masked region

### Implementation:
Add `float mask = 0.0f;` to the Vertex struct. In every brush operation, multiply the effect by `(1.0f - vertex.mask)`.

### Visualization:
Darken masked vertices in the shader: `color *= (1.0 - mask * 0.6)` — masked areas appear darker.

---

## 6. Undo/Redo System

### Architecture:
```
struct MeshSnapshot {
    std::vector<Vec3> positions;
    std::vector<Vec3> normals;
    std::vector<Vec3> colors;
    // For topology changes, store full mesh data
    bool topologyChanged = false;
    std::vector<std::array<uint32_t,3>> triangles; // only if topology changed
};

class UndoStack {
    std::vector<MeshSnapshot> undoStack;
    std::vector<MeshSnapshot> redoStack;
    int maxDepth = 50;

    void pushState(const HalfEdgeMesh& mesh, bool topologyChanged = false);
    bool undo(HalfEdgeMesh& mesh);
    bool redo(HalfEdgeMesh& mesh);
};
```

### Optimization:
- For sculpt strokes (no topology change): only store changed vertex positions (delta)
- For topology changes: store full mesh state
- Compress with RLE for unchanged regions

---

## 7. Symmetry System (Full Implementation)

### Current: `symmetryX` flag exists but is NOT implemented in SculptEngine

### Full Implementation:
- **X/Y/Z axis symmetry** — Toggle per axis
- **Radial symmetry** — Repeat brush N times around an axis (for flowers, gears)
- **Mirror plane visualization** — Semi-transparent plane showing symmetry axis
- **Topology symmetry** — Use mesh topology to find mirrored vertices (more accurate than spatial mirror)

### How to Implement:
In `SculptEngine::stroke()`:
1. Apply brush at the original position
2. If symmetryX: mirror the position across X axis, apply brush again
3. Repeat for Y and Z if enabled
4. For radial: rotate position N times around the axis

---

## 8. Performance Optimizations for High-Poly Sculpting

### Current Bottlenecks:
- `recomputeNormals()` updates ALL vertices after every stroke
- BVH rebuilt from scratch after topology changes
- Single-threaded brush application

### Solutions:

#### A. Partial Normal Update
Only recompute normals for affected vertices + their 1-ring neighbors:
```
void recomputeNormalsPartial(const std::vector<int32_t>& affectedVerts);
```

#### B. BVH Partial Update
Instead of full rebuild, only update nodes containing modified faces:
```
void updateRegion(const AABB& affectedRegion, const HalfEdgeMesh& mesh);
```

#### C. Multithreaded Brush Application
Use std::thread or OpenMP to parallelize vertex displacement:
```
#pragma omp parallel for
for (int i = 0; i < affectedVerts.size(); i++) {
    applyDisplacement(affectedVerts[i]);
}
```

#### D. GPU-Accelerated Sculpting
Move brush application to a compute shader:
1. Upload affected vertex positions to GPU
2. Run compute shader with brush parameters
3. Read back displaced positions
This enables sculpting on 10M+ poly meshes.

#### E. Mesh Streaming / LOD
For very high-poly meshes:
- Only render visible faces
- Use lower LOD for distant parts
- Stream detail in/out as camera moves

---

## 9. Surface Detail Tools

### Alpha/Stencil Brushes
Apply 2D textures as height displacement:
- Load PNG/JPG as brush alpha
- Project onto surface during stroke
- Use cases: skin pores, scales, fabric patterns, rock details

### Surface Noise
Apply procedural noise directly to surface:
- Perlin noise at configurable scale/amplitude
- Voronoi noise for organic patterns
- Parameters: frequency, amplitude, octaves, seed

### Extract/Project Detail
- **Extract**: Create a separate mesh from a selected region (like ZBrush Extract)
- **Project**: Project detail from a high-poly mesh onto a low-poly one

---

## 10. Advanced Camera & Navigation

### Focal Point Sculpting
- Double-click to set focus point (center of rotation)
- Automatically adjusts near/far planes for precision
- Zoom-to-selection: frame selected vertices

### Turntable vs Trackball
- **Turntable** (current): Y-axis fixed, good for characters
- **Trackball**: Free rotation, good for organic forms
- Toggle in preferences

### Camera Bookmarks
- Save camera positions (numpad 1-9)
- Quick snap to front/side/top views
- Smooth animated transitions between views

---

## 11. Material/Shader Sculpt Preview

### Cavity Shading
Darken concave areas, brighten convex — emphasizes surface detail:
```glsl
float cavity = 1.0 - smoothstep(0.0, 0.3, length(dFdx(vViewNormal) + dFdy(vViewNormal)));
color *= mix(0.6, 1.2, cavity);
```

### Ambient Occlusion (Screen-Space)
SSAO post-process for realistic depth perception:
1. Sample depth buffer around each pixel
2. Darken occluded areas
3. Massive improvement for evaluating sculpt quality

### Multiple MatCap Materials
Offer 6-8 matcap presets:
- Clay (current), Chrome, Skin, Jade, Wax, Red Clay, Dark Metal, Ghost

---

## 12. Sculpt Layers

### What it Does:
Separate sculpt modifications into layers (like Photoshop layers):
- Layer 1: Base shape
- Layer 2: Muscle definition
- Layer 3: Skin wrinkles
- Layer 4: Pores/fine detail

### Operations:
- Toggle layer visibility on/off
- Adjust layer opacity (strength)
- Merge layers
- Delete layer (removes its modifications)

### Architecture:
Each layer stores vertex position offsets relative to the base mesh:
```
struct SculptLayer {
    std::string name;
    float opacity = 1.0f;
    bool visible = true;
    std::vector<Vec3> offsets; // per-vertex displacement from base
};
```

---

## Priority Roadmap

### Phase 1 — Essential Pro Features
1. **Undo/Redo** — Can't sculpt without this
2. **6 New Brushes** — Clay, Flatten, Crease, Pinch, Inflate, Scrape
3. **Masking** — Protect areas during sculpting
4. **Symmetry** — Mirror brush strokes in real-time
5. **Partial Normal Update** — Performance fix for large meshes

### Phase 2 — Detail & Resolution
6. **Dynamic Topology** — Automatic mesh subdivision under brush
7. **Voxel Remesh** — Clean mesh regeneration
8. **Alpha Brushes** — Texture-based detail stamping
9. **Cavity Shading** — Better visual feedback
10. **Multi-Resolution** — Subdivision levels

### Phase 3 — Production Quality
11. **Sculpt Layers** — Non-destructive workflow
12. **GPU Sculpting** — Handle 10M+ poly meshes
13. **Quad Remesh** — Game-ready topology
14. **Surface Noise** — Procedural detail
15. **SSAO** — Screen-space ambient occlusion

### Phase 4 — Advanced
16. **Kelvinlet Brushes** — Physically-based elastic deformation
17. **Mesh Extract** — Create new meshes from selections
18. **Radial Symmetry** — For mechanical/organic patterns
19. **Brush Texture** — Custom brush tip shapes
20. **Stroke Stabilization** — Lazy mouse / smoothing
