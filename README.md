# arvis

A cross-platform (Windows / Linux / macOS) C++20 desktop HTTP client and request inspector with a native GUI.

Built with [libcurl](https://curl.se/libcurl/), [Dear ImGui](https://github.com/ocornut/imgui), and [GLFW](https://www.glfw.org/) + OpenGL3.

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

## Releases

Prebuilt binaries are published to [GitHub Releases](https://github.com/Rennn0/arvis/releases) by the [`release`](.github/workflows/release.yml) GitHub Actions workflow, which builds Linux, Windows, and macOS (arm64 + x64) binaries and attaches them to a tagged release.