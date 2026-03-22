cmake_minimum_required(VERSION 3.16)
project(SculptApp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(USE_OPENGL "Use OpenGL renderer" ON)

# Find packages
find_package(OpenGL REQUIRED)

# Try to find SDL2 via pkg-config or cmake
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(SDL2 QUIET sdl2)
endif()

if(NOT SDL2_FOUND)
    find_package(SDL2 QUIET)
endif()

# GLM - header only
find_path(GLM_INCLUDE_DIR glm/glm.hpp
    PATHS /usr/include /usr/local/include /opt/homebrew/include
)

# Source files
set(CORE_SOURCES
    src/core/HalfEdgeMesh.cpp
    src/core/BVH.cpp
    src/core/ObjLoader.cpp
    src/core/MeshGenerator.cpp
)

set(RENDER_SOURCES
    src/render/Shader.cpp
    src/render/Camera.cpp
    src/render/Renderer.cpp
)

set(INPUT_SOURCES
    src/input/InputManager.cpp
)

set(SCULPT_SOURCES
    src/sculpt/Brush.cpp
    src/sculpt/SculptEngine.cpp
    src/sculpt/Raycaster.cpp
)

# Core library (no OpenGL dependency - testable)
add_library(sculpt_core STATIC ${CORE_SOURCES} ${SCULPT_SOURCES})
target_include_directories(sculpt_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(GLM_INCLUDE_DIR)
    target_include_directories(sculpt_core PUBLIC ${GLM_INCLUDE_DIR})
endif()

# Full application (with OpenGL/SDL)
if(SDL2_FOUND OR SDL2_INCLUDE_DIRS)
    add_library(sculpt_render STATIC ${RENDER_SOURCES} ${INPUT_SOURCES})
    target_include_directories(sculpt_render PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
    )
    if(GLM_INCLUDE_DIR)
        target_include_directories(sculpt_render PUBLIC ${GLM_INCLUDE_DIR})
    endif()
    target_link_libraries(sculpt_render PUBLIC sculpt_core OpenGL::GL ${SDL2_LIBRARIES})

    add_executable(sculpt_app src/main.cpp)
    target_link_libraries(sculpt_app PRIVATE sculpt_render sculpt_core)
    target_include_directories(sculpt_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${SDL2_INCLUDE_DIRS}
    )
else()
    message(WARNING "SDL2 not found - building core library and tests only")
endif()

# Tests
if(BUILD_TESTS)
    enable_testing()

    # Simple test framework (no external dependency)
    add_executable(test_mesh tests/test_mesh.cpp)
    target_link_libraries(test_mesh PRIVATE sculpt_core)

    add_executable(test_bvh tests/test_bvh.cpp)
    target_link_libraries(test_bvh PRIVATE sculpt_core)

    add_executable(test_sculpt tests/test_sculpt.cpp)
    target_link_libraries(test_sculpt PRIVATE sculpt_core)

    add_executable(test_obj_loader tests/test_obj_loader.cpp)
    target_link_libraries(test_obj_loader PRIVATE sculpt_core)

    add_executable(test_camera tests/test_camera.cpp)
    target_include_directories(test_camera PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    if(GLM_INCLUDE_DIR)
        target_include_directories(test_camera PRIVATE ${GLM_INCLUDE_DIR})
    endif()

    add_executable(test_raycaster tests/test_raycaster.cpp)
    target_link_libraries(test_raycaster PRIVATE sculpt_core)

    add_test(NAME MeshTests COMMAND test_mesh)
    add_test(NAME BVHTests COMMAND test_bvh)
    add_test(NAME SculptTests COMMAND test_sculpt)
    add_test(NAME ObjLoaderTests COMMAND test_obj_loader)
    add_test(NAME CameraTests COMMAND test_camera)
    add_test(NAME RaycasterTests COMMAND test_raycaster)
endif()
