# binkw32.dll

An independent Windows x86 Bink 1 compatibility library with FFmpeg-based video and audio decoding.

## Build

GitHub Actions: **Actions → Build binkw32.dll (FFmpeg, Windows x86) → Run workflow**. The workflow builds static FFmpeg dependencies and publishes a single `binkw32.dll` file.

Local unit tests (without FFmpeg):

```sh
cmake -S . -B build -DBINK_WITH_FFMPEG=OFF -DBINK_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

For the standalone Windows build, use `.github/workflows/build.yml` and the supplied MinGW toolchain configuration.

## License

Project source: MIT (see `LICENSE`). FFmpeg is a separate project with its own licensing obligations. Static linking may require distributing FFmpeg license notices, corresponding source, and a way to relink with modified FFmpeg libraries; review the applicable FFmpeg LGPL terms before redistributing binaries. See https://ffmpeg.org/legal.html.
