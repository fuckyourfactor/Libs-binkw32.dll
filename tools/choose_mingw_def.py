#!/usr/bin/env python3

import json
from pathlib import Path
import sys
from pe_exports import read_exports

def recommend(reference, actual):
    if actual == reference:
        return 'verified'
    for candidate, transform in (
        ('double', lambda name: name[1:]),
        ('none', lambda name: '_' + name),
    ):
        if actual == [{**item, 'name': transform(item['name'])} for item in reference]:
            return candidate
    return None

def main():
    if len(sys.argv) != 3:
        raise SystemExit('Usage: choose_mingw_def.py exports.json compiled.dll')
    reference = [{'name': item['name'], 'ordinal': item['ordinal']}
                 for item in json.loads(Path(sys.argv[1]).read_text(encoding='utf-8'))]
    actual = read_exports(sys.argv[2])
    mode = recommend(reference, actual)
    if mode is None:
        raise SystemExit(
            f'Cannot safely correct PE exports: expected {len(reference)} named exports, '
            f'found {len(actual)}. Sample actual exports: {actual[:5]}. '
            'Do not publish this DLL.')
    print(mode)

if __name__ == '__main__':
    main()
