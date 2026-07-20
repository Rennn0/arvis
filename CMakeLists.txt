cmake_minimum_required(VERSION 3.24)
project(arvis
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The Windows build uses the x64-windows-static vcpkg triplet so libcurl (and
# its deps) are baked into the binary. The MSVC C runtime must then be linked
# statically too, or the released .exe would still need the VC++ redistributable
# and would mismatch the /MT-built vcpkg libs at link time. CMP0091 (NEW since
# CMake 3.15) makes this variable the single source of truth for /MT vs /MD, and
# it applies to every target below (arvis, imgui, glfw) so they stay consistent.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Use a prebuilt libcurl (install via vcpkg or system package manager).
# This keeps the generated solution clean — only the arvis project appears,
# with no third-party curl subprojects.
find_package(CURL REQUIRED)

# OpenGL is the cross-platform rendering backend for the GUI (provided by the OS).
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# GUI dependencies — cross-platform (Windows / Linux / macOS).
#
# Both GLFW (windowing/input) and Dear ImGui (immediate-mode UI) are fetched
# and built by CMake at configure time, each pinned to a release tag. There is
# no manual install step and no system package to hunt for; the only OS-level
# requirement is an OpenGL driver (always present) plus, on Linux, the usual
# X11/Wayland development headers that GLFW itself depends on.
# ---------------------------------------------------------------------------
include(FetchContent)

# --- GLFW --------------------------------------------------------------------
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4          # pinned for reproducible builds
    GIT_SHALLOW    TRUE
)
# Build only the library — skip GLFW's own examples/tests/docs.
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- Dear ImGui --------------------------------------------------------------
# ImGui ships no build system of its own, so we compile its core plus the
# GLFW + OpenGL3 backends into a small static library ourselves.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.5      # pinned for reproducible builds
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

# --- Sqlite --------------------------------------------------------------
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.2
)
FetchContent_MakeAvailable(SQLiteCpp)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# The OpenGL3 backend ships its own GL loader, so ImGui only needs GLFW + GL.
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

add_executable(arvis
    src/main.cpp
    src/av_net/network_manager.cpp
    src/av_root/root.cpp
    src/av_root/av_state.cpp
    src/av_root/ui_component.cpp
    src/av_root/ui_scoped_style.cpp
    src/av_ui/network_manager_ui.cpp
    src/av_ui/root_ui.cpp
    src/av_ui/request_list_view_ui.cpp
    src/av_ui/detailed_request_view_ui.cpp
    src/av_root/av_inter_view_shared_state.cpp
    src/av_root/av_request_list_state.cpp
    src/av_s/av_storage.cpp
    src/av_s/av_request_storage.cpp
)

target_include_directories(arvis PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(arvis PRIVATE CURL::libcurl imgui SQLiteCpp)

get_target_property(arvis_all_sources arvis SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${arvis_all_sources})

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT arvis)

if(MSVC)
    target_compile_options(arvis PRIVATE /W4)
    target_compile_options(imgui PRIVATE /W1)
else()
    target_compile_options(arvis PRIVATE -Wall -Wextra -Wpedantic)
endif()
