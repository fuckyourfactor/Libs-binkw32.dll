#!/usr/bin/env python3
"""Check output is PE32 x86, exact original .def exports, and standalone link."""
# SPDX-License-Identifier: MIT
import argparse
from pathlib import Path
import re
from win32_imports import is_windows_runtime_dll
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('dll', type=Path)
parser.add_argument('--require-static-ffmpeg', action='store_true')
parser.add_argument('--cmake-cache', type=Path)
args = parser.parse_args()
project = Path(__file__).resolve().parent.parent

if args.dll.name != 'binkw32.dll':
    parser.error('Output file must be named exactly binkw32.dll')
if not args.dll.is_file() or args.dll.stat().st_size == 0:
    parser.error('DLL does not exist or is empty')

with args.dll.open('rb') as stream:
    image = stream.read(512)
if image[:2] != b'MZ':
    parser.error('Not an MZ executable')
pe_offset = int.from_bytes(image[0x3c:0x40], 'little')
with args.dll.open('rb') as stream:
    stream.seek(pe_offset)
    pe = stream.read(26)
if pe[:4] != b'PE\0\0' or int.from_bytes(pe[4:6], 'little') != 0x14c:
    parser.error('Expected 32-bit x86 PE (IMAGE_FILE_MACHINE_I386)')
if int.from_bytes(pe[24:26], 'little') != 0x10b:
    parser.error('Expected PE32, not PE32+ (x64)')

subprocess.run(
    [sys.executable, str(project / 'tools/check_def.py'),
     str(project / 'src/binkw32.def'), str(project / 'docs/exports.json'),
     str(args.dll)], check=True,
)

imports = re.findall(
    r'^\s*DLL Name:\s*(\S+)',
    subprocess.check_output(['objdump', '-p', str(args.dll)], text=True),
    re.MULTILINE,
)
if not imports:
    parser.error('Cannot determine PE imported DLL dependencies')
if args.require_static_ffmpeg:
    if args.cmake_cache is None or not args.cmake_cache.is_file():
        parser.error('Require CMakeCache.txt for FFmpeg-enabled build')
    cache = args.cmake_cache.read_text(errors='replace')
    for option in ('BINK_WITH_FFMPEG:BOOL=ON', 'BINK_FFMPEG_STATIC:BOOL=ON'):
        if option not in cache:
            parser.error(f'FFmpeg build not enabled: {option}')
    non_system = [name for name in imports if not is_windows_runtime_dll(name)]
    if non_system:
        parser.error('Unexpected runtime DLL dependencies: ' + ', '.join(non_system))

print(f'OK: PE32/x86; exports matched; {len(imports)} DLL dependencies: {", ".join(imports)}')
