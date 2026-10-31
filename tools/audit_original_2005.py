#!/usr/bin/env python3
"""Optional evidence-based ABI audit against user-supplied RAD Bink 1.8f DLL.

Does not bundle, load, or call proprietary binaries; the tool uses objdump to
check ABI-facing instruction operands and the PE import table only. The
positions are specific to the 2005 DLL and are not universal Bink SDK rules.
"""
# SPDX-License-Identifier: MIT
import argparse
import re
import subprocess
from pathlib import Path


def dump(*args: str) -> str:
    return subprocess.check_output(['objdump', *args], text=True, errors='replace')


def main() -> None:
    p = argparse.ArgumentParser()
    p.add_argument('original_dll', type=Path)
    p.add_argument('rfactor_exe', type=Path)
    a = p.parse_args()
    # RVA is grounded by the 2005 original PE32 export address table.
    dll = str(a.original_dll)
    dis_rect = dump('-D', '-M', 'intel', '-j', 'BINK', dll,
                    '--start-address=0x1800d190', '--stop-address=0x1800d510')
    dis_copy = dump('-D', '-M', 'intel', '-j', 'BINK', dll,
                    '--start-address=0x1800b670', '--stop-address=0x1800b700')
    evidence = [
        ('FrameRects +0x34', 'lea    edi,[esi+0x34]' in dis_rect),
        ('NumRects +0xB4', bool(re.search(r'(?:cmp|mov)\s+(?:DWORD PTR )?\[esi\+0xb4\]', dis_rect))),
        ('FrameBuffers +0xB8', 'mov    eax,DWORD PTR [esi+0xb8]' in dis_copy),
    ]
    pe = dump('-p', str(a.rfactor_exe))
    m = re.search(r'DLL Name:\s+binkw32\.dll\s*\n(.*?)(?=\n\s*DLL Name:|\n\s*PE File Base)', pe, re.I | re.S)
    if m is None:
        raise SystemExit('No binkw32.dll import found in rFactor.exe')
    names = sorted(set(re.findall(r'_Bink[A-Za-z0-9]+@\d+\b', m.group(1))))
    for label, ok in evidence:
        print(f'{"OK" if ok else "FAIL"}: 2005 original {label}')
    print(f'rFactor import count: {len(names)}')
    print('rFactor pixel handoff imports:',
          {n for n in names if any(k in n for k in ('CopyToBuffer', 'GetRects', 'DoFrame'))})
    if not all(ok for _, ok in evidence) or len(names) != 16:
        raise SystemExit('Incompatible sample or ABI evidence missing')


if __name__ == '__main__':
    main()
