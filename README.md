<p align="center">
  <img src="assets/logo.svg" alt="arvis" width="360">
</p>

<p align="center">
  A cross-platform (Windows / Linux / macOS) C++20 desktop HTTP client and request inspector with a native GUI.
</p>

<p align="center">
  <a href="https://github.com/Rennn0/arvis/actions/workflows/release.yml"><img src="https://img.shields.io/github/actions/workflow/status/Rennn0/arvis/release.yml?branch=main&label=build" alt="Build status"></a>
  <a href="https://github.com/Rennn0/arvis/releases/latest"><img src="https://img.shields.io/github/v/release/Rennn0/arvis?label=release&sort=semver" alt="Latest release"></a>
  <a href="https://github.com/Rennn0/arvis/releases"><img src="https://img.shields.io/github/downloads/Rennn0/arvis/total" alt="Downloads"></a>
  <a href="LICENSE"><img src="https://img.shields.io/github/license/Rennn0/arvis" alt="License"></a>
  <img src="https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white" alt="C++20">
  <img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-4c9bd4" alt="Platforms">
</p>

<p align="center">
  Built with <a href="https://curl.se/libcurl/">libcurl</a>, <a href="https://github.com/ocornut/imgui">Dear ImGui</a>, and <a href="https://www.glfw.org/">GLFW</a> + OpenGL3.
</p>

---

## Quick install

Each installer downloads the latest [prebuilt release binary](https://github.com/Rennn0/arvis/releases) for your platform (Linux x64, Windows x64, macOS arm64/x64) and installs it to `~/.local/bin` — no compiler or dependencies required. If no prebuilt binary matches, it falls back to building from source.

**Linux / macOS**

```bash
curl -fsSL https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.sh | sh
```

Pass flags through the pipe with `sh -s --`:

| Flag | Effect |
|------|--------|
| `--run` | Download/build and launch without installing |
| `--from-source` | Always build from source instead of downloading |
| `--prefix DIR` | Install the binary to `DIR` (default: `$HOME/.local/bin`) |
| `--install-deps` | Install system build dependencies (only needed for a source build) |

```bash
curl -fsSL https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.sh | sh -s -- --run
```

**Windows** (PowerShell)

```powershell
irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1 | iex
```

To pass flags (`-Run`, `-FromSource`, `-Prefix DIR`, `-Ref TAG`), download the script first:

```powershell
$s = irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1
& ([scriptblock]::Create($s)) -Run
```

> A source build (the fallback, or `--from-source` / `-FromSource`) needs the [build prerequisites](#1-prerequisites) below.

---

## Build from source

### 1. Prerequisites

- **CMake ≥ 3.24** and **Git**
- A **C++20 compiler** — MSVC (Visual Studio 2022, "Desktop development with C++" workload) on Windows; GCC or Clang on Linux/macOS
- **Ninja** on Linux (required by the Linux presets)

libcurl is provided by the bundled **vcpkg** submodule; GLFW and Dear ImGui are fetched by CMake at configure time. The first configure is slow because it builds these dependencies.

On Linux, install the toolchain and GLFW's system libraries:

```bash
sudo apt update && sudo apt install -y \
  build-essential cmake ninja-build git pkg-config \
  curl zip unzip tar \
  libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev
```

### 2. Clone

`external/vcpkg` is a submodule and must be initialized, or configure will fail:

```bash
git clone --recurse-submodules https://github.com/Rennn0/arvis.git
cd arvis
# If already cloned without submodules:
git submodule update --init --recursive
```

### 3. Configure and build

**Linux**

```bash
cmake --preset linux-debug          # or: linux-release
cmake --build --preset linux-debug
./build/linux-debug/arvis
```

**Windows**

```powershell
cmake --preset windows
cmake --build --preset windows-debug    # or: windows-release
.\build\windows\Debug\arvis.exe
```

---