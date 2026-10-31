# SPDX-License-Identifier: MIT
# Linux -> Windows x86 (PE32), GNU MinGW-w64 cross compiler.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86)
set(CMAKE_C_COMPILER i686-w64-mingw32-gcc)
set(CMAKE_RC_COMPILER i686-w64-mingw32-windres)
set(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
# The CI-provided, cross-built FFmpeg is discovered by pkg-config, not by
# CMake's find_library, so its prefix is set through PKG_CONFIG_LIBDIR.
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
