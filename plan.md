# 3D Sculpting Application (C++) - Development Plan

This document serves as the execution plan for an AI coding assistant (e.g., Claude Code) to build a simple, lightweight 3D sculpting program in C++, heavily inspired by Blender's architecture.

## Project Considerations
*   **Language**: C++ (C++17 or C++20).
*   **Target Platforms**: PC (Windows/macOS/Linux) first, designed to be portable to Mobile (Android NDK / iOS) later.
*   **Graphics API**: OpenGL ES 3.0 or Vulkan (to ensure mobile compatibility).
*   **Windowing/Input**: SDL2 or SDL3 (excellent cross-platform support for PC and Mobile).
*   **UI**: Dear ImGui (lightweight, easy to integrate, supports touch screens).
*   **Math**: GLM (OpenGL Mathematics).

## Blender Codebase References
When implementing the features below, you should reference specific areas of the [Blender Repository](https://github.com/blender/blender) to understand industry-standard approaches:
1.  **BMesh Topology**: `source/blender/bmesh/` (Half-edge data structures for dynamic meshes).
2.  **PBVH (Poly Bounding Volume Hierarchy)**: `source/blender/blenkernel/intern/pbvh.cc` (How Blender achieves high-performance raycasting and vertex updates during a brush stroke).
3.  **Sculpt Brushes**: `source/blender/editors/sculpt_paint/` (Logic for standard, smooth, and grab brushes).

---

## 📋 Task Checklist for AI Execution

### Phase 1: Foundational Setup (PC-First)
- [ ] **Initialize CMake Project**: Create a `CMakeLists.txt` configured to output a desktop executable. Set up dependencies (SDL2, GLM, Glad/GLEW for OpenGL, Dear ImGui).
- [ ] **Window & Context Management**: Implement a `Window` class using SDL2 to create a window, initialize an OpenGL context, and handle the main application loop.
- [ ] **Input Handling**: Abstract mouse, keyboard, and generic "pointer" inputs. Map mouse events to generic pointer events so touch inputs can easily hook into this later.
- [ ] **Basic Rendering Pipeline**: Create an `OpenGLRenderer` class. Implement basic Shader compilation (`Shader.h`/`.cpp`) and a `Camera` class (Orbit controls).

### Phase 2: Core Data Structures (The "BMesh" Light)
- [ ] **Mesh Data Structure**: Implement a Half-Edge data structure (`HalfEdgeMesh.cpp`) or a highly optimized indexed vertex array system if dynamic topology isn't required in v1. *For sculpting, connecting vertices to their neighbors efficiently is mandatory (e.g., vertex-to-face adjacencies).*
- [ ] **Spatial Acceleration System**: Implement an Octree or a lightweight BVH (Bounding Volume Hierarchy). Sculpting touches thousands of vertices; without a BVH to find points within a brush radius, the program will lag instantly.
- [ ] **Mesh Import/Export**: Implement a basic `.obj` loader to load a starting sphere or baseline mesh.

### Phase 3: The Sculpting Engine Foundation
- [ ] **Raycasting**: Implement Screen-to-World raycasting. When the user clicks on the screen, calculate the intersection point with the 3D mesh using the BVH.
- [ ] **Symmetry System**: Establish a basic X-axis symmetry option that replicates brush strokes on the opposite side of the mesh.
- [ ] **Brush Data Model**: Create a `Brush` struct containing:
    - `Radius` (in screen space or world space).
    - `Strength` (intensity of the displacement).
    - `Falloff` profile (Smooth, Linear, Constant).

### Phase 4: Brush Implementation
- [ ] **Draw Brush (Standard)**: Displaces vertices along their average normals outwards or inwards.
- [ ] **Smooth Brush**: Re-averages the position of a vertex based on its neighbors (Laplacian smoothing).
- [ ] **Grab/Move Brush**: Calculates screen-space translation and applies it to vertices within the radius, falling off towards the edges.
- [ ] **Normal Recalculation**: After altering vertices, update face normals and vertex normals efficiently (only update the chunks/BVH nodes modified).

### Phase 5: Rendering Upgrades & UI
- [ ] **MatCap Shader**: Implement a Material Capture (MatCap) shader. This is standard in sculpting apps (like ZBrush and Blender) as it highlights surface details without costly lighting computations.
- [ ] **ImGui Interface**: Build the tool menus. Include sliders for Brush Size, Brush Strength, brush selection buttons, and a Wireframe toggle.

### Phase 6: Mobile Port Preparation
- [ ] **Touch Gesture Mapping**: Update the Input Manager map SDL multi-touch events.
    - 1 Finger: Rotate camera OR Sculpt (depending on tool mode).
    - 2 Fingers: Pinch to zoom, drag to pan.
- [ ] **CMake Android/iOS Chains**: Add toolchain definitions in CMake to build an `.apk` (Android) or Xcode project (iOS) reusing the exact same C++ core logic.

### Phase 7: Web Deployment (WebAssembly)
- [ ] **Emscripten Setup**: Set up the Emscripten toolchain (`emcc`) to compile the C++ codebase to WebAssembly (Wasm).
- [ ] **WebGL Translation**: Ensure the OpenGL ES 3.0 calls are correctly mapped to WebGL 2.0 (SDL2 and Emscripten handle this natively).
- [ ] **Web Build Pipeline**: Add a CMake target that outputs `index.html`, `index.js`, and `index.wasm` so the app can be hosted on any static web server (like GitHub Pages or Vercel).

---

## Instructions for Execution
1.  **Systematic Execution**: Go through Phase 1 to Phase 7 in order. Do not skip the BVH (Phase 2), or sculpting performance will fail.
2.  **Modularity**: Keep the platform-specific code (SDL/windowing) completely separated from the core logic (`Mesh`, `BVH`, `SculptEngine`).
3.  **Compile & Test**: Ensure the application compiles and runs successfully after each Phase.
