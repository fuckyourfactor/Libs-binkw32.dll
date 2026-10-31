#!/usr/bin/env python3

import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from make_mingw_def import generate

def main():
    entries = json.loads((ROOT / 'tools/exports.json').read_text(encoding='utf-8'))
    data = generate(ROOT / 'src/binkw32.def', ROOT / 'tools/exports.json')
    lines = data.splitlines()
    assert lines[:2] == ['LIBRARY binkw32', 'EXPORTS']
    assert len(lines) - 2 == len(entries) == 66
    for declaration, item in zip(lines[2:], entries):
        public = item['name']
        internal = public[1:]
        assert declaration.strip() == f'{public} = {internal} @{item["ordinal"]}', declaration
    for prefix, mark in (('double', '__'), ('none', '')):
        adjusted = generate(ROOT / 'src/binkw32.def', ROOT / 'tools/exports.json', prefix)
        lines = adjusted.splitlines()
        assert len(lines) - 2 == 66
        for declaration, item in zip(lines[2:], entries):
            assert declaration.strip() == (
                f'{mark}{item["name"][1:]} = {item["name"][1:]} @{item["ordinal"]}'
            ), declaration
    print('OK: all 66 MinGW aliases for single/double/none public prefixes')

if __name__ == '__main__':
    main()
