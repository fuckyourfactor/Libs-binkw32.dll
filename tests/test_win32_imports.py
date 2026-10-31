#!/usr/bin/env python3
"""Guard against import-check regex errors and shared FFmpeg dependencies.

The end-to-end test uses synthetic PE32 metadata and an objdump substitute.
It does not contain or load the proprietary DLL or claim codec functionality.
"""
# SPDX-License-Identifier: MIT
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from win32_imports import is_windows_runtime_dll  # noqa: E402
from test_pe_exports import make_pe  # noqa: E402


def main():
    for name in (
        'KERNEL32.dll', 'user32.DLL', 'gdi32.dll', 'WINMM.dll',
        'msvcrt.dll', 'ole32.dll', 'bcrypt.dll', 'advapi32.dll',
        'api-ms-win-core-synch-l1-2-0.dll',
        'ext-ms-win-ntuser-window-l1-1-0.dll',
    ):
        assert is_windows_runtime_dll(name), f'False rejection: {name}'
    for name in (
        'avcodec-61.dll', 'avformat-61.dll', 'avutil-59.dll',
        'swscale-8.dll', 'swresample-5.dll', 'libgcc_s_dw2-1.dll',
        'libwinpthread-1.dll', 'some_custom.dll',
        'api-ms-win-invalid.dll.evil', '',
    ):
        assert not is_windows_runtime_dll(name), f'False acceptance: {name}'

    # On POSIX CI, exercise the complete checker, including the code path that
    # failed after linking FFmpeg. Fake only objdump's imported-DLL listing.
    if os.name != 'nt':
        manifest = json.loads((ROOT / 'docs/exports.json').read_text(encoding='utf-8'))
        with tempfile.TemporaryDirectory() as dirname:
            base = Path(dirname)
            dll = base / 'binkw32.dll'
            dll.write_bytes(make_pe([(x['name'], x['ordinal']) for x in manifest]))
            cache = base / 'CMakeCache.txt'
            cache.write_text('BINK_WITH_FFMPEG:BOOL=ON\nBINK_FFMPEG_STATIC:BOOL=ON\n')
            fake_bin = base / 'bin'
            fake_bin.mkdir()
            fake_objdump = fake_bin / 'objdump'
            env = os.environ.copy()
            env['PATH'] = str(fake_bin) + os.pathsep + env.get('PATH', '')
            command = [sys.executable, str(ROOT / 'tools/check_win32_dll.py'),
                       str(dll), '--require-static-ffmpeg', '--cmake-cache', str(cache)]

            def verify(imports, success):
                fake_objdump.write_text('#!/bin/sh\n' + ''.join(
                    f'echo "DLL Name: {name}"\n' for name in imports))
                fake_objdump.chmod(0o755)
                result = subprocess.run(command, env=env, capture_output=True, text=True)
                if success:
                    assert result.returncode == 0, result.stderr + result.stdout
                    assert '66' in result.stdout and 'DLL dependencies' in result.stdout
                else:
                    assert result.returncode != 0, result.stdout
                    assert 'Unexpected runtime DLL dependencies' in result.stderr, result.stderr

            verify(['KERNEL32.dll', 'USER32.dll', 'GDI32.dll', 'WINMM.dll',
                    'bcrypt.dll', 'ole32.dll'], success=True)
            verify(['KERNEL32.dll', 'avcodec-61.dll'], success=False)

    print('OK: valid Windows DLL imports accepted; FFmpeg DLL dependencies rejected')


if __name__ == '__main__':
    main()
