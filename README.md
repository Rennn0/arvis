# arvis

A cross-platform (Windows / Linux / macOS) C++20 desktop HTTP client and request inspector with a native GUI.

Built with [libcurl](https://curl.se/libcurl/), [Dear ImGui](https://github.com/ocornut/imgui), and [GLFW](https://www.glfw.org/) + OpenGL3.

---

## Quick install

Each installer clones the repo, builds a Release binary from source, and installs it to `~/.local/bin` (override with the flags below).

**Linux / macOS**

```bash
curl -fsSL https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.sh | sh
```

Pass flags through the pipe with `sh -s --`:

| Flag | Effect |
|------|--------|
| `--run` | Build and launch without installing |
| `--install-deps` | Install system build dependencies via the detected package manager |
| `--prefix DIR` | Install the binary to `DIR` (default: `$HOME/.local/bin`) |

```bash
curl -fsSL https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.sh | sh -s -- --install-deps --run
```

**Windows** (PowerShell) — requires Visual Studio 2022 with the "Desktop development with C++" workload:

```powershell
irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1 | iex
```

To pass flags (`-Run`, `-Prefix DIR`, `-Ref BRANCH`), download the script first:

```powershell
$s = irm https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.ps1
& ([scriptblock]::Create($s)) -Run
```

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