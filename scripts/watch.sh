#!/usr/bin/env bash
# watch.sh — rebuild + relaunch arvis whenever a source file changes.
#
# arvis is compiled C++: there is no in-process hot reload. This script gives
# the closest equivalent to `dotnet watch` — on every save under src/, include/
# or CMakeLists.txt it rebuilds the active preset and, on a successful build,
# kills the running window and relaunches it.
#
# Usage:
#   scripts/watch.sh [preset]
#     preset  CMake build preset to use (default: linux-debug)
#
# Uses inotifywait (from inotify-tools) when available for instant, event-based
# rebuilds; otherwise falls back to a dependency-free mtime poll.

set -uo pipefail

PRESET="${1:-linux-debug}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/build/$PRESET/arvis"
WATCH_DIRS=("$ROOT/src" "$ROOT/include")
WATCH_FILES=("$ROOT/CMakeLists.txt" "$ROOT/CMakePresets.json")

APP_PID=""

log()  { printf '\033[36m[watch]\033[0m %s\n' "$*"; }
ok()   { printf '\033[32m[watch]\033[0m %s\n' "$*"; }
err()  { printf '\033[31m[watch]\033[0m %s\n' "$*" >&2; }

stop_app() {
  if [[ -n "$APP_PID" ]] && kill -0 "$APP_PID" 2>/dev/null; then
    kill "$APP_PID" 2>/dev/null
    wait "$APP_PID" 2>/dev/null
  fi
  APP_PID=""
}

cleanup() { stop_app; log "stopped"; exit 0; }
trap cleanup INT TERM

rebuild_and_run() {
  log "building ($PRESET)…"
  if cmake --build --preset "$PRESET"; then
    ok "build ok — relaunching"
    stop_app
    "$BIN" &
    APP_PID=$!
  else
    err "build failed — keeping the running instance up"
  fi
}

changed_dirs=()
for d in "${WATCH_DIRS[@]}"; do [[ -d "$d" ]] && changed_dirs+=("$d"); done

log "watching: ${changed_dirs[*]} ${WATCH_FILES[*]}"
log "preset:   $PRESET   ->   $BIN"
log "Ctrl-C to stop."

# Initial build + launch.
rebuild_and_run

if command -v inotifywait >/dev/null 2>&1; then
  log "using inotifywait (event-based) — triggers on save (close_write / atomic rename)"
  # DEBOUNCE: after the first save event, keep collecting events for this long
  # before building. This waits for you to *finish* editing — a burst of quick
  # saves coalesces into a single rebuild instead of one build per keystroke-save.
  DEBOUNCE="${WATCH_DEBOUNCE:-0.5}"
  while true; do
    # Block until a save COMPLETES. We listen for close_write (a file opened for
    # writing was closed — i.e. Ctrl-S flushed and released the handle) and
    # moved_to (editors that save atomically write a temp file then rename it
    # over the target). We deliberately do NOT watch bare `modify`, which fires
    # on every intermediate flush *while you are still typing*.
    inotifywait -q -r -e close_write,moved_to,create,delete \
      "${changed_dirs[@]}" "${WATCH_FILES[@]}" >/dev/null 2>&1

    # Drain any further save events that land during the debounce window, so a
    # save-heavy burst (or the editor's own follow-up writes) settles first.
    while inotifywait -q -q -r -t 1 -e close_write,moved_to,create,delete \
      "${changed_dirs[@]}" "${WATCH_FILES[@]}" >/dev/null 2>&1; do
      :
    done
    sleep "$DEBOUNCE"
    rebuild_and_run
  done
else
  log "inotifywait not found — using mtime poll (install inotify-tools for instant rebuilds)"
  snapshot() {
    find "${changed_dirs[@]}" "${WATCH_FILES[@]}" \
      -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' \
        -o -name 'CMakeLists.txt' -o -name 'CMakePresets.json' \) \
      -printf '%T@ %p\n' 2>/dev/null | sort
  }
  last="$(snapshot)"
  while true; do
    sleep 1
    cur="$(snapshot)"
    if [[ "$cur" != "$last" ]]; then
      # Something changed. Wait for you to FINISH editing: keep re-sampling until
      # the tree stops changing for a full interval (quiescence), so a run of
      # saves (or an auto-saving editor) coalesces into one rebuild instead of
      # firing mid-edit.
      while true; do
        last="$cur"
        sleep 1
        cur="$(snapshot)"
        [[ "$cur" == "$last" ]] && break
      done
      rebuild_and_run
    fi
  done
fi
