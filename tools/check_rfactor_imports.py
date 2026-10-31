#!/usr/bin/env python3
"""Static PE import compatibility audit. Does not execute rFactor.exe.

Usage: python3 tools/check_rfactor_imports.py /path/to/rFactor.exe
Requires objdump (binutils); does not redistribute the executable.
"""
import argparse
import json
from pathlib import Path
import re
import subprocess

ap = argparse.ArgumentParser()
ap.add_argument('exe', type=Path)
args = ap.parse_args()
project = Path(__file__).resolve().parent.parent
expected = {item['name'] for item in json.loads((project/'docs/exports.json').read_text())}
output = subprocess.check_output(['objdump','-p',str(args.exe)], text=True, errors='replace')
match = re.search(r'\bDLL Name:\s*binkw32\.dll\b(.*?)(?=\n\s*DLL Name:|\Z)', output, re.I|re.S)
if not match:
    ap.error('No binkw32.dll imports found')
imports = sorted(set(re.findall(r'^\s*[\da-fA-F]+\s+<none>\s+[\da-fA-F]+\s+(_Bink\w+@\d+)\s*$', match.group(1), re.M)))
missing = sorted(set(imports)-expected)
print('rFactor.exe Bink imports:', len(imports))
for name in imports:
    print(('MISSING' if name in missing else 'OK') + '  ' + name)
if not imports or missing:
    raise SystemExit(1)
print('PASS: every statically imported Bink function is exported by bink-open.')
print('Note: this does not verify semantics, binary layouts, decoding or playback.')
