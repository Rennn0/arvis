# CLAUDE.md

Guidance for AI assistants (and humans) working in this repository. Read this
first — it captures the project's purpose, layout, build system, architecture, and
conventions so you can be productive immediately.

---

## 0. HARD RULE — never edit code, write a proposal instead

**The owner of this repo writes all the code. AI assistants do not.**

Whenever a request would require *any* change to the source tree — new files,
edits, deletions, renames, `CMakeLists.txt` entries, config tweaks — **do not
touch a single file.** Instead:

1. Write the complete proposal to a markdown file under **`notes.dev/`**
   (git-ignored via the existing `*.dev*` rule), named after the task, e.g.
   `notes.dev/cookie_storage_plan.md`.
2. In that file, give the **full** intended content — whole functions, whole
   headers, exact diffs, exact `CMakeLists.txt` lines — not a summary. The owner
   copies from it and types the real change themselves, so anything left vague is
   work they have to redo.
3. Reply in chat with the file path and a short summary. Nothing else.

Applies to:

- All of `include/`, `src/`, `scripts/`, `assets/`, `.github/`, `CMakeLists.txt`,
  `CMakePresets.json`, `vcpkg.json`, `.clang-format`, `.gitignore`, and any other
  tracked file.
- Scaffolding scripts (`scripts/new_class.*`) — **do not run them**, they create
  files and edit `CMakeLists.txt`. Write out what they *would* generate instead.
- Refactors, bug fixes, and one-line changes alike. "It's tiny" is not an
  exception.
- Git-mutating commands: no `commit`, `add`, `checkout`, `restore`, `stash`,
  `rm`, `mv`.

Still allowed without asking:

- Reading, searching, and `git` inspection (`status`, `diff`, `log`, `show`).
- Configuring, building, and running the app to reproduce or verify behaviour.
- Writing/updating files inside `notes.dev/` (and other `*.dev*` paths).
- Editing this `CLAUDE.md` when explicitly asked to record a rule.

If a request seems to *require* editing to be useful, it doesn't — write the
proposal. The only way this rule is bypassed is the owner saying so explicitly,
for that one request.

---

## 1. HARD RULE — prefer Boost for new code

**Boost is a first-class dependency here. When new code needs a utility that
Boost provides, use Boost rather than hand-rolling it or reaching for a
hand-written loop.**

This applies to proposals too: a plan written into `notes.dev/` must already use
Boost where Boost fits — don't propose a manual `std::string::find` / `substr`
parser and leave the Boost version as a footnote.

Where it matters most today:

| Need | Use | Not |
|------|-----|-----|
| trim / case-insensitive compare / starts-with / split / replace / join | `boost/algorithm/string.hpp` (`boost::trim`, `boost::iequals`, `boost::istarts_with`, `boost::split`, `boost::replace_all`) | hand-written `find`/`substr` loops, `std::transform(::tolower)` |
| substring / delimiter search returning a range | `boost::algorithm::find_first` / `find_last` (`boost/algorithm/string/find.hpp`) | manual index arithmetic around `npos` |
| clamping | `boost::algorithm::clamp` | hand-written `min(max(...))` |
| any_of / all_of over a range with a classifier | `boost/algorithm/cxx11/any_of.hpp` + `boost::is_any_of`, `boost::is_space` | raw loops |
| filter/transform pipelines into a container | Boost.Range adaptors (`boost::adaptors::filtered`, `::transformed`) + `boost::push_back` | manual push_back loops |
| small containers that usually fit on the stack | `boost::container::small_vector<T, N>` | `std::vector` when N is known and small |

Existing precedent to follow — read these before writing new string/range code:

- `src/av_ui/input_autocomplete_ui.cpp` — the heaviest Boost user (algorithm,
  range adaptors, `small_vector`).
- `src/av_net/network_manager.cpp` — header/cookie parsing built on
  `boost::algorithm::find_first`, `trim`, `iequals`, `starts_with`.
- `src/av_ui/search_view_ui.cpp` — query tokenizing via `boost::split` + `trim`.

Rules of engagement:

- **Header-only Boost only.** `CMakeLists.txt` links `Boost::headers`; nothing
  links a compiled Boost library. If a feature needs a compiled Boost component
  (Boost.Filesystem, Boost.Thread, …), do **not** silently add it — call it out
  in the proposal, since it changes `vcpkg.json`, the link line, and the static
  Windows build.
- **Adding a Boost sub-library means editing `vcpkg.json`.** Today only
  `boost-algorithm` and `boost-container` are declared. A new `boost/…` include
  from a different sub-library needs its vcpkg port added *and* a reconfigure
  (`cmake --preset <name>`) — flag both in the proposal.
- **Include the narrow header** (`boost/algorithm/string/trim.hpp`) over the
  umbrella `boost/algorithm/string.hpp` when only one facility is used; both
  patterns exist in the tree, narrow is preferred for new code.
- **Don't replace working `std` code with Boost for its own sake.** The rule is
  about *new* code (and code you're already rewriting). `std::string_view`,
  `std::optional`, `std::filesystem`, `std::async`/`std::future`, and
  `std::unique_ptr`/`shared_ptr` stay as they are — Boost is not a substitute for
  the standard library where the standard library is already idiomatic here.

---

## 2. What this project is

**arvis** is a **cross-platform (Windows / Linux / macOS) C++20 desktop
application**: an HTTP client / request inspector with a native GUI, in the
Postman/Insomnia family but keyboard-first and deliberately small.

What it does today:

- **Request collection.** You create, rename, reorder (drag & drop), duplicate and
  delete named requests. Everything is persisted in a local **SQLite** database, so
  the collection survives restarts.
- **Full request editing.** Method (GET/POST/PUT/PATCH/DELETE/HEAD/OPTIONS), URL,
  query params, headers, cookies and body — each row individually includable /
  excludable, with a description column.
- **Sending.** The request runs **off the UI thread** (`std::async`) so the window
  never freezes; the loop polls the `std::future` each frame.
- **Response inspection.** Status code, elapsed time, response headers, response
  cookies, and the body — rendered as a raw dump, a pretty-printed dump, or an
  interactive collapsible **JSON tree** with per-node copy, filtering and stats.
  Bodies are also written to disk (see §5).
- **Environment variables.** `{{name}}` placeholders in URLs, param values, header
  values and cookie values are resolved at send time from the active environment.
  Inputs offer inline autocomplete when you type `{{`.
- **Search palette.** `Ctrl+F` opens a fuzzy command-palette over the request list
  with field prefixes (`:t` / `:title`, …).
- **Global shortcuts.** New / send / save / rename / search / settings / shortcut
  cheat-sheet / ImGui style editor, all registered in one place.
- **Settings window** — `Ctrl+Shift+P` (in progress; see §6 `AvAppSettings`).

The networking is built on **libcurl**; the GUI on **Dear ImGui** (immediate-mode)
with a **GLFW + OpenGL3** backend; persistence on **SQLiteCpp**; JSON on
**nlohmann/json**; string and range work on **Boost** (§1).

> Note: the project was previously named **maskj** and was renamed to **arvis**.
> The GitHub repo and the local `origin` remote are now `github.com/Rennn0/arvis`
> (the old `maskj` URL still works via GitHub's redirect). Do not reintroduce the
> `maskj` / `mj` names anywhere.

---

## 3. Tech stack & tooling

| Concern            | Choice                                                        |
|--------------------|---------------------------------------------------------------|
| Language           | C++20 (`CMAKE_CXX_STANDARD 20`, extensions OFF)               |
| Build system       | CMake ≥ 3.24, driven by **CMakePresets.json** (preset v3)     |
| Dependency manager | **vcpkg** (git submodule at `external/vcpkg`)                 |
| vcpkg packages     | `curl`, `nlohmann-json`, `boost-algorithm`, `boost-container` |
| FetchContent deps  | GLFW `3.4`, Dear ImGui `v1.91.5`, SQLiteCpp `3.3.2`           |
| Persistence        | SQLite via **SQLiteCpp**, one file DB in the OS app-data dir  |
| JSON               | **nlohmann/json** (header-only, vcpkg)                        |
| Windows generator  | Visual Studio 17 2022, x64 (MSVC), triplet `x64-windows-static` |
| Linux generator    | Ninja, triplet `x64-linux`                                    |
| Compiler warnings  | MSVC `/W4`, GCC/Clang `-Wall -Wextra -Wpedantic` (ImGui at `/W1`) |
| Formatting         | `.clang-format` (LLVM base, Allman braces, 4 spaces, 120 cols) |
| CI                 | `.github/workflows/release.yml` — builds + publishes binaries |

**Two dependency mechanisms, on purpose:**

- **libcurl, nlohmann/json and Boost (headers)** come from **vcpkg** (declared in
  `vcpkg.json`, toolchain wired in the `base` preset). Keeps the generated
  solution clean — no third-party subprojects in the IDE.
- **GLFW, Dear ImGui and SQLiteCpp** are pulled by **`FetchContent`** at configure
  time and pinned to release tags — no manual install. ImGui ships no build
  system, so `CMakeLists.txt` compiles its core + the `imgui_impl_glfw` /
  `imgui_impl_opengl3` backends + `misc/cpp/imgui_stdlib.cpp` (the
  `std::string` InputText overloads) into a local static lib target named `imgui`.

**Windows static build.** The Windows preset uses the `x64-windows-static` triplet
so libcurl and friends are baked into the `.exe`. `CMAKE_MSVC_RUNTIME_LIBRARY` is
therefore set to `MultiThreaded[Debug]` (CMP0091) for *every* target — arvis, imgui
and glfw — so the released binary needs no VC++ redistributable and doesn't
mismatch the `/MT`-built vcpkg libs at link time.

**MSVC quirks handled centrally.** `NOMINMAX` and `WIN32_LEAN_AND_MEAN` are
defined on the `arvis` target (curl pulls in `<windows.h>`, whose `min`/`max`
macros otherwise clobber `std::min`/`std::max`), so no individual `.cpp` needs its
own guard.

**SQLiteCpp quirk handled centrally.** SQLiteCpp runs cpplint/cppcheck over its own
sources by default, which fails on a CRLF Windows checkout. The four
`SQLITECPP_RUN_*` / `SQLITECPP_BUILD_*` cache variables are forced OFF before
`FetchContent_MakeAvailable` — same pattern as GLFW's examples/tests/docs.

---

## 4. Directory structure

```
arvis/
├── CLAUDE.md                     # this file
├── README.md                     # user-facing: install scripts, build from source, todos
├── CMakeLists.txt                # single build script (explicit source list)
├── CMakePresets.json             # windows / linux-debug / linux-release presets
├── vcpkg.json                    # curl, nlohmann-json, boost-algorithm, boost-container
├── .clang-format                 # project style (see §8)
│
├── include/                      # public headers, one folder per module
│   ├── av_net/
│   │   └── network_manager.hpp   # libcurl wrapper; http_request / http_result / enums
│   ├── av_root/                  # foundation: logging, state models, UI base
│   │   ├── root.hpp              # AvRoot — logger + timestamp helpers
│   │   ├── ui_component.hpp      # UiComponent base of the retained UI tree
│   │   ├── ui_scoped_style.hpp   # UiScopedStyle — RAII ImGui style guard
│   │   ├── av_state.hpp          # AvState — empty polymorphic base for shared state
│   │   ├── av_request.hpp        # data models: AvRequest / Param / Header / Cookie / Environment
│   │   ├── av_request_list_state.hpp    # AvRequestListState — the request collection + active env
│   │   ├── av_inter_view_shared_state.hpp # AvInterViewSharedState — cross-view bus
│   │   └── av_app_settings.hpp   # AvAppSettings — app settings model (stub, in progress)
│   ├── av_s/                     # storage layer (SQLite)
│   │   ├── av_storage.hpp        # AvStorage — DB handle, path, PRAGMAs, schema version
│   │   ├── av_request_storage.hpp        # requests table + migrations
│   │   ├── av_request_params_storage.hpp # request_params table
│   │   ├── av_request_headers_storage.hpp# request_headers table
│   │   ├── av_request_cookies_storage.hpp# request_cookies table
│   │   └── av_environment_storage.hpp    # environments + environment_variables tables
│   └── av_ui/
│       ├── network_manager_ui.hpp    # app host: GLFW window + ImGui context + loop
│       ├── root_ui.hpp               # RootUi — root of the component tree, fonts, style, shortcuts
│       ├── request_list_view_ui.hpp  # left panel: the request collection
│       ├── detailed_request_view_ui.hpp # main panel: edit + send + response
│       ├── search_view_ui.hpp        # Ctrl+F command palette
│       ├── settings_view_ui.hpp      # Ctrl+Shift+P settings window (in progress)
│       ├── json_tree_view.hpp        # JSON viewer widget (NOT a UiComponent)
│       ├── input_autocomplete_ui.hpp # {{var}} autocomplete InputText + resolve_vars
│       ├── ui_shortcut.hpp           # UiShortcut — predicate/callback keybinding tree
│       └── logo_icon.hpp             # AUTO-GENERATED window-icon pixels — do not hand-edit
│
├── src/                          # implementations — mirrors include/ exactly
│   ├── main.cpp                  # entry point -> avUi::NetworkManagerUi::run()
│   ├── av_net/…  av_root/…  av_s/…  av_ui/…
│   └── av_ui/fonts/              # AUTO-GENERATED compressed TTFs — do not hand-edit
│       ├── cousine_regular.h
│       ├── roboto_medium.h
│       └── noto_sans_georgian.h  # merged in for Georgian glyph coverage
│
├── assets/                       # icon/logo SVG+PNG, source TTFs, binary_to_compressed_c
├── scripts/                      # dev tooling — see §9
├── .github/workflows/release.yml # release build/publish pipeline
├── external/vcpkg/               # vcpkg submodule — DO NOT edit or grep for project code
├── build/                        # generated; git-ignored (build/<presetName>/)
└── notes.dev/                    # AI proposals per §0; git-ignored
```

`external/**` and `vcpkg_installed/**` are third-party (thousands of unrelated
files). **Always exclude them** from project-wide searches
(`grep ... --glob '!external/**' --glob '!vcpkg_installed/**'`).

> Files that no longer exist despite older references: `ui_foundation.md`,
> `include/av_root/im_scope.hpp` (`ScopedId`/`ScopedStyle`), `av_div.*`,
> `av_button.*`, `av_custom.*`. The generic-container experiment was dropped in
> favour of concrete view components (§6).

---

## 5. Runtime data locations

Nothing is written next to the binary. `avS::get_save_dir()`
(`src/av_s/av_storage.cpp`) picks the platform app-data directory and appends
`arvis/`:

| Platform | Base | Resulting dir |
|----------|------|---------------|
| Windows  | `%LOCALAPPDATA%` | `…\AppData\Local\arvis\` |
| macOS    | `$HOME/Library/Application Support` | `…/Library/Application Support/arvis/` |
| Linux    | `$XDG_DATA_HOME`, else `$HOME/.local/share` | `~/.local/share/arvis/` |

Inside it:

- **`_av_.db`** — the SQLite database (all requests, params, headers, cookies,
  environments). Opened with `foreign_keys=ON`, `journal_mode=WAL`,
  `busy_timeout=3000`.
- **`responses.dev/`** — every response body is also dumped here as
  `<sanitized-url><timestamp>.txt`. `NetworkManager` is constructed with the DB
  directory (`network_manager(this->request_storage->get_db_path())`) and appends
  `responses.dev`.

The repo-root `responses.dev/` and `imgui.ini` are leftovers from older runs; both
are git-ignored (`*.dev*`, `*.ini`), as is `*.db`.

---

## 6. Architecture

Four modules, one namespace each. **Namespace casing is strict** — mismatches cause
link errors (we have been bitten by this):

| Module folder | Namespace | Responsibility                                       |
|---------------|-----------|------------------------------------------------------|
| `av_net`      | `avNet`   | Networking (libcurl wrapper)                         |
| `av_ui`       | `avUi`    | Application host, component tree, views, widgets     |
| `av_root`     | `avR`     | Foundation: logging, data models, state, UI base     |
| `av_s`        | `avS`     | Storage layer (SQLite tables + migrations)           |

> Namespace derivation is regular for `av_net`→`avNet`, `av_ui`→`avUi` and
> `av_s`→`avS`, but **`av_root`→`avR` is irregular** (not `avRoot`). Remember this
> when scaffolding.

Dependency direction: `av_ui` → (`av_s`, `av_net`, `av_root`); `av_s` → `av_root`;
`av_net` → `av_root`; `av_root` depends on nothing of ours except ImGui/curl types
it exposes. Don't introduce a back-edge (`av_root` must never include `av_ui`).

### 6.1 Foundation — `av_root` / `avR`

- **`AvRoot`** (`root.hpp/.cpp`) — lightweight logger plus small time helpers.
  Ctor takes a name; `log_info` / `log_error` print `[level](name)<msg>`
  (info→`std::cout`, error→`std::cerr`) via a private `log_core`. Also exposes
  `get_timestamp()`, `timestamp_to_date(ts)` and `is_today(ts)` — used by the
  request list to group entries by day. **Always used by composition**
  (`AvRoot root{"Name"};`), never inheritance.

- **`UiComponent`** (`ui_component.hpp/.cpp`) — base of the retained UI tree
  rendered onto immediate-mode ImGui. **Template-method pattern**: `draw()` opens
  this node's ImGui ID scope (`PushID(id)`), constructs a `UiScopedStyle` from the
  protected `style` member, calls the protected pure-virtual `render()`, then
  pops. Subclasses implement `render()`, never `draw()`, so the "every node draws
  inside its own ID scope" invariant holds tree-wide for free.
  - Interface: `draw()`, `render()` (protected, pure virtual), `add_child`,
    `set_on_click` / `fire_click`, `draw_children`, `get_children`, `get_id`.
  - Non-copyable (copy ctor and assignment are `= delete`); **`virtual
    ~UiComponent() = default`** is mandatory.
  - Shared UI helpers live here so every view colours things identically:
    `get_method_color(request_method)`, `get_status_color(int code)` and the
    `environment_color` constant.
  - Holds an `AvRoot` by composition; subclasses log through the protected
    `log_info` / `log_error` forwarders.
  - **Adding a new component:** subclass `UiComponent`, implement `render()`, set
    `this->style` if the node wants custom ImGui style, done.

- **`UiScopedStyle`** (`ui_scoped_style.hpp/.cpp`) — RAII guard over ImGui's style
  stack. Takes a `UiScopedStyle::Style` of `std::optional` fields
  (`window_rounding`, `window_border_size`, `window_padding`, `frame_rounding`,
  `frame_padding`, `frame_border`), pushes only the engaged ones, and pops exactly
  that many in its destructor. **Use it instead of hand-counting
  `PopStyleVar(n)`** — manual counting was the source of unbalanced-stack bugs.

- **`AvState`** (`av_state.hpp`) — empty polymorphic base. Its only job is to let
  views take a single `avR::AvState *sharedState` ctor parameter and downcast, so
  view constructors have a uniform signature.

- **Data models** (`av_request.hpp`) — plain structs, no behaviour:
  - `AvRequestParam` — `{ included, editing, set_focus, id, request_id, order_by,
    key, value, description }`. The three bools are *UI* state kept next to the
    data on purpose (inline row editing, focus stealing after "add row").
  - `AvRequestHeader` / `AvRequestCookie` — empty subclasses of `AvRequestParam`,
    distinct types so the storage classes and tabs can't be mixed up.
  - `AvRequest` — `{ id, timestamp, order_by, method, url, params, headers,
    cookies, body?, title?, status_code?, collection? }` plus the in-memory-only
    `last_request` / `last_result` snapshot and a `pending_save` dirty flag.
    `display_name()` returns `title` or `request#<id>`.
  - `AvEnvironment` / `AvEnvironmentVariable` — a named bag of key/value pairs.

- **`AvRequestListState`** (`av_request_list_state.hpp`) — owns
  `std::vector<std::shared_ptr<AvRequest>>` and the active
  `std::shared_ptr<AvEnvironment>`. `shared_ptr` per request matters: the detailed
  view and the search palette hold on to a selection while the list vector can
  reallocate.

- **`AvInterViewSharedState`** (`av_inter_view_shared_state.hpp`) — the bus every
  view is wired through. Contains the per-view visibility flags
  (`show_req_list_view`, `show_req_detailed_view`, `show_search_view`,
  `show_settings_view`), the current `display_request`, a pointer to the
  `AvRequestListState`, the `UiShortcut shortcutManager`, and a set of
  `std::optional<std::function<void()>>` action slots (`on_new_request`,
  `on_save_changes`, `on_send_request`, `on_rename_request`, `on_show_shortcuts`,
  `on_show_style_editor`, `on_show_search`, `on_show_settings`,
  `on_display_request_change`).
  - **The pattern:** the view that *owns* an action `emplace()`s its slot in its
    constructor; `RootUi` binds shortcuts that call the slot **only when
    `has_value()`**. That keeps views decoupled — no view holds a pointer to
    another view.

- **`AvAppSettings`** (`av_app_settings.hpp/.cpp`) — **in progress**, currently an
  empty class. The settings model behind `SettingsViewUi`; see
  `notes.dev/settings_view_ui_plan.md` and the README todo list (env / general /
  shortcuts / network sections).

### 6.2 Networking — `av_net` / `avNet`

**`avNet::NetworkManager`** (`network_manager.hpp/.cpp`)

- Owns curl global init/cleanup (ctor/dtor). Ctor takes the base directory to save
  responses under (`NetworkManager(std::string_view path)`).
- Types: `request_method { get, post, put, patch, del, head, options }`,
  `response_status { Ok, Failed, Canceled }`, plus static `method_text()` /
  `status_text()` for display.
- **`http_request`** — self-contained snapshot: `{ method, url, headers, cookies,
  body? }`. It owns its strings **on purpose** so a worker thread can keep it alive
  independently of the UI model that produced it.
- **`http_result`** — `{ status, http_code, body, saved_path, response_headers,
  response_cookies, elapsed_mc }`.
- API: `http_result send(const http_request&)` is the one that matters;
  `get(url)` / `post(url)` and the `out_body`/`out_http_code` overloads are older
  conveniences.
- Private `fetch_core(request)` does the work: timeouts (`CONNECTTIMEOUT 15s`,
  `TIMEOUT 30s`, `NOSIGNAL`), follows redirects, collects response headers and
  `Set-Cookie` values through a header callback (parsed with Boost, §1), measures
  elapsed time, and writes the body to `<save_path>/responses.dev/`.
- **POST always sets an explicit empty body** (`POSTFIELDS ""`, `POSTFIELDSIZE 0`)
  when no body is given — otherwise libcurl reads the body from stdin and hangs
  forever in a GUI app.

### 6.3 Storage — `av_s` / `avS`

**`AvStorage`** (`av_storage.hpp/.cpp`) is the base: it resolves the app-data dir
(§5), opens/creates `_av_.db`, applies the PRAGMAs, and exposes
`get_db_path()` (returns the **directory**, not the file) plus protected
`get_schema_version()` / `set_schema_version()` over `PRAGMA user_version`.
`appSchemaVersion` is the current target (`2`). It forward-declares
`SQLite::Database` and holds it by `unique_ptr`, so SQLiteCpp's headers stay out
of every TU that merely includes a storage header.

Each table gets its own class, and they all use **private inheritance** from
`AvStorage` (`class AvRequestStorage : private AvStorage`) — the DB handle is an
implementation detail, deliberately not re-exported; a class that needs one member
re-exposes it explicitly (`using AvStorage::get_db_path;`).

| Class | Table(s) | Notes |
|-------|----------|-------|
| `AvRequestStorage` | `requests` | owns `migrate()` / `migrate_to_v1()` / `migrate_to_v2()` |
| `AvRequestParamsStorage` | `request_params` | FK → `requests(id)` ON DELETE CASCADE |
| `AvRequestHeadersStorage` | `request_headers` | same shape as params |
| `AvRequestCookiesStorage` | `request_cookies` | same shape as params |
| `AvEnvironmentStorage` | `environments` + `environment_variables` | one class, two tables; child FK cascades |

Shared conventions across all of them — **follow these for any new table**:

- The SQL lives as `const char *` members named after what it does
  (`create_*_table_sql`, `upsert_*_sql`, `select_all_*_sql`, `delete_*_sql`).
- Column indices are `const uint_fast8_t` members with a per-table prefix
  (`col_`, `pcol_`, `hcol_`, `ccol_`, `ecol_`, `vcol_`) so `getColumn(n)` calls
  read meaningfully.
- Writes are **upserts**: `INSERT … ON CONFLICT(id) DO UPDATE SET …`. There is no
  separate insert/update path.
- Ordering is explicit via an `order_by` column (drag-and-drop reordering is
  persisted), never insertion order.
- `key`/`value` are stored as `rkey`/`rvalue` (and `vkey`/`vvalue`) — `key` and
  `value` are awkward in SQLite contexts; keep the prefix.
- The child tables use `FOREIGN KEY(...) ON DELETE CASCADE` and rely on
  `PRAGMA foreign_keys=ON` from the base — deleting a request deletes its rows.
- Storage objects are cheap and short-lived: views hold them by `unique_ptr`
  members, and one-off work (`avS::AvEnvironmentStorage es;`) just constructs one
  on the stack.

### 6.4 UI — `av_ui` / `avUi`

**`avUi::NetworkManagerUi`** (`network_manager_ui.hpp/.cpp`) — the **host**.

- Ctor initializes GLFW, picks the primary monitor's video mode, and sets window
  hints (GL 3.3 core, decorated, resizable, maximized).
- `run()` creates the window, sets the multi-size window icon from
  `logo_icon.hpp`, initializes the ImGui context + GLFW/OpenGL3 backends,
  constructs the single `RootUi` node, and runs the loop:
  `glfwWaitEvents()` → new frame → `rootUi->draw()` → render → swap.
  **`glfwWaitEvents()` (blocking), not `glfwPollEvents()`** — arvis idles at 0%
  CPU when nothing is happening; anything that needs continuous animation has to
  wake the loop explicitly.
- **Deliberately NOT a `UiComponent`.** It is the driver of the tree, not a node
  in it.
- Windows-only: reaches the native `HWND` via `glfwGetWin32Window` and calls
  `DwmSetWindowAttribute(DWMWA_USE_IMMERSIVE_DARK_MODE)` so the OS title bar is
  dark. Guarded by `#ifdef _WIN32`; `dwmapi` is auto-linked with `#pragma comment`.

**`avUi::RootUi`** (`root_ui.hpp/.cpp`) — the root component. It:

1. Sets the global `ImGuiStyle` (rounding, padding, borders, tab/scrollbar sizing)
   and the custom colour overrides.
2. Builds the font atlas. Cousine and Roboto are added at 14px and 18px, and
   **each is followed immediately by a merged Noto Sans Georgian face** covering
   `U+10A0–10FF`, `U+1C90–1CBF`, `U+2D00–2D2F` — Latin-only faces left Georgian
   response text blank. `MergeMode` folds a face into the one added just before
   it, so **the pairs must stay adjacent**, and the `georgian_ranges` array is
   `static` because the atlas stores the pointer and rasterizes later.
3. Creates the four child views (`RequstListViewUi`, `DetailedRequestViewUi`,
   `SearchViewUi`, `SettingsViewUi`), passing each the shared
   `AvInterViewSharedState *`.
4. Registers every global shortcut on `shared->shortcutManager`.
5. `render()` draws children then calls `shortcutManager.process()`.

**Views** (all `UiComponent` subclasses, ctor `(std::string id, avR::AvState *sharedState)`):

- **`RequstListViewUi`** (note the spelling — *Requst*, no `e`; it is the real
  class name) — left panel: filter box, day-grouped list, drag & drop reorder,
  inline rename, delete confirmation (`pending_delete_req`), active-environment
  label, "new request". Owns an `AvRequestStorage`.
- **`DetailedRequestViewUi`** — the main panel: method + URL header, tabbed
  params/headers/cookies/body editor, send button, and the response footer.
  Owns the four request-scoped storages plus the `NetworkManager`. Sending goes
  through `send_request()` → `std::future<avNet::http_result> pending_response_v2`
  → `poll_response()` each frame. The response footer switches between
  `ResponseView::{tree, pretty, raw, res_headers, res_cookies}`; the tree/pretty
  modes delegate to a `JsonTreeView` held by `unique_ptr` so nlohmann's heavy
  header stays out of the view's own header.
  - **Member declaration order is load-bearing here.** The future is declared
    after `network_manager` so it is destroyed first, joining the worker before
    the manager it captured is torn down.
- **`SearchViewUi`** — the `Ctrl+F` palette. Parses a query into `QueryTerm`s with
  optional field prefixes (`:t` / `:title`), scores requests, and renders `Hit`s.
  **`Hit` stores an `id` plus copied strings, never a pointer into the request
  vector** — the list can reallocate while the palette is open.
- **`SettingsViewUi`** — `Ctrl+Shift+P` window; closes on `Escape` or focus loss.
  Currently a shell around the empty `AvAppSettings` (in progress).

**Widgets** (deliberately *not* `UiComponent`s — they are data-driven helpers a
view composes, not nodes in the retained tree):

- **`JsonTreeView`** (`json_tree_view.hpp/.cpp`) — parse once with `set_source()`
  (only when the text actually changes), then `render_tree()` / `render_pretty()`
  per frame. Caches the `dump(2)` and a `Stats` block (nodes/objects/arrays/keys/
  leaves/max depth/bytes). Filtering keeps parents of a hit visible.
- **`InputTextAutocomplete`** (`input_autocomplete_ui.hpp/.cpp`) — drop-in
  replacement for `ImGui::InputText(label, std::string*)` that pops up a
  `{{variable}}` completion list (Tab/Enter/click accepts, values shown greyed
  next to names). Companion free function `resolve_vars(text, vars, missing*)`
  substitutes `{{name}}` at send time. State lives in one function-local
  `VarCompleteState` keyed by the owning `ImGuiID` — there is exactly one popup at
  a time by design.
- **`UiShortcut`** (`ui_shortcut.hpp/.cpp`) — a `{display, binding, predicate,
  callback}` node with a `shortcuts` child vector, so the manager is itself a
  `UiShortcut`. `add()` registers, `process()` walks and fires. Predicates use
  `ImGui::Shortcut(..., ImGuiInputFlags_RouteGlobal)` and must also check the
  target action slot's `has_value()`.

Current bindings (all registered in `RootUi`'s ctor):

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | new request |
| `Ctrl+Enter` | send request |
| `Ctrl+F` | search palette |
| `Ctrl+S` | save changes |
| `Ctrl+/` | show shortcut cheat-sheet |
| `Ctrl+E` | ImGui style editor |
| `Ctrl+Shift+P` | settings |

### 6.5 The immediate-mode model (important mental model)

ImGui is immediate-mode: widgets aren't objects; you re-issue draw calls every frame
and a widget "returns true" on the frame it's interacted with. The `UiComponent`
tree is the **retained** layer that holds state/config; each frame `draw()`
translates it into ImGui calls and fires callbacks inline. **Build the tree once**
(in `RootUi`'s ctor) — never per frame.

---

## 7. Build & run

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
offered. The `base` preset wires the vcpkg toolchain; vcpkg packages auto-install
on first configure (which is slow — it also builds GLFW, ImGui and SQLiteCpp).

`external/vcpkg` is a **submodule**; a clone without `--recurse-submodules` fails
at configure. Fix with `git submodule update --init --recursive`.

**CRITICAL build-workflow rule:** after editing `CMakeLists.txt` (e.g. adding a
source file) **or `vcpkg.json`** (e.g. adding a Boost sub-library), you **must
re-run the configure step** (`cmake --preset <name>`), not just `cmake --build`. A
plain build reuses stale project files and the new `.cpp` won't be compiled →
`LNK2019` unresolved-symbol errors.

On Windows, if a build fails with `LNK1168: cannot open arvis.exe for writing`, a
previous instance is still running — `taskkill //F //IM arvis.exe` then rebuild.

**Iterating on Linux:** `scripts/watch.sh [preset]` (default `linux-debug`)
rebuilds and relaunches the app on every save under `src/`, `include/`,
`CMakeLists.txt` or `CMakePresets.json` — the closest thing to `dotnet watch` for a
compiled C++ GUI. Uses `inotifywait` when available, else an mtime poll.

---

## 8. Coding conventions

- **Boost first for new code** — see §1. This is the newest and most frequently
  missed convention.
- **Files:** one class per header/source. Header `include/<module>/<snake_case>.hpp`,
  source `src/<module>/<snake_case>.cpp` — `src/` mirrors `include/` with the same
  per-module folders (the sole exception is `main.cpp`, which stays at `src/`'s top
  level). File base name is the class name converted PascalCase→snake_case
  (`NetworkManagerUi` → `network_manager_ui`).
- **Namespaces:** `av` + module, as in §6. Match casing exactly everywhere
  (declaration and definition) or you get unresolved-symbol link errors.
- **Includes:** angle-bracket module paths, e.g. `#include <av_net/network_manager.hpp>`
  (works because `include/` is on the target's include path). Own header first —
  `SortIncludes: false` in `.clang-format` preserves that order deliberately.
- **`#pragma once`** in every header.
- **Member variables:** no `m_` prefix — name the member plainly (`config`, `label`,
  `root`) and **always refer to it through `this->`** inside methods (`this->config`),
  which also disambiguates it from same-named ctor params. Corollary: a data
  member and a member function can't share a name, so don't pair a `foo` member with a
  `foo()` accessor — use a distinct verb (`get_children()`, `configure()`) or drop the
  accessor.
- **Constructors:** initialize members in the **initializer list**, not by assignment
  in the body. Never assign `nullptr` to a `std::string`.
- **Member declaration order can be load-bearing** for destruction order — e.g. a
  `std::future` must be declared after whatever its worker captured, so it joins
  first. Comment it when you rely on it.
- **Polymorphic bases:** always `virtual ~T() = default;`.
- **Do not mark out-of-line (`.cpp`) function definitions `inline`** — it risks
  IFNDR/link errors and gives no inlining benefit. If you want inlining, define the
  body in the header instead.
- **Prefer `std::string_view`** for read-only string params, `std::string` (moved)
  for owned ones.
- **Ownership:** `unique_ptr` for exclusive ownership (children, storages, pimpl-ish
  members), `shared_ptr` only where two owners genuinely coexist (requests in the
  list vs. the current selection, the shared state block). Raw pointers are
  non-owning views (`AvInterViewSharedState *shared_state`).
- **Forward-declare heavy third-party types** in headers (`namespace SQLite { class
  Database; }`, `class JsonTreeView;`) and hold them by `unique_ptr`, so SQLiteCpp
  and nlohmann headers don't leak into every TU.
- **CMake source list is explicit** (not globbed) — a deliberate choice to keep the
  generated solution clean. Add new `.cpp` files to the `add_executable(arvis ...)`
  list (the scaffolding scripts do this automatically).
- **Formatting is enforced by `.clang-format`**: LLVM base, C++20, 4-space indent,
  Allman braces, 120-column limit, `NamespaceIndentation: All`, pointer/reference
  bound right (`const char *p`), `FixNamespaceComments: true` (keep the
  `} // namespace avR` trailers). Editors pick it up automatically; run
  `scripts/format.sh` (or `format.ps1`) for a full sweep, `--check` for a CI gate.
  **Never reformat `external/`** — it has its own `DisableFormat: true`.
- **Auto-generated files are off-limits to hand edits**: `include/av_ui/logo_icon.hpp`
  (from `scripts/bake_icon.ps1`) and `src/av_ui/fonts/*.h` (from
  `assets/binary_to_compressed_c.cpp`). Regenerate instead.

---

## 9. Scripts

| Script | Purpose |
|--------|---------|
| `new_class.ps1` / `new_class.sh` | scaffold a class (header + source + CMake registration) |
| `format.ps1` / `format.sh` | clang-format sweep over `include/` + `src/`; `--check` for CI |
| `watch.sh` | rebuild + relaunch on file change (Linux; default preset `linux-debug`) |
| `install.ps1` / `install.sh` | end-user installer — downloads the latest release binary, falls back to a source build |
| `bake_icon.ps1` | regenerates `include/av_ui/logo_icon.hpp` from `assets/icon.svg` |

### Scaffolding a class

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
won't overwrite existing files without `-Force`/`--force`. **After running,
reconfigure** (`cmake --preset <name>`).

The `.ps1` also runs on Linux under `pwsh`; the `.sh` needs bash 4+ (Linux standard;
macOS ships bash 3.2, so `brew install bash` there).

> Per §0, an AI assistant **does not run these** — write out what they would
> generate, including the exact `CMakeLists.txt` lines.

---

## 10. Common pitfalls (all previously hit in this project)

1. **Edited `CMakeLists.txt` or `vcpkg.json` but only ran `cmake --build`** → stale
   project, new sources not compiled / new package not installed, `LNK2019`. Fix:
   rerun `cmake --preset <name>`.
2. **Namespace casing mismatch** (e.g. `avUi` vs `avUI`, `avRoot` vs `avR`) →
   unresolved externals at link time. The compile succeeds per-file; only linking
   fails.
3. **`inline` on a `.cpp`-only definition used from another TU** → possible
   unresolved-symbol error; no benefit.
4. **POST with no body** → libcurl reads stdin and hangs the worker thread forever.
   Always set an empty POST body (already handled in `fetch_core`).
5. **No request timeout** → a dead endpoint hangs the UI's "sending…" state.
   Timeouts are set in `fetch_core`; keep them.
6. **Rebuild fails with `LNK1168`** on Windows → the app is still running; kill it.
7. **ImGui ID collisions** (duplicate labels / list rows) → wrong widget gets the
   click. Inside the `UiComponent` tree this is handled for you (base `draw()`
   pushes the node's id); for ad-hoc rows push your own `ImGui::PushID(index)`.
   Prefer `avR::UiScopedStyle` over manual `PushStyleVar` + hand-counted pops.
8. **Searching the whole tree** picks up `external/vcpkg` and `vcpkg_installed`
   noise → always exclude both.
9. **Storing a raw `AvRequest*` (or an iterator) across frames** → dangles when the
   request vector reallocates or an entry is deleted. Store the `int64_t id` (as
   `SearchViewUi::Hit` does) or a `shared_ptr`.
10. **Font merge pairs split apart** in `RootUi` → Georgian glyphs vanish.
    `MergeMode` folds a face into the one added immediately before it, and the
    glyph-range array must outlive the ctor (`static`).
11. **Adding a `boost/...` include without adding the port to `vcpkg.json`** →
    "cannot open include file" on a clean checkout even though it compiles on a
    machine with the headers already installed.
12. **Reformatting `external/`** or hand-editing `logo_icon.hpp` /
    `src/av_ui/fonts/*.h` → churn that gets blown away on the next regeneration.
13. **`glfwWaitEvents()` blocks** — a change that needs per-frame animation won't
    animate unless something wakes the loop.

---

## 11. Git & releases

- Branch: `main`. Remote: `origin` → `github.com/Rennn0/arvis.git`.
- `external/vcpkg` is a submodule.
- Git-ignored: `build/`, `.build/`, `out/`, `.out/`, `*.dev*` (so `notes.dev/` and
  `responses.dev/`), `.vs`, `.idea`, `*.db`, `*.ini`.
- Only commit when explicitly asked.
- **Releases** are driven by `.github/workflows/release.yml`: a push to `main`
  whose head-commit message contains `release v<X.Y.Z>` (case-insensitive) builds
  and publishes the binaries the install scripts download; a manual
  `workflow_dispatch` with an explicit version works too.

---
