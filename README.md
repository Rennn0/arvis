<p align="center">
  <img src="assets/logo.svg" alt="arvis" width="360">
</p>

<p align="center">
  A cross-platform (Windows / Linux / macOS) C++20 feature rich desktop HTTP client and request inspector with a native GUI.
</p>
<p align="center">
  Focuses on <b>developer experience</b>, shortcuts, quick navigation and simplicity.
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
  Built with <a href="https://curl.se/libcurl/">libcurl</a>, <a href="https://github.com/ocornut/imgui">Dear ImGui</a>,  <a href="https://www.glfw.org/">GLFW</a> + OpenGL3 and <a href="https://github.com/SRombauts/SQLiteCpp">SQLiteCpp</a>.
</p>

---

## Small on purpose

arvis is a native binary — no Chromium, no Node runtime, no per-tab renderer
process. A real ~10-minute working session (open the app, edit requests, send,
browse JSON responses) holds **~105 MB of RAM in a single process**, inside a
1.6 MB band with no sawtooth and no creep, while burning **0.3 – 0.6 % CPU** —
and it drops to **0 % when idle**. The release binary is
**~13 MB** with no runtime to install beside it; the Windows build is statically
linked and doesn't even need the VC++ redistributable.

Postman and Insomnia are Electron apps: each one starts a full browser engine, a
Node runtime and a handful of helper processes before it draws a single request
row. That typically means several hundred MB to 1 GB+ of RAM across those
processes, a ~100–150 MB+ installer, seconds of cold start, and an account
prompt with cloud-synced workspaces. arvis is one process, one ~13 MB binary,
one local SQLite file and no login — nothing leaves your machine. It's the kind
of tool you keep open all day next to a compiler, a browser, a database GUI and
a few containers, without it being the reason you run out of RAM.

> Those Postman/Insomnia figures are the usual ballpark rather than a controlled
> benchmark — measure them yourself against the numbers above and the gap will
> still be an order of magnitude.

### Gallery

Want a look before installing? The images live in [`assets/`](assets):

| | |
|---|---|
| **The workspace** — request editor, `{{variable}}` autocomplete, response footer and the collapsible JSON tree viewer | [`assets/arv_workspace.png`](assets/arv_workspace.png) |
| **`Ctrl+F` search palette** — fuzzy search over the whole collection with `:t` `:m` `:u` `:c` `:s` `:b` field prefixes | [`assets/arv_search.png`](assets/arv_search.png) |
| **`Ctrl+Shift+P` settings** — environments and `{{name}}` variable substitution | [`assets/arv_settings.png`](assets/arv_settings.png) |

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

## Todos
 - response info (x)
 - reopen last modified req (x)
 - finder shortcut/window for requests (x)
 - req history tree
 - response jwon view (better)
 - - found matching only shows leaf -> need whole node
 - - collapse / fold needs to work recursively 
 - - maybe preview map on right edge instead of scrollbar?
 - environment system (inlines defined variable properties into requests)
 - - settings window {env} (x)
 - - settings window {general}
 - - settings window {shortcuts}
 - - settings window {network}
 - schemas for requests

## Bugs
- RAW option not working in viewer
