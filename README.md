# binkw32.dll

An independent Windows x86 Bink 1 compatibility library with FFmpeg-based video and audio decoding. The Bink container is read directly; only the FFmpeg Bink codecs, image conversion, and audio conversion libraries are built and statically linked. No FFmpeg runtime DLLs or libavformat are required.

## Build

GitHub Actions: **Actions → Build binkw32.dll (FFmpeg, Windows x86) → Run workflow**. The workflow builds and caches the minimal static FFmpeg dependencies, builds the codec archives with size-oriented flags and link-time optimization, disables optional tracing, strips debug information, checks its 66 ABI exports and its static FFmpeg dependencies, and tests two UPX compression profiles. Only an export-compatible, smaller packed binary is accepted. The workflow publishes a single `binkw32.dll` file.

Local unit tests (without FFmpeg):

```sh
cmake -S . -B build -DBINK_WITH_FFMPEG=OFF -DBINK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For the standalone Windows build, use `.github/workflows/build.yml` and the supplied MinGW toolchain configuration. Release builds use `-Os` and supported LTO optimizations by default. Set `-DBINK_MINIMIZE_SIZE=OFF` to restore the `-O3` speed-oriented release configuration or `-DBINK_ENABLE_TRACE=ON` to restore the optional `BINK_OPEN_TRACE` diagnostic output. The size profile preserves video decoding and audio conversion; it does not strip Bink entry points or remove FFmpeg decoders.

Packing with UPX may increase DLL startup time and may trigger antivirus false positives. The workflow compares the unpacked size against packed candidates and retains the smallest candidate that passes static PE/exports/import verification. These static checks do **not** substitute for a real playback test in the game on Windows; only the unpacked DLL has the original load-time behavior. LZMA in particular prioritizes compression ratio over decompression speed.

## License

Project source: MIT (see `LICENSE`). FFmpeg is a separate project with its own licensing obligations. Static linking may require distributing FFmpeg license notices, corresponding source, and a way to relink with modified FFmpeg libraries; review the applicable FFmpeg LGPL terms before redistributing binaries. See https://ffmpeg.org/legal.html.
