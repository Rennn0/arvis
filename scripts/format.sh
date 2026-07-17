#!/usr/bin/env bash
#
# Run clang-format over the project's own C++ sources (include/ + src/).
#
# Formats every .hpp/.cpp under include/ and src/ in place using the repo's
# .clang-format. external/ and build/ are never touched (they aren't scanned,
# and external/.clang-format disables formatting there as a second guard).
#
# Editors that honour .clang-format (VS Code + ms-vscode.cpptools, Visual
# Studio 2022, clangd, CLion/Rider) format on save automatically; this script
# is for a full-tree sweep or a CI gate.
#
# Usage:
#   scripts/format.sh            format all project sources in place
#   scripts/format.sh --check    CI gate: non-zero exit if anything is unformatted
#
# clang-format is taken from $CLANG_FORMAT if set, else from PATH.

set -euo pipefail

check=0
if [[ "${1:-}" == "--check" ]]; then
    check=1
elif [[ -n "${1:-}" ]]; then
    echo "unknown argument: $1" >&2
    echo "usage: scripts/format.sh [--check]" >&2
    exit 2
fi

clang_format="${CLANG_FORMAT:-clang-format}"
if ! command -v "$clang_format" >/dev/null 2>&1; then
    echo "clang-format not found (set \$CLANG_FORMAT or install it)" >&2
    exit 1
fi

# Repo root is the parent of this scripts/ folder.
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Collect project sources; -print0 keeps paths with spaces intact.
mapfile -d '' files < <(
    find "$root/include" "$root/src" \
        -type f \( -name '*.hpp' -o -name '*.cpp' -o -name '*.h' -o -name '*.cc' \) \
        -print0
)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "no sources found"
    exit 0
fi

echo "using $("$clang_format" --version)"

if [[ $check -eq 1 ]]; then
    # --dry-run --Werror exits non-zero when a file would change.
    if "$clang_format" --dry-run --Werror "${files[@]}"; then
        echo "${#files[@]} file(s) already formatted"
    else
        echo "run scripts/format.sh to fix" >&2
        exit 1
    fi
else
    "$clang_format" -i "${files[@]}"
    echo "formatted ${#files[@]} file(s)"
fi
