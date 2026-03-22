# SculptApp Development Log

## Build Date: 2026-03-22

---

## Phase 1: Foundational Setup - COMPLETE

### Files Created
- `CMakeLists.txt` - Build configuration (CMake + manual g++ fallback)
- `include/core/HalfEdgeMesh.h` - Vec3, AABB, Vertex, HalfEdge, Face, HalfEdgeMesh
- `include/core/BVH.h` - Bounding Volume Hierarchy for spatial queries
- `include/core/ObjLoader.h` - OBJ file import/export
- `include/core/MeshGenerator.h` - Procedural mesh generation (sphere, cube, plane)
- `include/render/Camera.h` - Orbit camera with view/projection matrices
- `include/render/Shader.h` - OpenGL shader compilation
- `include/render/Renderer.h` - OpenGL renderer with MatCap shader
- `include/input/InputManager.h` - Abstracted pointer/scroll input
- `include/sculpt/Brush.h` - Brush settings and application (Draw, Smooth, Grab)
- `include/sculpt/SculptEngine.h` - Main sculpt engine coordinating mesh + BVH + brushes
- `include/sculpt/Raycaster.h` - Screen-to-world ray conversion, ray-primitive tests
- `src/` - All corresponding .cpp implementations
- `src/main.cpp` - SDL2 + ImGui application with mode system

## Phase 2: Core Data Structures - COMPLETE

### HalfEdgeMesh
- Half-edge topology with vertex-face adjacency
- Efficient neighbor queries via half-edge traversal
- Normal recomputation (face + vertex normals)
- Triangle data export for rendering
- **Mutation methods**: addVertex, addFace, removeFace, splitEdge, compact, relinkTwins

### BVH (Bounding Volume Hierarchy)
- SAH-like median split construction
- Ray-triangle intersection (Möller-Trumbore algorithm)
- Sphere query for brush vertex selection
- Rebuild support for dynamic mesh updates

### Mesh Generators
- UV Sphere (configurable segments/rings)
- Cube (6 faces, 12 triangles)
- Subdivided Plane

### OBJ Loader
- Import with fan triangulation for quads/n-gons
- Handles v//vn and v/vt/vn formats
- Export with vertex normals

## Phase 3 & 4: Sculpting Engine - COMPLETE

### Brushes Implemented
- **Draw Brush**: Displaces vertices along their normals (in/out with invert toggle)
- **Smooth Brush**: Laplacian smoothing using vertex neighbors
- **Grab Brush**: Translates vertices with falloff

### Falloff Types
- Smooth (Hermite interpolation), Linear, Constant

### Raycasting
- Screen-to-world ray unprojection
- Ray-sphere and ray-AABB intersection tests
- BVH-accelerated mesh raycasting

## Phase 5: Rendering & UI - COMPLETE

### OpenGL Rendering
- MatCap shader (clay material with rim lighting and specular)
- SDL2 window with OpenGL 3.3 Core Profile
- HiDPI/Retina support
- Wireframe toggle
- MSAA 4x antialiasing

### Dear ImGui Interface
- Sculpt Tools panel (brush selection, radius/strength sliders, falloff curve)
- Edit Tools panel (selection mode, tool buttons with parameter sliders)
- Mode tab bar (Sculpt / Edit mode switching)
- Mesh panel (shape selection, reset, OBJ export)
- Stats overlay (vertex/face count, FPS, current mode)
- Controls help overlay (context-sensitive per mode)

## Phase NEW: Edit Mode System - COMPLETE

### Mode Manager
- `include/core/ModeManager.h` - TAB key toggles between Sculpt and Edit modes
- Mode-dependent input routing (sculpting vs selection)
- Mode-dependent UI panels

### Edit Selection System
- `include/edit/EditSelection.h` - Vertex/Edge/Face selection modes
- Click-to-select with BVH raycasting
- Shift+Click to add to selection
- A key to select all / deselect all
- Nearest vertex/edge/face picking from raycast hit

### Edit Tools
- `include/edit/EditTools.h` + `src/edit/EditTools.cpp`
- **Extrude (E)**: Extrudes selected faces along average normal with configurable offset. Duplicates boundary vertices, creates side wall triangles.
- **Inset Face (I)**: Creates inner face + 6 side triangles per selected face. Configurable inset amount (0-1).
- **Loop Cut (Ctrl+R)**: Walks edge loop across triangle strip, inserts midpoint vertices, splits affected faces.
- **Bevel (Ctrl+B)**: Offsets edge vertices perpendicular to edge direction, creates bevel face strip.

### Selection Renderer
- `include/edit/SelectionRenderer.h` - OpenGL overlay rendering
- Selected vertices as GL_POINTS (white)
- Selected edges as GL_LINES (orange, 3px width)
- Selected faces as semi-transparent GL_TRIANGLES (orange, alpha 0.35)
- Mesh wireframe overlay in edit mode (faint gray)
- Depth-biased rendering to sit on top of mesh surface

---

## Tests: 140/140 PASSED

| Test Suite      | Tests | Status |
|-----------------|-------|--------|
| Mesh            | 33    | PASS   |
| BVH             | 16    | PASS   |
| Sculpt          | 18    | PASS   |
| OBJ Loader      | 15    | PASS   |
| Camera          | 16    | PASS   |
| Raycaster       | 12    | PASS   |
| Edit Tools      | 30    | PASS   |

---

## Bugs Found & Fixed

### BUG-001: Sphere normals pointing inward
- **Found by**: test_sculpt > testInvertBrush
- **Symptom**: Inverted brush pushed vertices outward instead of inward.
- **Root Cause**: Triangle winding order in `MeshGenerator::createSphere()` was CW from outside.
- **Fix**: Reversed winding order in top cap, middle quads, and bottom cap.
- **File**: `src/core/MeshGenerator.cpp`

### BUG-002: Infinite loop in recomputeNormals after edit operations
- **Found by**: test_edit_tools > testInsetFace (hung during recomputeNormals)
- **Symptom**: After removeFace + addFace, `getVertexFaces()` entered infinite loop walking dead half-edges.
- **Root Cause**: `removeFace()` marks half-edges with `face=-1` but leaves `prev/next` pointers intact. `getVertexFaces()` could start walking from a dead half-edge and loop forever.
- **Fix**:
  1. Added safety iteration limits in `getVertexNeighbors` and `getVertexFaces`
  2. Added fallback in `getVertexFaces` to find a live half-edge if the vertex's stored one is dead
  3. Changed all edit tools to call `compact()` before `recomputeNormals()` to clean up dead topology first
- **Files**: `src/core/HalfEdgeMesh.cpp`, `src/edit/EditTools.cpp`
- **Verified**: All 140 tests pass after fix

---

## Application Output
```
SculptApp v0.1.0
OpenGL: 4.1 Metal - 89.4
Renderer: Apple M3 Pro
Mesh: 1986 verts, 3968 faces

API IS WORKING
```

### BUG-003: Extrude crash (SIGSEGV in faceVertices)
- **Found by**: User crash report — `EXC_BAD_ACCESS` in `faceVertices()` called from `extrude()`
- **Root Cause**: `extrude()` deleted faces with `removeFace()` then called `faceVertices()` on deleted indices (halfEdge=-1 → access m_halfEdges[-1])
- **Fix**: Save face vertex data BEFORE deletion, use saved data for reconstruction
- **File**: `src/edit/EditTools.cpp`

### BUG-004: Raycast position mismatch (cursor offset)
- **Root Cause**: `screenToWorldRay()` incorrectly transposed view matrix rotation — read rows as columns from column-major storage
- **Fix**: Changed to read columns directly: `viewMatrix[0]*v.x + viewMatrix[1]*v.y + viewMatrix[2]*v.z`
- **File**: `src/sculpt/Raycaster.cpp`

### BUG-005: Sphere looked bumpy/sculpted on start
- **Root Cause**: `transpose(inverse(mat3(mv)))` in vertex shader produced numerical artifacts on Apple M3 Metal/OpenGL
- **Fix**: Replaced with `mat3(mv)` — equivalent for uniform-scale transforms, numerically stable
- **File**: `src/render/Renderer.cpp`

### BUG-006: Paint mode didn't paint on click
- **Root Cause**: MOUSEBUTTONDOWN only set `painting=true` but didn't paint; painting only happened on MOUSEMOTION (drag)
- **Fix**: Added immediate paint stroke on initial click + paint on drag

### BUG-007: Audit batch (8 fixes)
1. SelectionRenderer VAO not bound before glBufferData — wireframe/selection didn't render
2. SelectionRenderer shader uniform vec3/vec4 mismatch — dead code removed
3. EditSelection `getAffectedVertices()` — added bounds checks for deleted faces
4. PaintEngine — added bounds checks on vertex indices from BVH queries
5. LowpolyTools `mirror()` — skip faces with invalid (-1) vertex indices
6. `compact()` now removes orphaned vertices (was keeping all, causing memory bloat)
7. `decimate()` — fixed live face counting (was including deleted faces in count)
8. Navigation gizmo — added `IsWindowHovered()` check to prevent accidental camera rotation when clicking other UI
9. Mode switching — reset sculpting/painting flags, restore shading mode on mode change

---

## Remaining Work (Future Phases)
- **Phase 6**: Touch gesture mapping, Android/iOS CMake toolchains
- **Symmetry System**: X-axis brush mirroring (data model ready, application logic pending)
- **Undo System**: Mesh state snapshots before each operation
- **Interactive Drag**: Mouse-drag to set extrude distance / inset amount in real-time
