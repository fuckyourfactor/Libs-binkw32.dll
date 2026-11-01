#!/usr/bin/env bash
set -euo pipefail

if (( $# != 2 )); then
  echo 'Usage: pack_dll.sh dist/binkw32.dll build-win32-ffmpeg/CMakeCache.txt' >&2
  exit 2
fi

dll="$1"
cache="$2"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [[ ! -s "$dll" || ! -s "$cache" ]]; then
  echo 'Missing DLL or CMake cache' >&2
  exit 1
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
cp "$dll" "$work/original.dll"
best_size="$(stat -c '%s' "$dll")"
selected=none

for mode in lzma exhaustive; do
  cp "$work/original.dll" "$work/binkw32.dll"
  case "$mode" in
    lzma)
      args=(--best --lzma)
      ;;
    exhaustive)
      args=(--ultra-brute)
      ;;
  esac
  echo "Testing UPX profile: $mode"
  if ! timeout 300 upx -q "${args[@]}" "$work/binkw32.dll"; then
    echo "UPX $mode failed or timed out; trying another profile"
    continue
  fi
  if ! upx -t "$work/binkw32.dll"; then
    echo "UPX test failed for $mode"
    continue
  fi
  if ! python3 "$root/tools/check_win32_dll.py" "$work/binkw32.dll" \
       --require-static-ffmpeg --cmake-cache "$cache"; then
    echo "UPX $mode changed the ABI or imports; rejecting candidate"
    continue
  fi
  size="$(stat -c '%s' "$work/binkw32.dll")"
  if (( size < best_size )); then
    cp "$work/binkw32.dll" "$dll"
    best_size="$size"
    selected="$mode"
  fi
done

echo "Result: ${best_size} bytes; compressor: ${selected}"
