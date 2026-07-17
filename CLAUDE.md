# CLAUDE.md

Guidance for AI assistants (and humans) working in this repository. Read this
first — it captures the project's purpose, layout, build system, architecture, and
conventions so you can be productive immediately.

---

## 1. What this project is

**arvis** is a small, **cross-platform (Windows / Linux / macOS) C++20 desktop
application**: an HTTP client / request inspector with a native GUI.

- You type a URL, pick **GET** or **POST**, hit **Send**.
- The request runs **off the UI thread** (via `std::async`) so the window never
  freezes.
- The response body is shown in a scrollable panel and also saved to disk under
  `responses.dev/` (git-ignored), named `<sanitized-url><timestamp>.txt`.

The networking is built on **libcurl**; the GUI on **Dear ImGui** (immediate-mode)
with a **GLFW + OpenGL3** backend.

> Note: the project was previously named **maskj** and was renamed to **arvis**.
> The GitHub repo and the local `origin` remote are now `github.com/Rennn0/arvis`
> (the old `maskj` URL still works via GitHub's redirect). Do not reintroduce the
> `maskj` / `mj` names anywhere.

---

## 2. Tech stack & tooling

| Concern            | Choice                                                        |
|--------------------|---------------------------------------------------------------|
| Language           | C++20 (`CMAKE_CXX_STANDARD 20`, extensions OFF)               |
| Build system       | CMake ≥ 3.24, driven by **CMakePresets.json** (preset v3)     |
| Dependency manager | **vcpkg** (git submodule at `external/vcpkg`) for libcurl     |
| GUI deps           | GLFW + Dear ImGui, fetched at configure time via `FetchContent` |
| Windows generator  | Visual Studio 17 2022, x64 (MSVC)                             |
| Linux generator    | Ninja                                                         |
| Compiler warnings  | MSVC `/W4`, GCC/Clang `-Wall -Wextra -Wpedantic` (ImGui exempted) |

**Two dependency mechanisms, on purpose:**
- **libcurl** is a prebuilt package via **vcpkg** (declared in `vcpkg.json`,
  toolchain wired in the `base` preset). Keeps the generated solution clean.
- **GLFW (tag `3.4`)** and **Dear ImGui (tag `v1.91.5`)** are pulled by
  **`FetchContent`** at configure time and pinned to release tags — no manual
  install. ImGui ships no build system, so `CMakeLists.txt` compiles its core +
  the `imgui_impl_glfw` / `imgui_impl_opengl3` backends into a local static lib
  target named `imgui`.

---

## 3. Directory structure

```
arvis/
├── CLAUDE.md                     # this file
├── CMakeLists.txt                # single build script (explicit source list)
├── CMakePresets.json             # windows / linux-debug / linux-release presets
├── vcpkg.json                    # declares the 'curl' dependency
├── ui_foundation.md              # design guide for the UiComponent system (in progress)
│
├── include/                      # public headers, one folder per module
│   ├── av_net/network_manager.hpp
│   ├── av_ui/network_manager_ui.hpp
│   └── av_root/
│       ├── root.hpp              # AvRoot logging base
│       ├── im_scope.hpp          # RAII ImGui guards: ScopedId / ScopedStyle (header-only)
│       ├── ui_component.hpp      # UiComponent base (UI foundation, in progress)
│       ├── av_div.hpp            # AvDiv container (layout / splitters)
│       ├── av_button.hpp         # AvButton leaf
│       └── av_custom.hpp         # AvCustom callback leaf
│
├── src/                          # implementations — mirrors include/, one folder per module
│   ├── main.cpp                  # entry point -> constructs avUi::NetworkManagerUi, calls run()
│   ├── av_net/network_manager.cpp       # libcurl wrapper
│   ├── av_ui/network_manager_ui.cpp     # GLFW + ImGui window + event loop
│   └── av_root/
│       ├── root.cpp              # AvRoot logging impl
│       ├── ui_component.cpp      # UiComponent impl (in progress)
│       ├── av_div.cpp            # AvDiv impl
│       ├── av_button.cpp         # AvButton impl
│       └── av_custom.cpp         # AvCustom callback leaf
│
├── scripts/                      # dev tooling
│   ├── new_class.ps1             # class scaffolder (Windows / PowerShell)
│   └── new_class.sh              # class scaffolder (Linux / bash 4+)
│
├── external/vcpkg/               # vcpkg submodule — DO NOT edit or grep for project code
├── build/                        # generated; git-ignored (build/<presetName>/)
└── responses.dev/                # saved HTTP responses; git-ignored, created at runtime
```

`external/**` is third-party (thousands of unrelated files). **Always exclude it**
from project-wide searches (`grep ... --glob '!external/**'`).

---

## 4. Build & run

Configure (generates into `build/<presetName>/`), then build:

```bash
# Windows
cmake --preset windows
cmake --build --preset windows-debug        # or windows-release
# -> build/windows/Debug/arvis.exe

# Linux
cmake --preset linux-debug                   # or linux-release
cmake --build --preset linux-debug
# -> build/linux-debug/arvis
```

Presets are host-gated by a `condition`, so only the matching OS's presets are
offered. The `base` preset wires the vcpkg toolchain; libcurl auto-installs on first
configure.

**CRITICAL build-workflow rule:** after editing `CMakeLists.txt` (e.g. adding a
source file), you **must re-run the configure step** (`cmake --preset <name>`), not
just `cmake --build`. A plain build reuses stale project files and the new `.cpp`
won't be compiled → `LNK2019` unresolved-symbol errors. (This has bitten us; the
scaffolding scripts register sources for you, but you still reconfigure.)

On Windows, if a build fails with `LNK1168: cannot open arvis.exe for writing`, a
previous instance is still running — `taskkill //F //IM arvis.exe` then rebuild.

---

## 5. Architecture

Three modules, one namespace each. **Namespace casing is strict** — mismatches cause
link errors (also bitten by this):

| Module folder | Namespace | Responsibility                                    |
|---------------|-----------|---------------------------------------------------|
| `av_net`      | `avNet`   | Networking (libcurl wrapper)                      |
| `av_ui`       | `avUi`    | Application window + ImGui event loop             |
| `av_root`     | `avR`     | Foundation utilities (logging) + UI component base |

> Namespace derivation is regular for `av_net`→`avNet` and `av_ui`→`avUi`, but
> **`av_root`→`avR` is irregular** (not `avRoot`). Remember this when scaffolding.

### Key types

- **`avNet::NetworkManager`** (`network_manager.hpp/.cpp`)
  - Owns curl global init/cleanup (ctor/dtor).
  - `http_result get(const char*)` / `http_result post(const char*)`.
  - `http_result` = `{ response_status status; long http_code; std::string body;
    std::string saved_path; }`.
  - Private `fetch_core(method, url)` does the actual request: sets timeouts
    (`CONNECTTIMEOUT 15s`, `TIMEOUT 30s`, `NOSIGNAL`), follows redirects, writes the
    body to `responses.dev/`. **POST always sets an explicit empty body**
    (`POSTFIELDS ""`, `POSTFIELDSIZE 0`) — otherwise libcurl reads the body from
    stdin and hangs forever in a GUI app.

- **`avUi::NetworkManagerUi`** (`network_manager_ui.hpp/.cpp`)
  - `void run()` — creates the GLFW window + OpenGL context, initializes ImGui, builds
    the retained `UiComponent` tree once, and runs the render/event loop until the
    window closes. `main` constructs it on the stack and calls `run()`.
  - **Deliberately NOT a `UiComponent`.** It is the host/driver of the component tree,
    not a node within it — making it a `UiComponent` would break the invariant that
    `draw()` renders one frame inside a parent's ID scope (see below).
  - Holds an `avR::AvRoot avRoot` logger by composition (constructed in the initializer
    list).
  - Requests are dispatched on a worker thread via `std::async`; the loop polls the
    `std::future` each frame and shows results when ready. An in-flight request is
    awaited before shutdown so the worker never outlives the `NetworkManager`.
  - GLFW C callbacks (e.g. key handler) can't capture, so instance access uses
    `glfwSetWindowUserPointer(window, this)` + `glfwGetWindowUserPointer` inside a
    captureless lambda.

- **`avR::AvRoot`** (`root.hpp/.cpp`)
  - Lightweight logger. Ctor takes a name; `log_info` / `log_error` print
    `[level](name)<msg>` (info→`std::cout`, error→`std::cerr`) via a private
    `log_core`. **Always used by composition** (`AvRoot root{"Name"};`), never
    inheritance — both `UiComponent` and `NetworkManagerUi` hold one as a member.

- **`avR::ScopedId` / `avR::ScopedStyle`** (`im_scope.hpp`, header-only)
  - RAII guards over ImGui's push/pop stacks. `ScopedId` wraps `PushID`/`PopID`;
    `ScopedStyle` has chainable `.color(idx, v)` / `.var(idx, v)` calls and pops
    exactly what it pushed in its destructor. **Use these instead of hand-counting
    `PopStyleColor(n)` / `PopStyleVar(n)`** — that manual counting was the source of
    unbalanced-stack and ID-collision bugs. Colors and vars are independent stacks,
    so interleaving them on one guard is fine.

- **`avR::UiComponent`** (`ui_component.hpp/.cpp`) — **in progress.**
  - Base of a retained UI-component tree rendered onto immediate-mode ImGui. Uses the
    **template-method pattern**: `draw()` is **non-virtual** — it opens a `ScopedId(this)`
    scope and dispatches to a protected pure-virtual `render()`. Subclasses implement
    `render()`, never `draw()`, so the "every node draws inside its own ID scope"
    invariant holds for the whole tree automatically.
  - Interface: `draw()` (non-virtual entry), `render()` (protected pure virtual),
    `add_child`, optional `set_on_click`, `set_layout_size` / `preferred_size` (layout
    negotiation). Concrete components today: `AvDiv`, `AvButton`, `AvCustom`; planned:
    `Input`, `Select`, `GridLayout`, lists.
  - Holds its `AvRoot` logger by composition; subclasses log via the protected
    `log_info` / `log_error` forwarders (the logger member is declared before `id` so
    the ctor can copy the id into the logger name before moving it into `id`).
  - **Adding a new component:** subclass `UiComponent`, implement `render()`, declare
    any style pushes on a local `ScopedStyle`. That's the whole contract.
  - **Full design rationale lives in `ui_foundation.md`** — read it before extending
    the UI. Key points: Composite pattern, `unique_ptr` child ownership, **virtual
    destructor is mandatory**, build the tree once (not per frame), ID scoping handled
    by the base `draw()`, data binding via pointers/callbacks, and the goal of reducing
    the event loop to `root->draw();`.

### The immediate-mode model (important mental model)

ImGui is immediate-mode: widgets aren't objects; you re-issue draw calls every frame
and a widget "returns true" on the frame it's interacted with. The `UiComponent` tree
is the **retained** layer that holds state/config; each frame `draw()` translates it
into ImGui calls and fires callbacks inline. Do not rebuild the tree every frame.

---

## 6. Coding conventions

- **Files:** one class per header/source. Header `include/<module>/<snake_case>.hpp`,
  source `src/<module>/<snake_case>.cpp` — `src/` mirrors `include/` with the same
  per-module folders (the sole exception is `main.cpp`, which stays at `src/`'s top
  level). File base name is the class name converted PascalCase→snake_case
  (`NetworkManagerUi` → `network_manager_ui`).
- **Namespaces:** `av` + module, as in §5. Match casing exactly everywhere
  (declaration and definition) or you get unresolved-symbol link errors.
- **Includes:** angle-bracket module paths, e.g. `#include <av_net/network_manager.hpp>`
  (works because `include/` is on the target's include path).
- **`#pragma once`** in every header.
- **Member variables:** no `m_` prefix — name the member plainly (`config`, `label`,
  `root`) and **always refer to it through `this->`** inside methods (`this->config`),
  which also disambiguates it from same-named ctor params (`config(config)` in the
  init list works, `this->config = config;` in the body works). Corollary: a data
  member and a member function can't share a name, so don't pair a `foo` member with a
  `foo()` accessor — use a distinct verb (`get_children()`, `configure()`) or drop the
  accessor.
- **Constructors:** initialize members in the **initializer list**, not by assignment
  in the body. Never assign `nullptr` to a `std::string`.
- **Polymorphic bases:** always `virtual ~T() = default;`.
- **Do not mark out-of-line (`.cpp`) function definitions `inline`** — it risks
  IFNDR/link errors and gives no inlining benefit. If you want inlining, define the
  body in the header instead.
- **Prefer `std::string_view`** for read-only string params, `std::string` (moved)
  for owned ones.
- **CMake source list is explicit** (not globbed) — a deliberate choice to keep the
  generated solution clean. Add new `.cpp` files to the `add_executable(arvis ...)`
  list (the scaffolding scripts do this automatically).

---

## 7. Scaffolding scripts

Generate a new class (header + source + CMake registration) instead of hand-writing:

```powershell
# Windows (PowerShell)
scripts/new_class.ps1 <module> <ClassName> [-Namespace ns] [-FileName name] [-Force]
scripts/new_class.ps1 av_net Downloader                 # namespace avNet, downloader.*
scripts/new_class.ps1 av_root UiElement -Namespace avR  # avR is irregular -> override
```

```bash
# Linux (bash 4+)
scripts/new_class.sh <module> <ClassName> [--namespace ns] [--filename name] [--force]
scripts/new_class.sh av_net Downloader
scripts/new_class.sh av_root UiElement --namespace avR
```

Both scripts:
1. create `include/<module>/<file>.hpp` (with `#pragma once`, namespace, class skeleton),
2. create `src/<module>/<file>.cpp` (with the matching `#include` + ctor/dtor stubs),
3. insert `src/<module>/<file>.cpp` into the `add_executable(arvis ...)` list in `CMakeLists.txt`,
4. insert `include/<module>/<file>.hpp` into the `target_sources(arvis PRIVATE ...)` list — so the
   header shows up (and is editable) in the generated Visual Studio / IDE project, not just the source.

Both CMake insertions are idempotent (re-running skips entries already present) and each warns without
failing if its block is missing.

They derive the namespace from the module (`av_net`→`avNet`) and the filename from the
class (PascalCase→snake_case); override with `-Namespace`/`--namespace` for the
irregular `av_root`→`avR`, or `-FileName`/`--filename` for acronym-heavy names. They
won't overwrite existing files without `-Force`/`--force`, and the CMake insertion is
idempotent. **After running, reconfigure** (`cmake --preset <name>`).

The `.ps1` also runs on Linux under `pwsh`; the `.sh` needs bash 4+ (Linux standard;
macOS ships bash 3.2, so `brew install bash` there).

---

## 8. Common pitfalls (all previously hit in this project)

1. **Edited `CMakeLists.txt` but only ran `cmake --build`** → stale project, new
   sources not compiled, `LNK2019`. Fix: rerun `cmake --preset <name>`.
2. **Namespace casing mismatch** (e.g. `avUi` vs `avUI`) → unresolved externals at
   link time. The compile succeeds per-file; only linking fails.
3. **`inline` on a `.cpp`-only definition used from another TU** → possible
   unresolved-symbol error; no benefit. Keep bodies out-of-line *without* `inline`,
   or move them to the header.
4. **POST with no body** → libcurl reads stdin and hangs the worker thread forever.
   Always set an empty POST body (already handled in `fetch_core`).
5. **No request timeout** → a dead endpoint hangs the UI's "sending…" state. Timeouts
   are set in `fetch_core`; keep them.
6. **Rebuild fails with `LNK1168`** on Windows → the app is still running; kill it.
7. **ImGui ID collisions** (duplicate labels / list rows) → wrong widget gets the
   click. Inside the `UiComponent` tree this is handled for you (base `draw()` opens a
   `ScopedId(this)`); for ad-hoc rows use `avR::ScopedId(index)` (or raw `PushID`).
   Prefer `avR::ScopedStyle` over manual `PushStyleColor`/`Var` + hand-counted pops.
8. **Searching the whole tree** picks up `external/vcpkg` noise → always exclude
   `external/**`.

---

## 9. Git

- Branch: `main`. Remote: `origin` → `github.com/Rennn0/arvis.git`.
- `build/` and `responses.dev/` are git-ignored.
- Only commit when explicitly asked.

---