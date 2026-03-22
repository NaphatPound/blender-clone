#include "core/HalfEdgeMesh.h"
#include "core/MeshGenerator.h"
#include "core/ObjLoader.h"
#include "core/BVH.h"
#include "core/ModeManager.h"
#include "sculpt/SculptEngine.h"
#include "sculpt/Raycaster.h"
#include "render/Camera.h"
#include "render/Renderer.h"
#include "input/InputManager.h"
#include "edit/EditEngine.h"
#include "edit/SelectionRenderer.h"
#include "paint/PaintEngine.h"
#include "tools/LowpolyTools.h"
#include "export/GltfExporter.h"
#include "anim/Armature.h"
#include "anim/AutoRig.h"
#include "anim/BoneRenderer.h"
#include "anim/Animation.h"

#include "SDL.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <string>
#include <cmath>

static const int WINDOW_W = 1280;
static const int WINDOW_H = 800;

int main(int argc, char* argv[]) {
    std::cout << "SculptApp v0.1.0" << std::endl;

    // --- SDL Init ---
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_Window* window = SDL_CreateWindow(
        "SculptApp - 3D Sculpting",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glCtx = SDL_GL_CreateContext(window);
    if (!glCtx) {
        std::cerr << "GL context failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_GL_SetSwapInterval(1);

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
    glEnable(GL_MULTISAMPLE);

    // --- ImGui Init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.15f, 0.95f);
    colors[ImGuiCol_Header]          = ImVec4(0.22f, 0.22f, 0.28f, 1.00f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.32f, 0.32f, 0.40f, 1.00f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.40f, 0.40f, 0.50f, 1.00f);
    colors[ImGuiCol_Button]          = ImVec4(0.25f, 0.28f, 0.36f, 1.00f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.35f, 0.40f, 0.52f, 1.00f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.45f, 0.50f, 0.65f, 1.00f);
    colors[ImGuiCol_SliderGrab]      = ImVec4(0.50f, 0.55f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]= ImVec4(0.60f, 0.65f, 0.80f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]  = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_FrameBgActive]   = ImVec4(0.30f, 0.30f, 0.38f, 1.00f);
    colors[ImGuiCol_CheckMark]       = ImVec4(0.55f, 0.75f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBg]         = ImVec4(0.10f, 0.10f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    colors[ImGuiCol_Separator]       = ImVec4(0.30f, 0.30f, 0.35f, 0.50f);
    colors[ImGuiCol_Tab]             = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TabHovered]      = ImVec4(0.35f, 0.40f, 0.52f, 1.00f);
    colors[ImGuiCol_TabActive]       = ImVec4(0.28f, 0.32f, 0.42f, 1.00f);

    int fbW_init, fbH_init, winW_init, winH_init;
    SDL_GL_GetDrawableSize(window, &fbW_init, &fbH_init);
    SDL_GetWindowSize(window, &winW_init, &winH_init);
    float dpiScale = static_cast<float>(fbW_init) / static_cast<float>(winW_init);
    io.FontGlobalScale = dpiScale;

    ImGui_ImplSDL2_InitForOpenGL(window, glCtx);
    ImGui_ImplOpenGL3_Init("#version 330");

    // --- Create mesh ---
    sculpt::HalfEdgeMesh mesh = sculpt::MeshGenerator::createSphere(1.0f, 64, 32);
    std::cout << "Mesh: " << mesh.vertexCount() << " verts, " << mesh.faceCount() << " faces" << std::endl;

    // --- Mode Manager ---
    sculpt::ModeManager modeManager;

    // --- Sculpt engine ---
    sculpt::SculptEngine sculptEngine;
    sculptEngine.setMesh(&mesh);
    sculptEngine.brush().settings().radius = 0.3f;
    sculptEngine.brush().settings().strength = 0.05f;
    sculptEngine.brush().settings().type = sculpt::BrushType::Draw;
    sculptEngine.brush().settings().falloff = sculpt::FalloffType::Smooth;

    // --- Edit engine ---
    sculpt::EditEngine editEngine;
    editEngine.setMesh(&mesh);
    // Share BVH from sculpt engine
    editEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh()));

    // --- Paint engine ---
    sculpt::PaintEngine paintEngine;
    paintEngine.setMesh(&mesh);
    paintEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh()));

    // --- Camera ---
    sculpt::Camera camera;
    camera.setDistance(3.0f);

    // --- Renderer ---
    sculpt::Renderer renderer;
    if (!renderer.init()) {
        std::cerr << "Renderer init failed" << std::endl;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        SDL_GL_DeleteContext(glCtx);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    renderer.uploadMesh(mesh);

    // --- Selection Renderer ---
    sculpt::SelectionRenderer selRenderer;
    selRenderer.init();

    // --- State ---
    bool running = true;
    bool sculpting = false;
    bool orbiting = false;
    bool panning = false;
    bool needsUpload = false;
    int prevMouseX = 0, prevMouseY = 0;

    // Sculpt mode state
    int currentBrush = 0;
    int currentFalloff = 0;
    bool wireframe = false;
    bool invertBrush = false;
    bool symmetryX = false;
    float brushRadius = 0.3f;
    float brushStrength = 0.05f;

    // Paint mode state
    bool painting = false;
    float paintColor[3] = {0.85f, 0.25f, 0.20f};
    float paintRadius = 0.3f;
    float paintStrength = 1.0f;
    bool paintFaceFill = false;
    int shadingModeIdx = 0; // 0=MatCap, 1=Flat, 2=VertexColor, 3=GamePreview
    bool flatShading = false;

    // Lowpoly tools state
    float gridSize = 0.1f;
    int decimateTarget = 500;
    int mirrorAxis = 0;

    // Export state
    int exportPresetIdx = 0;

    // Animation / Pose state
    sculpt::Armature armature;
    sculpt::BoneRenderer boneRenderer;
    sculpt::AnimationPlayer animPlayer;
    sculpt::Animation idleAnim, walkAnim, waveAnim, jumpAnim;
    std::vector<sculpt::Vec3> restPositions, restNormals;
    int selectedBone = -1;
    bool hasArmature = false;
    int animPresetIdx = 0;

    boneRenderer.init();

    // Edit mode state
    int selectionModeIdx = 0; // 0=Vertex, 1=Edge, 2=Face
    float extrudeOffset = 0.2f;
    float bevelAmount = 0.1f;
    float insetAmount = 0.3f;
    int loopCutSegments = 1;

    // General
    bool showToolPanel = true;
    bool showStats = true;
    std::string statusMsg = "Ready";
    float statusTimer = 0.0f;
    int meshType = 0;
    int sphereSegments = 64;
    int sphereRings = 32;

    Uint64 lastTime = SDL_GetPerformanceCounter();
    std::cout << "\nAPI IS WORKING" << std::endl;

    // Helper: rebuild everything after mesh changes
    auto refreshMesh = [&]() {
        sculptEngine.setMesh(&mesh);
        editEngine.setMesh(&mesh);
        editEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh()));
        paintEngine.setMesh(&mesh);
        paintEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh()));
        renderer.uploadMesh(mesh);
        if (modeManager.isEdit()) {
            selRenderer.updateMeshWireframe(mesh);
            selRenderer.updateSelection(mesh, editEngine.selection());
        }
    };

    // Helper: make a ray from current mouse position
    auto makeRay = [&](int mouseX, int mouseY) -> sculpt::Ray {
        int fbW, fbH, winW, winH;
        SDL_GL_GetDrawableSize(window, &fbW, &fbH);
        SDL_GetWindowSize(window, &winW, &winH);
        float scaleX = static_cast<float>(fbW) / static_cast<float>(winW);
        float scaleY = static_cast<float>(fbH) / static_cast<float>(winH);
        auto view = camera.getViewMatrix();
        auto proj = camera.getProjectionMatrix(static_cast<float>(fbW) / static_cast<float>(fbH));
        return sculpt::Raycaster::screenToWorldRay(
            static_cast<float>(mouseX) * scaleX,
            static_cast<float>(mouseY) * scaleY,
            static_cast<float>(fbW), static_cast<float>(fbH),
            view.data(), proj.data());
    };

    // --- Main loop ---
    while (running) {
        Uint64 nowTime = SDL_GetPerformanceCounter();
        float dt = static_cast<float>(nowTime - lastTime) / static_cast<float>(SDL_GetPerformanceFrequency());
        lastTime = nowTime;
        if (statusTimer > 0) statusTimer -= dt;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);

            bool imguiMouse = io.WantCaptureMouse;
            bool imguiKB = io.WantCaptureKeyboard;

            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN: {
                if (imguiKB) break;
                auto key = event.key.keysym.sym;
                bool ctrl = (event.key.keysym.mod & KMOD_CTRL) != 0;
                bool shift = (event.key.keysym.mod & KMOD_SHIFT) != 0;

                // TAB: toggle mode
                if (key == SDLK_TAB) {
                    // Save old mode BEFORE toggling
                    bool wasPose = modeManager.isPose();
                    modeManager.toggle();
                    // Restore rest pose when LEAVING Pose mode
                    if (wasPose && hasArmature) {
                        animPlayer.stop();
                        armature.resetPose();
                        for (size_t ri = 0; ri < std::min(restPositions.size(), mesh.vertexCount()); ri++) {
                            mesh.vertex(static_cast<int32_t>(ri)).position = restPositions[ri];
                            mesh.vertex(static_cast<int32_t>(ri)).normal = restNormals[ri];
                        }
                        renderer.uploadMesh(mesh);
                    }
                    sculpting = false;
                    painting = false;
                    needsUpload = false;
                    if (modeManager.isSculpt()) {
                        editEngine.deselectAll();
                        // Restore matcap shading when returning to sculpt
                        if (shadingModeIdx == 2) { // was vertex color for paint
                            renderer.setShadingMode(sculpt::ShadingMode::MatCap);
                            shadingModeIdx = 0;
                        }
                        statusMsg = "Sculpt Mode";
                    } else if (modeManager.isEdit()) {
                        editEngine.setMesh(&mesh);
                        editEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh()));
                        selRenderer.updateMeshWireframe(mesh);
                        statusMsg = "Edit Mode";
                    } else if (modeManager.isPaint()) {
                        renderer.setShadingMode(sculpt::ShadingMode::VertexColor);
                        shadingModeIdx = 2;
                        statusMsg = "Paint Mode";
                    } else if (modeManager.isPose()) {
                        // Snapshot rest pose
                        restPositions.resize(mesh.vertexCount());
                        restNormals.resize(mesh.vertexCount());
                        for (size_t i = 0; i < mesh.vertexCount(); i++) {
                            restPositions[i] = mesh.vertex(static_cast<int32_t>(i)).position;
                            restNormals[i] = mesh.vertex(static_cast<int32_t>(i)).normal;
                        }
                        if (hasArmature) {
                            boneRenderer.update(armature, selectedBone);
                        }
                        statusMsg = "Pose Mode";
                    }
                    statusTimer = 2.0f;
                    break;
                }

                if (key == SDLK_ESCAPE) { running = false; break; }

                if (modeManager.isSculpt()) {
                    // Sculpt mode keys
                    switch (key) {
                        case SDLK_1:
                            currentBrush = 0;
                            sculptEngine.brush().settings().type = sculpt::BrushType::Draw;
                            break;
                        case SDLK_2:
                            currentBrush = 1;
                            sculptEngine.brush().settings().type = sculpt::BrushType::Smooth;
                            break;
                        case SDLK_3:
                            currentBrush = 2;
                            sculptEngine.brush().settings().type = sculpt::BrushType::Grab;
                            break;
                        case SDLK_x:
                            invertBrush = !invertBrush;
                            sculptEngine.brush().settings().invert = invertBrush;
                            break;
                        case SDLK_w:
                            wireframe = !wireframe;
                            renderer.setWireframe(wireframe);
                            break;
                        case SDLK_EQUALS: case SDLK_PLUS:
                            brushRadius = std::min(3.0f, brushRadius * 1.15f);
                            sculptEngine.brush().settings().radius = brushRadius;
                            break;
                        case SDLK_MINUS:
                            brushRadius = std::max(0.02f, brushRadius * 0.85f);
                            sculptEngine.brush().settings().radius = brushRadius;
                            break;
                        case SDLK_RIGHTBRACKET:
                            brushStrength = std::min(1.0f, brushStrength + 0.01f);
                            sculptEngine.brush().settings().strength = brushStrength;
                            break;
                        case SDLK_LEFTBRACKET:
                            brushStrength = std::max(0.005f, brushStrength - 0.01f);
                            sculptEngine.brush().settings().strength = brushStrength;
                            break;
                        default: break;
                    }
                } else {
                    // Edit mode keys
                    switch (key) {
                        case SDLK_1:
                            selectionModeIdx = 0;
                            editEngine.selection().mode = sculpt::SelectionMode::Vertex;
                            editEngine.deselectAll();
                            break;
                        case SDLK_2:
                            selectionModeIdx = 1;
                            editEngine.selection().mode = sculpt::SelectionMode::Edge;
                            editEngine.deselectAll();
                            break;
                        case SDLK_3:
                            selectionModeIdx = 2;
                            editEngine.selection().mode = sculpt::SelectionMode::Face;
                            editEngine.deselectAll();
                            break;
                        case SDLK_a:
                            if (editEngine.selection().hasSelection())
                                editEngine.deselectAll();
                            else
                                editEngine.selectAll();
                            selRenderer.updateSelection(mesh, editEngine.selection());
                            break;
                        case SDLK_e:
                            if (editEngine.extrude(extrudeOffset)) {
                                refreshMesh();
                                statusMsg = "Extruded"; statusTimer = 2.0f;
                            }
                            break;
                        case SDLK_i:
                            if (editEngine.insetFace(insetAmount)) {
                                refreshMesh();
                                statusMsg = "Inset Face"; statusTimer = 2.0f;
                            }
                            break;
                        case SDLK_w:
                            wireframe = !wireframe;
                            renderer.setWireframe(wireframe);
                            break;
                        default: break;
                    }
                    if (ctrl && key == SDLK_r) {
                        if (editEngine.loopCut(loopCutSegments)) {
                            refreshMesh();
                            statusMsg = "Loop Cut"; statusTimer = 2.0f;
                        }
                    }
                    if (ctrl && key == SDLK_b) {
                        if (editEngine.bevel(bevelAmount)) {
                            refreshMesh();
                            statusMsg = "Bevel"; statusTimer = 2.0f;
                        }
                    }
                }
                break;
            }

            case SDL_MOUSEBUTTONDOWN:
                if (!imguiMouse) {
                    prevMouseX = event.button.x;
                    prevMouseY = event.button.y;

                    if (event.button.button == SDL_BUTTON_LEFT) {
                        if (modeManager.isPaint()) {
                            painting = true;
                            // Update paint settings
                            paintEngine.settings().color = sculpt::Vec3(paintColor[0], paintColor[1], paintColor[2]);
                            paintEngine.settings().radius = paintRadius;
                            paintEngine.settings().strength = paintStrength;
                            // Paint on initial click
                            sculpt::Ray ray = makeRay(event.button.x, event.button.y);
                            if (paintFaceFill) {
                                if (paintEngine.fillFace(ray)) {
                                    renderer.updateMeshVertices(mesh);
                                }
                            } else {
                                sculpt::RayHit hit = sculptEngine.raycast(ray);
                                if (hit.hit) {
                                    paintEngine.paintStroke(hit.position);
                                    renderer.updateMeshVertices(mesh);
                                }
                            }
                        } else if (modeManager.isSculpt()) {
                            sculpting = true;
                        } else {
                            // Edit mode: pick selection
                            bool shift = (SDL_GetModState() & KMOD_SHIFT) != 0;
                            sculpt::Ray ray = makeRay(event.button.x, event.button.y);
                            editEngine.pick(ray, shift);
                            selRenderer.updateSelection(mesh, editEngine.selection());
                        }
                    } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                        orbiting = true;
                    } else if (event.button.button == SDL_BUTTON_RIGHT) {
                        panning = true;
                    }
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    sculpting = false;
                    painting = false;
                    if (needsUpload) {
                        sculptEngine.rebuildBVH();
                        needsUpload = false;
                    }
                } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                    orbiting = false;
                } else if (event.button.button == SDL_BUTTON_RIGHT) {
                    panning = false;
                }
                break;

            case SDL_MOUSEMOTION: {
                if (imguiMouse) break;

                float dx = static_cast<float>(event.motion.x - prevMouseX);
                float dy = static_cast<float>(event.motion.y - prevMouseY);
                prevMouseX = event.motion.x;
                prevMouseY = event.motion.y;

                if (orbiting) {
                    camera.orbit(dx, dy);
                } else if (panning) {
                    camera.pan(dx, dy);
                } else if (sculpting && modeManager.isSculpt()) {
                    sculpt::Ray ray = makeRay(event.motion.x, event.motion.y);
                    sculpt::RayHit hit = sculptEngine.raycast(ray);
                    if (hit.hit) {
                        if (sculptEngine.brush().settings().type == sculpt::BrushType::Grab) {
                            sculpt::Vec3 delta = camera.getRight() * (dx * 0.005f) +
                                                 camera.getUp() * (-dy * 0.005f);
                            sculptEngine.grabStroke(hit.position, delta);
                        } else {
                            sculptEngine.stroke(hit.position, hit.normal);
                        }
                        renderer.updateMeshVertices(mesh); // fast: vertices only
                        needsUpload = true;
                    }
                } else if (painting && modeManager.isPaint() && !paintFaceFill) {
                    sculpt::Ray ray = makeRay(event.motion.x, event.motion.y);
                    sculpt::RayHit hit = sculptEngine.raycast(ray);
                    if (hit.hit) {
                        paintEngine.settings().color = sculpt::Vec3(paintColor[0], paintColor[1], paintColor[2]);
                        paintEngine.settings().radius = paintRadius;
                        paintEngine.settings().strength = paintStrength;
                        paintEngine.paintStroke(hit.position);
                        renderer.updateMeshVertices(mesh); // fast: colors only
                    }
                } else if (modeManager.isEdit()) {
                    sculpt::Ray ray = makeRay(event.motion.x, event.motion.y);
                    editEngine.updateHover(ray);
                }
                break;
            }

            case SDL_MOUSEWHEEL:
                if (!imguiMouse) {
                    camera.zoom(static_cast<float>(event.wheel.y));
                }
                break;
            }
        }

        // Handle edit engine dirty flag
        if (editEngine.isDirty()) {
            refreshMesh();
            editEngine.clearDirty();
        }

        // --- Pose mode animation update ---
        if (modeManager.isPose() && hasArmature && animPlayer.playing) {
            animPlayer.update(dt, armature);
            armature.applyPose(mesh, restPositions, restNormals);
            renderer.updateMeshVertices(mesh); // fast path: no realloc
            boneRenderer.update(armature, selectedBone);
        }

        // --- ImGui Frame ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ============================
        // MODE TAB BAR (Top)
        // ============================
        {
            ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
            ImGui::Begin("##mode_header", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);

            float btnW4 = (ImGui::GetContentRegionAvail().x - 24) / 4.0f;

            struct ModeBtn { const char* name; sculpt::AppMode mode; ImVec4 col; ImVec4 hov; };
            ModeBtn modes[] = {
                {"Sculpt", sculpt::AppMode::Sculpt, {0.20f,0.45f,0.75f,1}, {0.25f,0.50f,0.80f,1}},
                {"Edit",   sculpt::AppMode::Edit,   {0.75f,0.45f,0.15f,1}, {0.80f,0.50f,0.20f,1}},
                {"Paint",  sculpt::AppMode::Paint,  {0.65f,0.20f,0.65f,1}, {0.75f,0.30f,0.75f,1}},
                {"Pose",   sculpt::AppMode::Pose,   {0.20f,0.65f,0.45f,1}, {0.25f,0.75f,0.50f,1}},
            };
            for (int mi = 0; mi < 4; mi++) {
                if (mi > 0) ImGui::SameLine();
                bool active = (modeManager.current == modes[mi].mode);
                if (active) { ImGui::PushStyleColor(ImGuiCol_Button, modes[mi].col); ImGui::PushStyleColor(ImGuiCol_ButtonHovered, modes[mi].hov); }
                if (ImGui::Button(modes[mi].name, ImVec2(btnW4, 34))) {
                    modeManager.current = modes[mi].mode;
                    if (modes[mi].mode == sculpt::AppMode::Sculpt) editEngine.deselectAll();
                    if (modes[mi].mode == sculpt::AppMode::Edit) {
                        editEngine.setMesh(&mesh); editEngine.setBVH(const_cast<sculpt::BVH*>(&sculptEngine.bvh())); selRenderer.updateMeshWireframe(mesh);
                    }
                    if (modes[mi].mode == sculpt::AppMode::Paint) {
                        renderer.setShadingMode(sculpt::ShadingMode::VertexColor); shadingModeIdx = 2;
                    }
                    if (modes[mi].mode == sculpt::AppMode::Pose) {
                        restPositions.resize(mesh.vertexCount());
                        restNormals.resize(mesh.vertexCount());
                        for (size_t pi = 0; pi < mesh.vertexCount(); pi++) {
                            restPositions[pi] = mesh.vertex(static_cast<int32_t>(pi)).position;
                            restNormals[pi] = mesh.vertex(static_cast<int32_t>(pi)).normal;
                        }
                        if (hasArmature) boneRenderer.update(armature, selectedBone);
                    }
                }
                if (active) ImGui::PopStyleColor(2);
            }
            ImGui::TextDisabled("TAB to cycle modes");

            ImGui::End();
        }

        // ============================
        // TOOL PANEL
        // ============================
        if (showToolPanel) {
            ImGui::SetNextWindowPos(ImVec2(10, 75), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_FirstUseEver);

            if (modeManager.isSculpt()) {
                // ---- SCULPT MODE PANEL ----
                ImGui::Begin("Sculpt Tools", &showToolPanel, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::Text("BRUSH");
                ImGui::Separator();
                ImGui::Spacing();

                const char* brushNames[] = { "Draw", "Smooth", "Grab" };
                float buttonW = (ImGui::GetContentRegionAvail().x - 16) / 3.0f;
                for (int i = 0; i < 3; i++) {
                    if (i > 0) ImGui::SameLine();
                    bool isSel = (currentBrush == i);
                    if (isSel) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.50f, 0.75f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.55f, 0.80f, 1.0f));
                    }
                    if (ImGui::Button(brushNames[i], ImVec2(buttonW, 32))) {
                        currentBrush = i;
                        sculptEngine.brush().settings().type = static_cast<sculpt::BrushType>(i);
                    }
                    if (isSel) ImGui::PopStyleColor(2);
                }

                ImGui::Spacing();
                ImGui::Text("SETTINGS");
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::SliderFloat("Radius", &brushRadius, 0.02f, 2.0f, "%.3f"))
                    sculptEngine.brush().settings().radius = brushRadius;
                if (ImGui::SliderFloat("Strength", &brushStrength, 0.005f, 0.5f, "%.3f"))
                    sculptEngine.brush().settings().strength = brushStrength;

                const char* falloffNames[] = { "Smooth", "Linear", "Constant" };
                if (ImGui::Combo("Falloff", &currentFalloff, falloffNames, 3))
                    sculptEngine.brush().settings().falloff = static_cast<sculpt::FalloffType>(currentFalloff);

                // Falloff curve preview
                ImGui::Text("Falloff Curve:");
                ImVec2 canvasPos = ImGui::GetCursorScreenPos();
                ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, 50);
                ImGui::InvisibleButton("falloff_cv", canvasSize);
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 30, 35, 255), 4.0f);
                ImVec2 prev;
                for (int i = 0; i <= 50; i++) {
                    float t = static_cast<float>(i) / 50.0f;
                    float val = 0;
                    switch (currentFalloff) {
                        case 0: val = 1.0f - (3*t*t - 2*t*t*t); break;
                        case 1: val = 1.0f - t; break;
                        case 2: val = (t < 1.0f) ? 1.0f : 0.0f; break;
                    }
                    ImVec2 p(canvasPos.x + t * canvasSize.x, canvasPos.y + canvasSize.y - val * canvasSize.y);
                    if (i > 0) dl->AddLine(prev, p, IM_COL32(140, 170, 255, 255), 2.0f);
                    prev = p;
                }

                ImGui::Spacing();
                ImGui::Text("OPTIONS");
                ImGui::Separator();
                if (ImGui::Checkbox("Invert (X)", &invertBrush))
                    sculptEngine.brush().settings().invert = invertBrush;
                if (ImGui::Checkbox("Wireframe (W)", &wireframe))
                    renderer.setWireframe(wireframe);

                ImGui::End();

            } else if (modeManager.isEdit()) {
                // ---- EDIT MODE PANEL ----
                ImGui::Begin("Edit Tools", &showToolPanel, ImGuiWindowFlags_AlwaysAutoResize);

                // Selection mode
                ImGui::Text("SELECT MODE");
                ImGui::Separator();
                ImGui::Spacing();

                const char* selModeNames[] = { "Vertex", "Edge", "Face" };
                float btnW = (ImGui::GetContentRegionAvail().x - 16) / 3.0f;
                for (int i = 0; i < 3; i++) {
                    if (i > 0) ImGui::SameLine();
                    bool isSel = (selectionModeIdx == i);
                    if (isSel) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.45f, 0.15f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.80f, 0.50f, 0.20f, 1.0f));
                    }
                    if (ImGui::Button(selModeNames[i], ImVec2(btnW, 32))) {
                        selectionModeIdx = i;
                        editEngine.selection().mode = static_cast<sculpt::SelectionMode>(i);
                        editEngine.deselectAll();
                        selRenderer.updateSelection(mesh, editEngine.selection());
                    }
                    if (isSel) ImGui::PopStyleColor(2);
                }

                ImGui::Spacing();
                ImGui::TextDisabled("Click to select, Shift+Click to add");
                ImGui::TextDisabled("A = Select/Deselect All");

                ImGui::Spacing();
                ImGui::Spacing();

                // Tools
                ImGui::Text("TOOLS");
                ImGui::Separator();
                ImGui::Spacing();

                float fullW = ImGui::GetContentRegionAvail().x;

                // Extrude
                ImGui::SliderFloat("Extrude Dist", &extrudeOffset, 0.01f, 1.0f, "%.2f");
                if (ImGui::Button("Extrude (E)", ImVec2(fullW, 30))) {
                    if (editEngine.extrude(extrudeOffset)) {
                        refreshMesh();
                        statusMsg = "Extruded"; statusTimer = 2.0f;
                    } else {
                        statusMsg = "Select faces to extrude"; statusTimer = 2.0f;
                    }
                }

                ImGui::Spacing();

                // Inset Face
                ImGui::SliderFloat("Inset Amount", &insetAmount, 0.01f, 0.9f, "%.2f");
                if (ImGui::Button("Inset Face (I)", ImVec2(fullW, 30))) {
                    if (editEngine.insetFace(insetAmount)) {
                        refreshMesh();
                        statusMsg = "Inset Face"; statusTimer = 2.0f;
                    } else {
                        statusMsg = "Select faces to inset"; statusTimer = 2.0f;
                    }
                }

                ImGui::Spacing();

                // Loop Cut
                ImGui::SliderInt("Loop Segments", &loopCutSegments, 1, 5);
                if (ImGui::Button("Loop Cut (Ctrl+R)", ImVec2(fullW, 30))) {
                    if (editEngine.loopCut(loopCutSegments)) {
                        refreshMesh();
                        statusMsg = "Loop Cut"; statusTimer = 2.0f;
                    } else {
                        statusMsg = "Hover over an edge first"; statusTimer = 2.0f;
                    }
                }

                ImGui::Spacing();

                // Bevel
                ImGui::SliderFloat("Bevel Amount", &bevelAmount, 0.01f, 0.5f, "%.2f");
                if (ImGui::Button("Bevel (Ctrl+B)", ImVec2(fullW, 30))) {
                    if (editEngine.bevel(bevelAmount)) {
                        refreshMesh();
                        statusMsg = "Bevel"; statusTimer = 2.0f;
                    } else {
                        statusMsg = "Select edges to bevel"; statusTimer = 2.0f;
                    }
                }

                ImGui::Spacing();

                if (ImGui::Checkbox("Wireframe (W)", &wireframe))
                    renderer.setWireframe(wireframe);

                ImGui::End();

            } else if (modeManager.isPaint()) {
                // ---- PAINT MODE PANEL ----
                ImGui::Begin("Paint Tools", &showToolPanel, ImGuiWindowFlags_AlwaysAutoResize);

                ImGui::Text("COLOR");
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::ColorEdit3("Paint Color", paintColor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueWheel);

                ImGui::Spacing();
                ImGui::Text("PALETTE");
                ImGui::Separator();
                auto& palette = paintEngine.palette();
                int cols = 4;
                for (size_t i = 0; i < palette.size(); i++) {
                    if (i % cols != 0) ImGui::SameLine();
                    ImGui::PushID(static_cast<int>(i));
                    ImVec4 c(palette[i].x, palette[i].y, palette[i].z, 1);
                    if (ImGui::ColorButton("##pal", c, 0, ImVec2(28, 28))) {
                        paintColor[0] = palette[i].x;
                        paintColor[1] = palette[i].y;
                        paintColor[2] = palette[i].z;
                    }
                    ImGui::PopID();
                }
                ImGui::Spacing();
                if (ImGui::Button("Save to Palette", ImVec2(ImGui::GetContentRegionAvail().x, 24))) {
                    if (palette.size() < 32) palette.push_back(sculpt::Vec3(paintColor[0], paintColor[1], paintColor[2]));
                }

                ImGui::Spacing();
                ImGui::Text("BRUSH");
                ImGui::Separator();
                ImGui::SliderFloat("Radius##p", &paintRadius, 0.02f, 2.0f, "%.3f");
                ImGui::SliderFloat("Strength##p", &paintStrength, 0.1f, 1.0f, "%.2f");
                ImGui::Checkbox("Face Fill (click fills face)", &paintFaceFill);

                ImGui::Spacing();
                if (ImGui::Button("Eyedropper (Alt+Click)", ImVec2(ImGui::GetContentRegionAvail().x, 24))) {
                    statusMsg = "Alt+Click on mesh to sample"; statusTimer = 2.0f;
                }

                if (ImGui::Checkbox("Wireframe (W)", &wireframe))
                    renderer.setWireframe(wireframe);

                ImGui::End();

            } else if (modeManager.isPose()) {
                // ---- POSE MODE PANEL ----
                ImGui::Begin("Pose Tools", &showToolPanel, ImGuiWindowFlags_AlwaysAutoResize);

                float fullW = ImGui::GetContentRegionAvail().x;

                ImGui::Text("ARMATURE");
                ImGui::Separator();
                ImGui::Spacing();

                if (!hasArmature) {
                    if (ImGui::Button("Auto-Rig Humanoid", ImVec2(fullW, 32))) {
                        sculpt::AutoRig::rigHumanoid(mesh, armature);
                        sculpt::AutoRig::computeWeights(mesh, armature);
                        hasArmature = true;
                        // Generate preset animations
                        idleAnim = sculpt::Animation::createIdle(armature);
                        walkAnim = sculpt::Animation::createWalk(armature);
                        waveAnim = sculpt::Animation::createWave(armature);
                        jumpAnim = sculpt::Animation::createJump(armature);
                        armature.updatePoseMatrices();
                        boneRenderer.update(armature, selectedBone);
                        statusMsg = "Auto-rigged with " + std::to_string(armature.boneCount()) + " bones";
                        statusTimer = 3.0f;
                    }
                    ImGui::TextWrapped("Click to auto-rig the mesh with a humanoid skeleton.");
                } else {
                    ImGui::Text("Bones: %d", armature.boneCount());
                    ImGui::Spacing();

                    // Bone list
                    ImGui::Text("BONES");
                    ImGui::Separator();
                    if (ImGui::BeginListBox("##bonelist", ImVec2(fullW, 150))) {
                        for (int32_t bi = 0; bi < armature.boneCount(); bi++) {
                            bool isSel = (selectedBone == bi);
                            if (ImGui::Selectable(armature.bone(bi).name.c_str(), isSel)) {
                                selectedBone = bi;
                            }
                        }
                        ImGui::EndListBox();
                    }

                    // Selected bone rotation
                    if (selectedBone >= 0 && selectedBone < armature.boneCount()) {
                        ImGui::Spacing();
                        ImGui::Text("ROTATION: %s", armature.bone(selectedBone).name.c_str());
                        ImGui::Separator();
                        bool changed = false;
                        changed |= ImGui::SliderAngle("X##brot", &armature.bone(selectedBone).poseRotation[0], -180, 180);
                        changed |= ImGui::SliderAngle("Y##brot", &armature.bone(selectedBone).poseRotation[1], -180, 180);
                        changed |= ImGui::SliderAngle("Z##brot", &armature.bone(selectedBone).poseRotation[2], -180, 180);
                        if (changed && !animPlayer.playing) {
                            armature.updatePoseMatrices();
                            armature.applyPose(mesh, restPositions, restNormals);
                            renderer.updateMeshVertices(mesh);
                            boneRenderer.update(armature, selectedBone);
                        }
                    }

                    if (ImGui::Button("Reset Pose", ImVec2(fullW, 26))) {
                        armature.resetPose();
                        armature.applyPose(mesh, restPositions, restNormals);
                        renderer.updateMeshVertices(mesh);
                        boneRenderer.update(armature, selectedBone);
                        animPlayer.stop();
                    }

                    // Animation controls
                    ImGui::Spacing();
                    ImGui::Text("ANIMATION");
                    ImGui::Separator();
                    ImGui::Spacing();

                    const char* animNames[] = {"Idle", "Walk", "Wave", "Jump"};
                    ImGui::Combo("Preset", &animPresetIdx, animNames, 4);

                    if (ImGui::Button("Load Animation", ImVec2(fullW, 26))) {
                        sculpt::Animation* anims[] = {&idleAnim, &walkAnim, &waveAnim, &jumpAnim};
                        animPlayer.current = anims[animPresetIdx];
                        animPlayer.time = 0;
                        animPlayer.playing = false;
                        statusMsg = "Loaded: " + std::string(animNames[animPresetIdx]);
                        statusTimer = 2.0f;
                    }

                    // Transport controls
                    float halfBtn = (fullW - 8) * 0.333f;
                    if (ImGui::Button(animPlayer.playing ? "Pause" : "Play", ImVec2(halfBtn, 28))) {
                        if (animPlayer.playing) animPlayer.pause();
                        else animPlayer.play();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop", ImVec2(halfBtn, 28))) {
                        animPlayer.stop();
                        armature.resetPose();
                        armature.applyPose(mesh, restPositions, restNormals);
                        renderer.updateMeshVertices(mesh);
                        boneRenderer.update(armature, selectedBone);
                    }
                    ImGui::SameLine();
                    ImGui::Checkbox("Loop", &animPlayer.loop);

                    ImGui::SliderFloat("Speed", &animPlayer.speed, 0.1f, 3.0f, "%.1fx");

                    if (animPlayer.current) {
                        float dur = animPlayer.current->duration;
                        if (dur > 0 && ImGui::SliderFloat("Time", &animPlayer.time, 0, dur, "%.2fs")) {
                            animPlayer.current->evaluate(animPlayer.time, armature);
                            armature.updatePoseMatrices();
                            armature.applyPose(mesh, restPositions, restNormals);
                            renderer.updateMeshVertices(mesh);
                            boneRenderer.update(armature, selectedBone);
                        }
                    }

                    // Remove rig
                    ImGui::Spacing();
                    if (ImGui::Button("Remove Rig", ImVec2(fullW, 24))) {
                        armature.clear();
                        hasArmature = false;
                        selectedBone = -1;
                        animPlayer.stop();
                        animPlayer.current = nullptr;
                        for (size_t ri = 0; ri < std::min(restPositions.size(), mesh.vertexCount()); ri++) {
                            mesh.vertex(static_cast<int32_t>(ri)).position = restPositions[ri];
                            mesh.vertex(static_cast<int32_t>(ri)).normal = restNormals[ri];
                        }
                        renderer.uploadMesh(mesh);
                    }
                }

                ImGui::End();
            }
        }

        // ============================
        // MESH PANEL (shared)
        // ============================
        {
            ImGui::SetNextWindowPos(ImVec2(10, 580), ImGuiCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(280, 0), ImGuiCond_Always);
            ImGui::Begin("Mesh", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

            const char* meshNames[] = { "Sphere","Cube","Plane","Cylinder","Cone","Torus","Capsule","Wedge","Pyramid" };
            ImGui::Combo("Shape", &meshType, meshNames, 9);
            if (meshType == 0) {
                ImGui::SliderInt("Segments", &sphereSegments, 8, 128);
                ImGui::SliderInt("Rings", &sphereRings, 4, 64);
            }

            float fullW = ImGui::GetContentRegionAvail().x;
            if (ImGui::Button("Create Mesh", ImVec2(fullW, 28))) {
                switch (meshType) {
                    case 0: mesh = sculpt::MeshGenerator::createSphere(1.0f, sphereSegments, sphereRings); break;
                    case 1: mesh = sculpt::MeshGenerator::createCube(2.0f); break;
                    case 2: mesh = sculpt::MeshGenerator::createPlane(2.0f, 20); break;
                    case 3: mesh = sculpt::MeshGenerator::createCylinder(0.5f, 2.0f, 16); break;
                    case 4: mesh = sculpt::MeshGenerator::createCone(0.6f, 1.5f, 16); break;
                    case 5: mesh = sculpt::MeshGenerator::createTorus(0.7f, 0.25f, 24, 12); break;
                    case 6: mesh = sculpt::MeshGenerator::createCapsule(0.4f, 1.0f, 16, 8); break;
                    case 7: mesh = sculpt::MeshGenerator::createWedge(1.0f, 0.8f, 1.5f); break;
                    case 8: mesh = sculpt::MeshGenerator::createPyramid(1.5f, 1.2f); break;
                }
                refreshMesh();
                statusMsg = "Created " + std::string(meshNames[meshType]); statusTimer = 2.0f;
            }

            // Game templates
            ImGui::Spacing();
            ImGui::Text("GAME TEMPLATES");
            ImGui::Separator();
            float halfW2 = (fullW - 8) * 0.5f;
            if (ImGui::Button("Tree", ImVec2(halfW2, 24))) { mesh = sculpt::MeshGenerator::createGameTree(); refreshMesh(); }
            ImGui::SameLine();
            if (ImGui::Button("Rock", ImVec2(halfW2, 24))) { mesh = sculpt::MeshGenerator::createGameRock(); refreshMesh(); }
            if (ImGui::Button("House", ImVec2(halfW2, 24))) { mesh = sculpt::MeshGenerator::createGameHouse(); refreshMesh(); }
            ImGui::SameLine();
            if (ImGui::Button("Sword", ImVec2(halfW2, 24))) { mesh = sculpt::MeshGenerator::createGameSword(); refreshMesh(); }

            // Procedural generators
            ImGui::Spacing();
            ImGui::Text("GENERATORS");
            ImGui::Separator();
            static sculpt::MeshGenerator::RockParams rockP;
            static sculpt::MeshGenerator::TreeParams treeP;
            static sculpt::MeshGenerator::TerrainParams terrainP;
            static sculpt::MeshGenerator::CrystalParams crystalP;
            static int genType = 0;
            const char* genNames[] = {"Rock","Tree","Terrain","Crystal"};
            ImGui::Combo("Generator", &genType, genNames, 4);
            static int genSeed = 42;
            ImGui::SliderInt("Seed", &genSeed, 1, 9999);
            if (ImGui::Button("Generate", ImVec2(fullW, 26))) {
                switch (genType) {
                    case 0: rockP.seed = genSeed; mesh = sculpt::MeshGenerator::generateRock(rockP); break;
                    case 1: treeP.seed = genSeed; mesh = sculpt::MeshGenerator::generateTree(treeP); break;
                    case 2: terrainP.seed = genSeed; mesh = sculpt::MeshGenerator::generateTerrain(terrainP); break;
                    case 3: crystalP.seed = genSeed; mesh = sculpt::MeshGenerator::generateCrystal(crystalP); break;
                }
                refreshMesh();
                statusMsg = "Generated " + std::string(genNames[genType]); statusTimer = 2.0f;
            }

            // Shading modes
            ImGui::Spacing();
            ImGui::Text("SHADING");
            ImGui::Separator();
            const char* shadingNames[] = {"MatCap","Flat Color","Vertex Color","Game Preview"};
            if (ImGui::Combo("Mode##shading", &shadingModeIdx, shadingNames, 4)) {
                renderer.setShadingMode(static_cast<sculpt::ShadingMode>(shadingModeIdx));
            }
            if (ImGui::Checkbox("Flat Shading", &flatShading)) {
                renderer.setFlatShading(flatShading);
            }

            // Lowpoly tools
            ImGui::Spacing();
            ImGui::Text("LOWPOLY TOOLS");
            ImGui::Separator();
            ImGui::SliderFloat("Grid Size", &gridSize, 0.01f, 0.5f, "%.2f");
            if (ImGui::Button("Snap to Grid", ImVec2(fullW, 24))) {
                sculpt::LowpolyTools::snapToGrid(mesh, gridSize);
                refreshMesh();
                statusMsg = "Snapped to grid"; statusTimer = 2.0f;
            }
            const char* axisNames[] = {"X","Y","Z"};
            ImGui::Combo("Mirror Axis", &mirrorAxis, axisNames, 3);
            if (ImGui::Button("Mirror", ImVec2(fullW, 24))) {
                sculpt::LowpolyTools::mirror(mesh, mirrorAxis);
                refreshMesh();
                statusMsg = "Mirrored"; statusTimer = 2.0f;
            }
            ImGui::SliderInt("Target Faces", &decimateTarget, 10, static_cast<int>(mesh.faceCount()));
            if (ImGui::Button("Decimate", ImVec2(fullW, 24))) {
                sculpt::LowpolyTools::decimate(mesh, decimateTarget);
                refreshMesh();
                statusMsg = "Decimated to " + std::to_string(mesh.faceCount()) + " faces"; statusTimer = 2.0f;
            }

            // Export
            ImGui::Spacing();
            ImGui::Text("EXPORT");
            ImGui::Separator();
            if (ImGui::Button("Export OBJ", ImVec2(fullW, 24))) {
                if (sculpt::ObjLoader::save("sculpt_output.obj", mesh)) {
                    statusMsg = "Saved: sculpt_output.obj"; statusTimer = 3.0f;
                }
            }
            const char* presetNames[] = {"Default","Unity","Godot","Web (glb)"};
            ImGui::Combo("Preset", &exportPresetIdx, presetNames, 4);
            if (ImGui::Button("Export glTF", ImVec2(fullW, 24))) {
                sculpt::ExportPreset preset;
                switch (exportPresetIdx) {
                    case 0: preset = sculpt::GltfExporter::presetDefault(); break;
                    case 1: preset = sculpt::GltfExporter::presetUnity(); break;
                    case 2: preset = sculpt::GltfExporter::presetGodot(); break;
                    case 3: preset = sculpt::GltfExporter::presetWeb(); break;
                }
                std::string ext = preset.binary ? ".glb" : ".gltf";
                std::string path = "sculpt_output" + ext;
                if (sculpt::GltfExporter::exportGltf(path, mesh, preset)) {
                    statusMsg = "Saved: " + path; statusTimer = 3.0f;
                }
            }

            if (statusTimer > 0) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.8f, 1.0f, 1.0f));
                ImGui::TextWrapped("%s", statusMsg.c_str());
                ImGui::PopStyleColor();
            }

            ImGui::End();
        }

        // ============================
        // STATS (Bottom-right)
        // ============================
        {
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(winW) - 230, static_cast<float>(winH) - 110), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(220, 100), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.7f);
            ImGui::Begin("##stats", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

            ImGui::Text("Vertices: %zu", mesh.vertexCount());
            ImGui::Text("Faces:    %zu", mesh.faceCount());
            ImGui::Text("FPS:      %.0f", io.Framerate);
            const char* modeStr = modeManager.isSculpt() ? "SCULPT" : modeManager.isEdit() ? "EDIT" : modeManager.isPaint() ? "PAINT" : "POSE";
            ImVec4 modeCol = modeManager.isSculpt() ? ImVec4(0.3f,0.6f,1,1) : modeManager.isEdit() ? ImVec4(1,0.6f,0.2f,1) : modeManager.isPaint() ? ImVec4(0.8f,0.3f,0.8f,1) : ImVec4(0.2f,0.8f,0.5f,1);
            ImGui::TextColored(modeCol, "Mode: %s", modeStr);

            ImGui::End();
        }

        // ============================
        // CONTROLS (Top-right)
        // ============================
        {
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            ImGui::SetNextWindowPos(ImVec2(static_cast<float>(winW) - 260, 155), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(250, 0), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.6f);
            ImGui::Begin("##controls", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

            if (modeManager.isSculpt()) {
                ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "Sculpt Controls");
                ImGui::Separator();
                ImGui::Text("LMB Drag  : Sculpt");
                ImGui::Text("MMB Drag  : Orbit");
                ImGui::Text("RMB Drag  : Pan");
                ImGui::Text("Scroll    : Zoom");
                ImGui::Text("1/2/3     : Brush type");
                ImGui::Text("+/- [/]   : Size/Strength");
                ImGui::Text("TAB       : Edit Mode");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Edit Controls");
                ImGui::Separator();
                ImGui::Text("LMB       : Select");
                ImGui::Text("Shift+LMB : Add to sel");
                ImGui::Text("MMB Drag  : Orbit");
                ImGui::Text("RMB Drag  : Pan");
                ImGui::Text("1/2/3     : Vert/Edge/Face");
                ImGui::Text("A         : Select All");
                ImGui::Text("E         : Extrude");
                ImGui::Text("I         : Inset Face");
                ImGui::Text("Ctrl+R    : Loop Cut");
                ImGui::Text("Ctrl+B    : Bevel");
                ImGui::Text("TAB       : Paint Mode");
            }

            if (modeManager.isPaint()) {
                ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.8f, 1.0f), "Paint Controls");
                ImGui::Separator();
                ImGui::Text("LMB Drag  : Paint color");
                ImGui::Text("MMB Drag  : Orbit");
                ImGui::Text("RMB Drag  : Pan");
                ImGui::Text("Scroll    : Zoom");
                ImGui::Text("TAB       : Pose Mode");
            }

            if (modeManager.isPose()) {
                ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.5f, 1.0f), "Pose Controls");
                ImGui::Separator();
                ImGui::Text("Auto-Rig  : Place skeleton");
                ImGui::Text("Bone List : Select bone");
                ImGui::Text("Sliders   : Rotate bone");
                ImGui::Text("Play/Pause: Animate");
                ImGui::Text("MMB Drag  : Orbit");
                ImGui::Text("RMB Drag  : Pan");
                ImGui::Text("TAB       : Sculpt Mode");
            }

            ImGui::End();
        }

        // ============================
        // NAVIGATION GIZMO (Top-right, above controls)
        // ============================
        {
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);

            const float gizmoSize = 120.0f;
            const float gizmoMargin = 15.0f;
            const float gizmoRadius = gizmoSize * 0.38f;
            // Position: top-right corner, above the controls panel
            float gizmoX = static_cast<float>(winW) - gizmoSize - gizmoMargin;
            float gizmoY = gizmoMargin;

            ImGui::SetNextWindowPos(ImVec2(gizmoX, gizmoY), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(gizmoSize, gizmoSize), ImGuiCond_Always);
            ImGui::SetNextWindowBgAlpha(0.0f);
            ImGui::Begin("##nav_gizmo", nullptr,
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground);

            ImVec2 gCenter = ImVec2(gizmoX + gizmoSize * 0.5f, gizmoY + gizmoSize * 0.5f);
            ImDrawList* dl = ImGui::GetWindowDrawList();

            // Draw background circle
            dl->AddCircleFilled(gCenter, gizmoRadius + 8, IM_COL32(25, 25, 30, 200), 48);
            dl->AddCircle(gCenter, gizmoRadius + 8, IM_COL32(60, 60, 70, 255), 48, 1.5f);

            // Get camera rotation to orient the gizmo axes
            float yaw = camera.getYaw();
            float pitch = camera.getPitch();
            float cosY = std::cos(yaw), sinY = std::sin(yaw);
            float cosP = std::cos(pitch), sinP = std::sin(pitch);

            // Project 3D axis directions to 2D gizmo space
            // Camera looks from (sinY*cosP, sinP, cosY*cosP) toward origin
            // We project world axes onto the screen plane
            struct AxisInfo {
                float sx, sy;    // screen projected position
                float depth;     // for draw order (back to front)
                ImU32 color;
                ImU32 colorDim;
                const char* label;
                const char* negLabel;
                float dirYaw;    // camera yaw to snap to this axis
                float dirPitch;  // camera pitch to snap
                bool positive;
            };

            // Project a 3D direction to 2D gizmo coordinates
            // View-space: right = (cosY, 0, -sinY), up = (-sinY*sinP, cosP, -cosY*sinP), fwd = (sinY*cosP, sinP, cosY*cosP)
            auto project = [&](float wx, float wy, float wz) -> std::tuple<float, float, float> {
                float rx = cosY * wx + 0 * wy + (-sinY) * wz;
                float ry = (-sinY * sinP) * wx + cosP * wy + (-cosY * sinP) * wz;
                float rz = (sinY * cosP) * wx + sinP * wy + (cosY * cosP) * wz;
                return {rx * gizmoRadius, -ry * gizmoRadius, rz};
            };

            float pi = 3.14159265f;
            float halfPi = pi * 0.5f;

            auto [xx, xy, xz] = project(1, 0, 0);  // +X
            auto [yx, yy, yz] = project(0, 1, 0);  // +Y
            auto [zx, zy, zz] = project(0, 0, 1);  // +Z
            auto [nxx, nxy, nxz] = project(-1, 0, 0); // -X
            auto [nyx, nyy, nyz] = project(0, -1, 0); // -Y
            auto [nzx, nzy, nzz] = project(0, 0, -1); // -Z

            AxisInfo axes[] = {
                {xx, xy, xz,  IM_COL32(230, 70, 70, 255),  IM_COL32(150, 50, 50, 180), "X", "-X",  halfPi, 0, true},
                {nxx, nxy, nxz, IM_COL32(150, 50, 50, 180), IM_COL32(100, 40, 40, 140), "-X", "X", -halfPi, 0, false},
                {yx, yy, yz,  IM_COL32(70, 200, 70, 255),  IM_COL32(50, 130, 50, 180), "Y", "-Y",  0, halfPi, true},
                {nyx, nyy, nyz, IM_COL32(50, 130, 50, 180), IM_COL32(40, 90, 40, 140),  "-Y", "Y", 0, -halfPi * 0.98f, false},
                {zx, zy, zz,  IM_COL32(70, 100, 230, 255), IM_COL32(50, 70, 150, 180), "Z", "-Z",  0, 0, true},
                {nzx, nzy, nzz, IM_COL32(50, 70, 150, 180), IM_COL32(40, 50, 100, 140), "-Z", "Z", pi, 0, false},
            };

            // Sort by depth (draw back-to-front)
            int order[6] = {0, 1, 2, 3, 4, 5};
            std::sort(order, order + 6, [&](int a, int b) {
                return axes[a].depth < axes[b].depth;
            });

            // Draw axes and handle clicks
            bool gizmoClicked = false;

            for (int oi = 0; oi < 6; oi++) {
                int i = order[oi];
                AxisInfo& a = axes[i];
                ImVec2 tip(gCenter.x + a.sx, gCenter.y + a.sy);

                // Draw axis line
                float lineAlpha = (a.depth > 0) ? 1.0f : 0.4f;
                ImU32 lineCol = a.positive ? a.color : a.colorDim;

                dl->AddLine(gCenter, tip, lineCol, a.positive ? 2.5f : 1.5f);

                // Draw axis endpoint circle
                float dotRadius = a.positive ? 14.0f : 10.0f;
                bool hovered = false;

                ImVec2 mousePos = ImGui::GetMousePos();
                float dx = mousePos.x - tip.x;
                float dy = mousePos.y - tip.y;
                if (dx * dx + dy * dy < dotRadius * dotRadius) {
                    hovered = true;
                }

                ImU32 dotColor = hovered ? IM_COL32(255, 255, 255, 255) : (a.positive ? a.color : a.colorDim);
                dl->AddCircleFilled(tip, dotRadius, dotColor, 24);

                if (a.positive) {
                    // Draw label
                    const char* lbl = a.label;
                    ImVec2 textSize = ImGui::CalcTextSize(lbl);
                    dl->AddText(ImVec2(tip.x - textSize.x * 0.5f, tip.y - textSize.y * 0.5f),
                        IM_COL32(255, 255, 255, 255), lbl);
                }

                // Click to snap view (only if gizmo window is hovered)
                if (hovered && ImGui::IsMouseClicked(0) && !gizmoClicked && ImGui::IsWindowHovered()) {
                    camera.setYaw(a.dirYaw);
                    camera.setPitch(a.dirPitch);
                    gizmoClicked = true;
                }
            }

            // Drag on gizmo background to orbit
            ImVec2 mousePos = ImGui::GetMousePos();
            float distFromCenter = std::sqrt(
                (mousePos.x - gCenter.x) * (mousePos.x - gCenter.x) +
                (mousePos.y - gCenter.y) * (mousePos.y - gCenter.y));

            static bool gizmoDragging = false;
            static ImVec2 gizmoDragPrev = {0, 0};

            if (distFromCenter < gizmoRadius + 12 && ImGui::IsMouseClicked(0) && !gizmoClicked && ImGui::IsWindowHovered()) {
                gizmoDragging = true;
                gizmoDragPrev = mousePos;
            }
            if (!ImGui::IsMouseDown(0)) {
                gizmoDragging = false;
            }
            if (gizmoDragging) {
                float gdx = mousePos.x - gizmoDragPrev.x;
                float gdy = mousePos.y - gizmoDragPrev.y;
                gizmoDragPrev = mousePos;
                camera.orbit(gdx * 2.0f, gdy * 2.0f);
            }

            // Perspective / Ortho label
            dl->AddText(ImVec2(gCenter.x - 15, gCenter.y + gizmoRadius + 12),
                IM_COL32(150, 150, 160, 200), "Persp");

            ImGui::End();
        }

        // Shift controls panel down to make room for gizmo
        {
            int winW, winH;
            SDL_GetWindowSize(window, &winW, &winH);
            // The controls help text was positioned at top-right, move it below gizmo
            // (already repositioned by the gizmo taking the top-right spot)
        }

        // --- Render ---
        int fbW, fbH;
        SDL_GL_GetDrawableSize(window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);

        float aspect = static_cast<float>(fbW) / static_cast<float>(fbH);
        renderer.render(camera, aspect);

        // Edit mode overlay
        if (modeManager.isEdit()) {
            selRenderer.render(camera, aspect);
        }

        // Pose mode bone overlay
        if (modeManager.isPose() && hasArmature) {
            boneRenderer.render(camera, aspect);
        }

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(window);
    }

    // --- Cleanup ---
    boneRenderer.shutdown();
    selRenderer.shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    renderer.shutdown();
    SDL_GL_DeleteContext(glCtx);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << "SculptApp closed." << std::endl;
    return 0;
}
