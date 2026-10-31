#!/usr/bin/env python3
"""For clang+lld cross-builds with manually generated import archives only.

Only edits the import-name byte strings, never executable instructions.
The linker-generated import symbols carry @N, while Windows actually exports
these system functions without decorations. MinGW/MSVC SDK builds don't need it.
"""
import pathlib, re, sys
path=pathlib.Path(sys.argv[1]); blob=path.read_bytes()
for name, argc in {
 'CreateFileA':28, 'GetFileSize':8, 'ReadFile':20, 'CloseHandle':4,
 'GetTickCount':0, 'Sleep':4, 'GetDC':4, 'ReleaseDC':8,
 'StretchDIBits':52, 'ClientToScreen':8, 'GetClientRect':8,
 'waveOutOpen':24, 'waveOutPrepareHeader':12, 'waveOutUnprepareHeader':12,
 'waveOutWrite':12, 'waveOutReset':4, 'waveOutPause':4,
 'waveOutRestart':4, 'waveOutClose':4,
}.items():
 original=f'{name}@{argc}'.encode()+b'\0'
 clean=name.encode()+b'\0'+b'\0'*(len(original)-len(name)-1)
 matches=blob.count(original)
 if matches != 1:
  raise RuntimeError(f'{name}: expected exactly one PE import name; got {matches}')
 blob=blob.replace(original,clean)
path.write_bytes(blob)
print('PE import strings now match actual system DLL exports')
