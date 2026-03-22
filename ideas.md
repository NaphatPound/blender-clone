# SculptApp Ideas — Lowpoly Game Asset Creation Tool

## Vision

Transform SculptApp into a **focused, beginner-friendly tool for creating lowpoly 3D game assets** — the kind used in indie games, mobile games, and stylized art. Not a Blender replacement, but a fast, simple tool where anyone can go from idea to game-ready model in minutes.

**Target users:** Indie game devs, hobbyists, students, game jam participants, mobile game creators.

---

## What We Already Have

| Feature | Status |
|---------|--------|
| Sculpt Mode (Draw, Smooth, Grab brushes) | Done |
| Edit Mode (Extrude, Inset, Loop Cut, Bevel) | Done |
| Vertex/Edge/Face selection | Done |
| BVH raycasting | Done |
| OBJ import/export | Done |
| Mesh generators (Sphere, Cube, Plane) | Done |
| MatCap rendering | Done |
| Navigation Gizmo | Done |
| ImGui tool panels | Done |
| 140 unit tests passing | Done |

---

## Core Ideas for the Lowpoly Tool

### 1. Smart Lowpoly Workflow

**Problem:** Blender is powerful but overwhelming for making simple game models.

**Idea: Opinionated lowpoly pipeline**
- Start from primitive shapes (cube, sphere, cylinder, cone, torus, capsule)
- Edit mode tools designed specifically for lowpoly modeling:
  - **Snap to Grid** — Vertices snap to a configurable 3D grid for clean, geometric shapes
  - **Auto-Flat Shading** — One click to make everything flat-shaded (the lowpoly look)
  - **Edge Crease** — Mark edges as sharp for flat-shaded normals
  - **Mirror Modifier** — Model half, mirror automatically (essential for characters/vehicles)
  - **Decimate Tool** — Reduce polygon count while preserving shape (for optimization)
- Lowpoly doesn't need subdivision surfaces — keep the tool simple

### 2. Vertex Color Painting

**Problem:** Lowpoly games often use vertex colors instead of textures — no UV mapping needed.

**Idea: Built-in vertex color painter**
- Add a **Paint Mode** (third mode alongside Sculpt and Edit)
- Color palette with common lowpoly palettes (pastel, earth tones, neon, retro)
- Paint per-vertex colors with a brush
- Fill entire faces with one click
- Color picker / eyedropper tool
- Import/export vertex colors in OBJ or glTF
- **Palette presets** inspired by popular lowpoly games (Crossy Road, Poly Bridge, Monument Valley)
- Preview with flat shading + vertex colors = instant lowpoly game look

### 3. Starter Shape Library

**Problem:** Starting from a cube every time is slow. Lowpoly games reuse common shapes.

**Idea: Built-in shape/template library**
- **Basic Primitives:** Cube, Sphere, Cylinder, Cone, Torus, Capsule, Wedge, Pyramid
- **Game Templates:**
  - Tree (trunk + foliage cone/sphere)
  - Rock (deformed sphere)
  - House (cube + pyramid roof)
  - Character base (humanoid blockout)
  - Sword / Weapon base
  - Vehicle base (car, boat, plane)
  - Terrain chunk (subdivided plane with noise)
- **Community templates** — users can save and share templates
- One-click insert from a visual grid panel

### 4. Procedural Generation Tools

**Problem:** Making 50 slightly different rocks or trees by hand is tedious.

**Idea: Simple procedural generators**
- **Rock Generator** — Sphere + random vertex displacement + optional smoothing
  - Parameters: size, roughness, seed, polygon count
- **Tree Generator** — Trunk cylinder + branch recursion + leaf clusters
  - Parameters: height, branching, leaf density, style (pine, oak, palm)
- **Terrain Generator** — Plane + Perlin noise displacement
  - Parameters: size, height, roughness, plateaus
- **Crystal Generator** — Elongated octahedron with random facets
- **Cloud Generator** — Merged spheres with flat bottom
- Each generator outputs editable mesh — generate, then hand-tweak

### 5. Game-Ready Export Pipeline

**Problem:** Exporting from modeling tools to game engines requires manual steps.

**Idea: One-click export for game engines**
- **Export formats:**
  - `.obj` (already done)
  - `.glTF 2.0` / `.glb` (the modern standard — Unity, Godot, Three.js)
  - `.fbx` (Unreal Engine)
- **Auto-optimize on export:**
  - Remove internal faces
  - Merge duplicate vertices
  - Generate LOD levels (Level of Detail)
  - Calculate tangent space for normal maps
- **Export presets:**
  - "Unity Lowpoly" — Y-up, 1 unit = 1 meter, .fbx
  - "Godot Lowpoly" — Y-up, .glTF
  - "Three.js Web" — .glb with Draco compression
  - "Unreal Engine" — Z-up, .fbx
- **Batch export** — Export all objects in scene at once

### 6. Simple Scene/Multi-Object Support

**Problem:** Currently we edit one mesh. Games need multiple objects.

**Idea: Basic scene management**
- **Object list panel** — Add/remove/rename objects
- **Transform gizmo** — Move, rotate, scale objects with handles
- **Duplicate** — Copy object with Shift+D
- **Object hierarchy** — Parent objects for grouping
- **Scene grid** — Visible ground plane with grid lines
- **Snap to grid** — Hold Ctrl to snap transforms
- Keep it simple — not a full scene graph, just a list of meshes with transforms

### 7. Undo/Redo System

**Problem:** One wrong edit and you lose work. Critical for any creative tool.

**Idea: Mesh state snapshot stack**
- Snapshot mesh before every tool operation
- Ctrl+Z = undo, Ctrl+Shift+Z = redo
- Stack depth: 50 operations
- Memory efficient: only store changed vertices (delta compression)
- Visual indicator showing undo depth

### 8. Material & Shader Preview

**Problem:** Vertex colors look different with different lighting. Need to preview the final look.

**Idea: Multiple preview modes**
- **MatCap** (current) — For sculpting, shows surface detail
- **Flat Shaded** — The classic lowpoly look, no smoothing
- **Vertex Color** — Show painted vertex colors
- **Wireframe Overlay** — Wireframe on top of shaded view
- **Game Preview** — Simple directional light + ambient, like a typical game engine
- **Silhouette** — Black shape on white, check readability at distance
- **Normal Map Preview** — View baked normals
- Quick-switch buttons or number keys (Numpad 1-6)

### 9. Symmetry & Mirror Tools

**Problem:** Characters, vehicles, and many objects are symmetric. Modeling both sides doubles the work.

**Idea: Full symmetry system**
- **X/Y/Z Mirror** — Toggle per-axis mirroring
- **Mirror Modifier** — Non-destructive mirroring (like Blender)
  - Edit one half, see both
  - Apply modifier to make it real geometry
- **Symmetrize** — Make one side match the other
- Visual mirror plane indicator
- Works in both Sculpt and Edit modes

### 10. Texture Baking (Advanced)

**Problem:** Some game engines need UV-mapped textures, not vertex colors.

**Idea: Simple auto-UV + bake pipeline**
- **Auto UV Unwrap** — Smart projection, box projection, or simple unwrap
- **Bake Vertex Colors to Texture** — Convert vertex colors to a UV texture
- **Bake Normals** — High-poly to low-poly normal map baking
- **Texture size presets:** 64x64, 128x128, 256x256 (lowpoly doesn't need big textures)
- Preview UV layout in a 2D panel

### 11. Animation Support (Future)

**Problem:** Lowpoly characters need simple animations.

**Idea: Basic skeletal animation**
- **Bone/Armature creation** — Simple bone chain tool
- **Weight painting** — Assign vertex weights to bones
- **Pose mode** — Move bones to create keyframes
- **Simple animations:** Idle, walk, run, jump, attack
- **Export animations** — Embedded in glTF/FBX
- Start with just FK (Forward Kinematics) — keep it simple

### 12. Reference Image System

**Problem:** Artists need reference images while modeling.

**Idea: Floating reference images**
- **Import reference image** — PNG/JPG displayed as a plane in the viewport
- **Front/Side/Top reference** — Snap images to orthographic views
- **Opacity control** — Make references semi-transparent
- **Lock in place** — Prevent accidental selection/movement
- Perfect for character modeling from concept art

### 13. Keyboard-Driven Quick Actions

**Problem:** Clicking through menus is slow during creative flow.

**Idea: Blender-style quick access**
- **Spacebar search** — Type any command name to execute it
- **Quick favorites** — Customizable toolbar of frequently used tools
- **Pie menus** — Radial menus on key hold (e.g., hold Z for shading modes)
- **Tool shortcuts overlay** — Semi-transparent reminder of shortcuts

### 14. Mobile/Tablet Mode

**Problem:** Many indie devs want to sketch 3D ideas on iPad/tablet.

**Idea: Touch-optimized interface**
- **1 finger** — Sculpt/select
- **2 fingers** — Orbit camera (rotate)
- **Pinch** — Zoom
- **3 fingers** — Pan
- **Larger touch targets** for buttons
- **Apple Pencil / Stylus support** — Pressure sensitivity for brush strength
- **Floating tool wheel** — Touch-friendly radial tool selector
- SDL2 already supports touch — architecture is ready

### 15. Plugin / Scripting System (Long-term)

**Problem:** Every user has different needs. Can't build everything.

**Idea: Lua scripting for custom tools**
- Expose mesh API to Lua scripts
- Users can write custom generators, tools, exporters
- Script editor panel in the app
- Share scripts in a community repository
- Example scripts: spiral staircase generator, fence builder, terrain scatterer

---

## Prioritized Roadmap

### Phase A — Essential (Makes the tool useful)
1. Undo/Redo system
2. More primitive shapes (Cylinder, Cone, Torus, Capsule)
3. Flat shading mode
4. Vertex color painting
5. Mirror modifier / X-symmetry
6. Grid snapping

### Phase B — Game-Ready (Makes models exportable)
7. glTF/glb export
8. Multi-object scene support
9. Transform gizmo (Move/Rotate/Scale)
10. Simple scene hierarchy
11. Auto UV unwrap
12. Export presets (Unity, Godot, Unreal)

### Phase C — Productivity (Makes users faster)
13. Shape/template library
14. Procedural generators (rocks, trees, terrain)
15. Reference image support
16. Spacebar command search
17. Pie menus
18. Keyboard shortcut customization

### Phase D — Polish & Advanced
19. Texture baking
20. Basic skeletal animation
21. LOD generation
22. Mobile/tablet optimization
23. Plugin/scripting system
24. Community template sharing

---

## Competitive Advantage

| Tool | Strength | Weakness |
|------|----------|----------|
| Blender | Everything | Too complex for beginners |
| Blockbench | Great for Minecraft-style | Limited to voxel/cube modeling |
| MagicaVoxel | Beautiful voxel art | No polygon modeling |
| Asset Forge | Fast kitbashing | No custom modeling |
| Picocad | Charming, simple | Very limited tools |
| **SculptApp** | **Sculpt + Edit + Lowpoly focused** | **New, needs more features** |

**Our niche:** The gap between "too simple" (Picocad) and "too complex" (Blender) for lowpoly game assets. A tool that's powerful enough for real game models but simple enough to learn in an hour.

---

## Design Principles

1. **Less is more** — Don't add features nobody uses. Every button should serve lowpoly game creation.
2. **Fast to learn** — A new user should make their first model in under 10 minutes.
3. **Game-first** — Every feature should answer: "Does this help ship a game?"
4. **Visual feedback** — Show results immediately. No hidden settings, no confusing dialogs.
5. **Export-ready** — The model you see is the model you get in your game engine.
6. **Cross-platform** — Works on Windows, Mac, Linux. Future: mobile/tablet.
7. **Offline-first** — No accounts, no cloud, no subscriptions. It's your tool, on your machine.

---

## Name Ideas

- **PolyForge** — Forge your lowpoly worlds
- **MeshCraft** — Craft meshes for games
- **LowPolyLab** — Laboratory for lowpoly creations
- **ShapeMaker** — Simple, descriptive
- **QuickMesh** — Speed-focused naming
- **TinyForge** — Small tool, big results
- **VertexKit** — Toolkit for vertex-level modeling
- **GameShape** — Shapes for games
