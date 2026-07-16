#!/bin/sh
# arvis installer — build from source and install (or run) the app.
#
# Usage (remote):
#   curl -fsSL https://raw.githubusercontent.com/Rennn0/arvis/main/scripts/install.sh | sh
#
# Pass flags through the pipe with `sh -s --`:
#   curl -fsSL .../install.sh | sh -s -- --run
#   curl -fsSL .../install.sh | sh -s -- --prefix /usr/local/bin
#   curl -fsSL .../install.sh | sh -s -- --install-deps
#
# Or override behaviour with env vars:
#   ARVIS_REPO   git URL to clone            (default: https://github.com/Rennn0/arvis.git)
#   ARVIS_REF    branch/tag to build         (default: main)
#   ARVIS_PREFIX install dir for the binary  (default: $HOME/.local/bin)
#
# This is a source build: it clones the repo (shallow, with the vcpkg
# submodule), bootstraps vcpkg, and compiles a Release binary. libcurl comes
# from vcpkg; GLFW + Dear ImGui are fetched by CMake at configure time.
#
# POSIX sh on purpose — `curl | sh` feeds the script to /bin/sh and ignores the
# shebang, so no bashisms (no [[ ]], arrays, or `local` reliance).

set -eu

# --- configuration (all overridable via env) --------------------------------
APP="arvis"
REPO_URL="${ARVIS_REPO:-https://github.com/Rennn0/arvis.git}"
REF="${ARVIS_REF:-main}"
PREFIX="${ARVIS_PREFIX:-$HOME/.local/bin}"
BUILD_TYPE="${ARVIS_BUILD_TYPE:-Release}"

MODE="install"     # install | run
INSTALL_DEPS=0
KEEP_BUILD=0
SRC=""             # set once we have a work dir; used by cleanup()

# --- pretty logging (only colorize when stderr is a terminal) ---------------
if [ -t 2 ]; then
    C_INFO='\033[1;34m'; C_OK='\033[1;32m'; C_WARN='\033[1;33m'; C_ERR='\033[1;31m'; C_OFF='\033[0m'
else
    C_INFO=''; C_OK=''; C_WARN=''; C_ERR=''; C_OFF=''
fi
info() { printf "${C_INFO}==>${C_OFF} %s\n" "$*" >&2; }
ok()   { printf "${C_OK}==>${C_OFF} %s\n" "$*" >&2; }
warn() { printf "${C_WARN}warning:${C_OFF} %s\n" "$*" >&2; }
die()  { printf "${C_ERR}error:${C_OFF} %s\n" "$*" >&2; exit 1; }

usage() {
    cat >&2 <<EOF
arvis installer

  --run             build and run without installing
  --prefix DIR      install the binary here (default: $HOME/.local/bin)
  --ref REF         git branch or tag to build (default: main)
  --install-deps    install system build dependencies via the detected package
                    manager (needs sudo); otherwise missing deps just print a hint
  --keep-build      do not delete the temporary build directory on exit
  -h, --help        show this help
EOF
}

# --- arg parsing ------------------------------------------------------------
while [ $# -gt 0 ]; do
    case "$1" in
        --run)          MODE="run" ;;
        --prefix)       PREFIX="${2:?--prefix needs a directory}"; shift ;;
        --ref)          REF="${2:?--ref needs a value}"; shift ;;
        --install-deps) INSTALL_DEPS=1 ;;
        --keep-build)   KEEP_BUILD=1 ;;
        -h|--help)      usage; exit 0 ;;
        *)              die "unknown argument: $1 (try --help)" ;;
    esac
    shift
done

# --- cleanup on exit --------------------------------------------------------
cleanup() {
    if [ "$KEEP_BUILD" -eq 0 ] && [ -n "$SRC" ] && [ -d "$SRC" ]; then
        rm -rf "$SRC"
    fi
}
trap cleanup EXIT INT TERM

have() { command -v "$1" >/dev/null 2>&1; }

# --- platform + package-manager detection -----------------------------------
OS="$(uname -s)"
case "$OS" in
    Linux)  PLATFORM="linux" ;;
    Darwin) PLATFORM="macos" ;;
    *)      die "unsupported OS '$OS'. On Windows, build with the 'windows' CMake preset (see CLAUDE.md §4)." ;;
esac

PKG=""
for pm in apt-get dnf yum pacman zypper brew; do
    if have "$pm"; then PKG="$pm"; break; fi
done

# Full dependency install line for the detected package manager.
deps_command() {
    case "$PKG" in
        apt-get) echo "sudo apt-get update && sudo apt-get install -y git cmake ninja-build build-essential pkg-config curl zip unzip tar libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev" ;;
        dnf|yum) echo "sudo $PKG install -y git cmake ninja-build gcc-c++ make pkgconf-pkg-config curl zip unzip tar libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel" ;;
        pacman)  echo "sudo pacman -S --needed git cmake ninja base-devel pkgconf curl zip unzip tar libx11 libxrandr libxinerama libxcursor libxi mesa" ;;
        zypper)  echo "sudo zypper install -y git cmake ninja gcc-c++ make pkg-config curl zip unzip tar libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel Mesa-libGL-devel" ;;
        brew)    echo "brew install git cmake ninja pkg-config" ;;
        *)       echo "" ;;
    esac
}

maybe_install_deps() {
    [ "$INSTALL_DEPS" -eq 1 ] || return 0
    cmd="$(deps_command)"
    [ -n "$cmd" ] || die "--install-deps: no supported package manager found; install the build tools manually."
    info "installing system dependencies via $PKG ..."
    sh -c "$cmd"
}

# --- required tools ---------------------------------------------------------
check_tools() {
    missing=""
    for t in git cmake curl tar; do
        have "$t" || missing="$missing $t"
    done
    # Linux presets use the Ninja generator; a C++ compiler is always needed.
    if [ "$PLATFORM" = "linux" ]; then
        have ninja || have ninja-build || missing="$missing ninja"
    fi
    if ! have c++ && ! have g++ && ! have clang++; then
        missing="$missing c++-compiler"
    fi

    [ -n "$missing" ] || return 0

    warn "missing required tools:$missing"
    hint="$(deps_command)"
    if [ -n "$hint" ]; then
        printf "  install them with:\n    %s\n" "$hint" >&2
        printf "  or re-run with --install-deps to do it automatically.\n" >&2
    fi
    die "prerequisites not satisfied."
}

# --- build steps ------------------------------------------------------------
fetch_sources() {
    SRC="$(mktemp -d "${TMPDIR:-/tmp}/arvis-install.XXXXXX")"
    info "cloning $REPO_URL@$REF into $SRC"
    # Shallow clone + shallow submodule keeps the vcpkg download small.
    git clone --depth 1 --branch "$REF" --recurse-submodules --shallow-submodules \
        "$REPO_URL" "$SRC" \
        || die "git clone failed (is '$REF' a valid branch/tag on $REPO_URL?)"
}

bootstrap_vcpkg() {
    if [ ! -x "$SRC/external/vcpkg/vcpkg" ]; then
        info "bootstrapping vcpkg ..."
        "$SRC/external/vcpkg/bootstrap-vcpkg.sh" -disableMetrics \
            || die "vcpkg bootstrap failed."
    fi
}

build() {
    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
    if [ "$PLATFORM" = "linux" ]; then
        # Linux has a CMake preset; use it (Ninja + vcpkg toolchain wired in).
        info "configuring (linux-release) ..."
        ( cd "$SRC" && cmake --preset linux-release ) || die "cmake configure failed."
        info "building ($jobs jobs) — first build compiles curl/GLFW/ImGui, this takes a while ..."
        ( cd "$SRC" && cmake --build --preset linux-release -j "$jobs" ) || die "build failed."
        BIN="$SRC/build/linux-release/$APP"
    else
        # macOS: no preset defined, so configure directly with the vcpkg toolchain.
        BDIR="$SRC/build/macos-release"
        info "configuring (macOS, $BUILD_TYPE) ..."
        cmake -S "$SRC" -B "$BDIR" \
            -DCMAKE_TOOLCHAIN_FILE="$SRC/external/vcpkg/scripts/buildsystems/vcpkg.cmake" \
            -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
            || die "cmake configure failed."
        info "building ($jobs jobs) — first build compiles curl/GLFW/ImGui, this takes a while ..."
        cmake --build "$BDIR" --config "$BUILD_TYPE" -j "$jobs" || die "build failed."
        BIN="$BDIR/$APP"
    fi
    [ -x "$BIN" ] || die "build finished but binary not found at $BIN"
}

do_install() {
    mkdir -p "$PREFIX"
    install -m 0755 "$BIN" "$PREFIX/$APP" 2>/dev/null \
        || { cp "$BIN" "$PREFIX/$APP" && chmod 0755 "$PREFIX/$APP"; }
    ok "installed $APP -> $PREFIX/$APP"

    case ":$PATH:" in
        *":$PREFIX:"*) : ;;  # already on PATH
        *) warn "$PREFIX is not on your PATH. Add this to your shell profile:"
           printf '    export PATH="%s:$PATH"\n' "$PREFIX" >&2 ;;
    esac
    info "run it with: $APP"
}

do_run() {
    ok "launching $APP from build tree"
    KEEP_BUILD=1          # don't delete the binary out from under the process
    exec "$BIN"
}

# --- main -------------------------------------------------------------------
info "arvis installer ($PLATFORM)"
maybe_install_deps
check_tools
fetch_sources
bootstrap_vcpkg
build

if [ "$MODE" = "run" ]; then
    do_run
else
    do_install
fi
