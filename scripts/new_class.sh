#!/usr/bin/env bash
#
# Scaffold a new C++ class: header + source + CMake registration.
#
# Creates include/<module>/<file>.hpp and src/<module>/<file>.cpp pre-filled with
# #pragma once, the right namespace, a class skeleton, and the matching
# #include, then registers both in CMakeLists.txt: the .cpp into the
# add_executable(arvis ...) list (so it compiles) and the .hpp into the
# target_sources(arvis PRIVATE ...) list (so it shows up in the generated
# Visual Studio / IDE project).
#
# File base name is derived from the class name (PascalCase -> snake_case);
# the namespace is derived from the module (av_net -> avNet). Override either
# with --namespace / --filename when the defaults don't fit (e.g. --namespace avR).
#
# Usage:
#   scripts/new_class.sh <module> <ClassName> [options]
#   scripts/new_class.sh av_net Downloader
#   scripts/new_class.sh av_root Root --namespace avR
#
# Options:
#   --namespace <ns>   override derived namespace
#   --filename  <name> override derived file base name
#   --target    <t>    add_executable target to register into (default: arvis)
#   --force            overwrite existing files
#
# Requires bash 4+ (standard on Linux).

set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }

# --- args --------------------------------------------------------------------
[[ $# -ge 2 ]] || die "usage: $0 <module> <ClassName> [--namespace ns] [--filename name] [--target t] [--force]"

module="$1"; shift
class_name="$1"; shift

namespace=""
filename=""
target="arvis"
force=0

while [[ $# -gt 0 ]]; do
    case "$1" in
        --namespace) namespace="$2"; shift 2 ;;
        --filename)  filename="$2";  shift 2 ;;
        --target)    target="$2";    shift 2 ;;
        --force)     force=1;        shift ;;
        *) die "unknown option: $1" ;;
    esac
done

# --- repo root (parent of this scripts/ dir) ---------------------------------
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(dirname "$script_dir")"

# --- derive filename / namespace ---------------------------------------------
if [[ -z "$filename" ]]; then
    # PascalCase -> snake_case: insert _ at lower/digit->Upper boundary, lowercase.
    filename="$(sed -E 's/([a-z0-9])([A-Z])/\1_\2/g' <<<"$class_name" | tr '[:upper:]' '[:lower:]')"
fi

if [[ -z "$namespace" ]]; then
    # av_net -> avNet : first part lowercase, following parts Title-cased.
    IFS='_' read -ra parts <<<"$module"
    namespace="${parts[0],,}"
    for ((i = 1; i < ${#parts[@]}; i++)); do
        p="${parts[$i],,}"
        namespace+="${p^}"
    done
fi

header_rel="include/$module/$filename.hpp"
source_rel="src/$module/$filename.cpp"
header_abs="$repo_root/$header_rel"
source_abs="$repo_root/$source_rel"
cmake_abs="$repo_root/CMakeLists.txt"

# --- guard against clobbering ------------------------------------------------
if [[ $force -eq 0 ]]; then
    for f in "$header_abs" "$source_abs"; do
        [[ -e "$f" ]] && die "refusing to overwrite existing file: $f (use --force)"
    done
fi

# --- header ------------------------------------------------------------------
mkdir -p "$(dirname "$header_abs")"
cat >"$header_abs" <<EOF
#pragma once

namespace $namespace
{
    class $class_name
    {
    public:
        $class_name();
        ~$class_name();

    private:
    };
} // namespace $namespace
EOF
echo "created  $header_rel"

# --- source ------------------------------------------------------------------
mkdir -p "$(dirname "$source_abs")"
cat >"$source_abs" <<EOF
#include <$module/$filename.hpp>

namespace $namespace
{
    $class_name::$class_name()
    {
    }

    $class_name::~$class_name()
    {
    }
} // namespace $namespace
EOF
echo "created  $source_rel"

# --- register in CMakeLists.txt ----------------------------------------------
# The .cpp goes into add_executable(<target> ...) so it gets compiled; the .hpp
# goes into target_sources(<target> PRIVATE ...) so it shows up (and is
# editable) in the generated Visual Studio / IDE project. Both are inserted the
# same way: find the opening "<block>(<target>" line, then insert just before
# the line that closes it (a lone ')').

# register_in_block <block> <entry> <rel-path>
# Idempotent; leaves the file untouched (with a warning) if the block is absent.
register_in_block() {
    local block="$1" entry="$2" rel="$3"

    if grep -qF "$entry" "$cmake_abs"; then
        echo "CMake    already lists $rel (skipped)"
        return 0
    fi

    local tmp
    tmp="$(mktemp)"
    awk -v block="$block" -v target="$target" -v entry="$entry" '
        BEGIN { inside = 0; done = 0 }
        {
            if ($0 ~ block "\\([ \t]*" target "([ \t]|$)") inside = 1
            if (inside && !done && $0 ~ /^[ \t]*\)[ \t]*$/) {
                print entry
                inside = 0
                done = 1
            }
            print
        }
    ' "$cmake_abs" >"$tmp"

    # The entry is present in $tmp only if awk found the block and inserted it.
    if grep -qF "$entry" "$tmp"; then
        mv "$tmp" "$cmake_abs"
        echo "CMake    registered $rel in $block($target ...)"
    else
        rm -f "$tmp"
        echo "warning: could not find $block($target ...); add '$rel' manually." >&2
    fi
}

register_in_block "add_executable" "    $source_rel" "$source_rel"
register_in_block "target_sources" "    $header_rel" "$header_rel"

echo ""
echo "Done. Reconfigure to pick it up:  cmake --preset linux-debug"
