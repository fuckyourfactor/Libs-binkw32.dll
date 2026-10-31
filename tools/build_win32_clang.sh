#!/usr/bin/env bash
# Cross-builds a metadata-only 32-bit Windows DLL with Clang+LLD and no SDK.
# Requires: clang, lld-link, objdump, Python3, bash.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build-win32
cat > build-win32/crt.def <<'END'
LIBRARY msvcrt.dll
EXPORTS
malloc
free
memcpy
memset
_vsnprintf
END
cat > build-win32/kernel.def <<'END'
LIBRARY KERNEL32.dll
EXPORTS
CreateFileA@28=CreateFileA
GetFileSize@8=GetFileSize
VirtualQuery@12=VirtualQuery
ReadFile@20=ReadFile
CloseHandle@4=CloseHandle
GetTickCount@0=GetTickCount
Sleep@4=Sleep
GetEnvironmentVariableA@12=GetEnvironmentVariableA
WriteFile@20=WriteFile
END
cat > build-win32/user.def <<'END'
LIBRARY USER32.dll
EXPORTS
GetDC@4=GetDC
GetClientRect@8=GetClientRect
ClientToScreen@8=ClientToScreen
ReleaseDC@8=ReleaseDC
END
cat > build-win32/gdi.def <<'END'
LIBRARY GDI32.dll
EXPORTS
StretchDIBits@52=StretchDIBits
END
cat > build-win32/winmm.def <<'END'
LIBRARY WINMM.dll
EXPORTS
waveOutOpen@24=waveOutOpen
waveOutPrepareHeader@12=waveOutPrepareHeader
waveOutUnprepareHeader@12=waveOutUnprepareHeader
waveOutWrite@12=waveOutWrite
waveOutReset@4=waveOutReset
waveOutPause@4=waveOutPause
waveOutRestart@4=waveOutRestart
waveOutClose@4=waveOutClose
END
for item in crt kernel user gdi winmm; do
  lld-link /dll /noentry /machine:x86 \
    "/def:build-win32/$item.def" "/implib:build-win32/$item.lib" \
    "/out:build-win32/$item.dll"
done
for src in bink_api container pixel audio_mix sound_driver surface trace audio_win32 platform_win32 decoder_none; do
  clang -target i686-pc-windows-msvc -O2 -std=c11 -ffreestanding \
    -fno-stack-protector -Iinclude -Isrc -DBINK_BUILD \
    -c "src/$src.c" -o "build-win32/$src.obj"
done
aliases=()
for entry in CreateFileA@28 GetFileSize@8 VirtualQuery@12 ReadFile@20 CloseHandle@4 GetTickCount@0 \
             Sleep@4 GetEnvironmentVariableA@12 WriteFile@20 GetDC@4 GetClientRect@8 ClientToScreen@8 ReleaseDC@8 StretchDIBits@52 waveOutOpen@24 waveOutPrepareHeader@12 waveOutUnprepareHeader@12 waveOutWrite@12 waveOutReset@4 waveOutPause@4 waveOutRestart@4 waveOutClose@4; do
  aliases+=("/alternatename:__imp__${entry}=__imp_${entry}")
done
lld-link /dll /machine:x86 /safeseh:no /entry:DllMain@12 /nodefaultlib \
  /out:build-win32/binkw32.dll /def:src/binkw32.def \
  build-win32/*.obj build-win32/crt.lib build-win32/kernel.lib \
  build-win32/gdi.lib build-win32/user.lib build-win32/winmm.lib "${aliases[@]}"
python3 tools/fix_win32_imports.py build-win32/binkw32.dll
python3 tools/check_def.py src/binkw32.def docs/exports.json build-win32/binkw32.dll
