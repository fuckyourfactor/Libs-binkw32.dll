#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
# Build only the FFmpeg libraries/codecs needed for Bink1 into static .a files.
# Windows x86 cross compilation; no proprietary Bink binaries or codecs.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
version="${FFMPEG_VERSION:-7.1.5}"
build_root="${FFMPEG_BUILD_ROOT:-$root/.build-ffmpeg}"
prefix="${FFMPEG_PREFIX:-$build_root/prefix}"
mkdir -p "$build_root" "$prefix"

case "$version" in
  *[!0-9.]*|'') echo "Invalid FFmpeg version: $version" >&2; exit 1 ;;
esac

for program in curl tar make pkg-config i686-w64-mingw32-gcc i686-w64-mingw32-ar; do
  command -v "$program" >/dev/null || { echo "Missing: $program" >&2; exit 1; }
done

archive="$build_root/ffmpeg-$version.tar.xz"
source_dir="$build_root/ffmpeg-$version"
if [[ ! -f "$archive" ]]; then
  curl --fail --location --retry 3 --retry-delay 3 \
    "https://ffmpeg.org/releases/ffmpeg-$version.tar.xz" -o "$archive"
fi
if [[ ! -d "$source_dir" ]]; then
  tar -xJf "$archive" -C "$build_root"
fi

mkdir -p "$build_root/build"
cd "$build_root/build"
"$source_dir/configure" \
  --prefix="$prefix" \
  --target-os=mingw32 \
  --arch=x86 \
  --cross-prefix=i686-w64-mingw32- \
  --enable-cross-compile \
  --enable-static \
  --disable-shared \
  --disable-programs \
  --disable-doc \
  --disable-debug \
  --disable-avdevice \
  --disable-avfilter \
  --disable-network \
  --disable-autodetect \
  --disable-asm \
  --disable-pthreads \
  --enable-w32threads \
  --disable-everything \
  --enable-demuxer=bink \
  --enable-decoder=bink,binkaudio_dct,binkaudio_rdft \
  --enable-protocol=file

make -j"$(nproc)"
make install

# Do not accept a "successful" build without every required archive.
for module in avformat avcodec avutil swscale swresample; do
  test -s "$prefix/lib/lib${module}.a" || {
    echo "Missing static FFmpeg library: lib${module}.a" >&2; exit 1;
  }
done

PKG_CONFIG_LIBDIR="$prefix/lib/pkgconfig" \
  pkg-config --static --exists libavformat libavcodec libavutil libswscale libswresample
printf 'FFmpeg %s static Windows x86 libraries at: %s\n' "$version" "$prefix"
