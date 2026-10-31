#!/usr/bin/env python3
"""Recommend the exact MinGW .def export-name spelling after a first link.

Some PE linkers normalize the source-language/COFF leading underscore.  This
probe inspects the actual PE export names and suggests a different public name
prefix *only* when all 66 ordinals match, with exactly a one-underscore shift.
The workflow then RE-LINKS with that .def and rechecks the finished DLL.
"""
# SPDX-License-Identifier: MIT
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
