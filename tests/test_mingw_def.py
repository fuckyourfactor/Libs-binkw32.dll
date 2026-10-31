#!/usr/bin/env python3
"""Assert exactly-preserved public names/ordinals and MinGW internal aliases."""
# SPDX-License-Identifier: MIT
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from make_mingw_def import generate  # noqa: E402


def main():
    entries = json.loads((ROOT / 'docs/exports.json').read_text(encoding='utf-8'))
    data = generate(ROOT / 'src/binkw32.def', ROOT / 'docs/exports.json')
    lines = data.splitlines()
    assert lines[:2] == ['LIBRARY binkw32', 'EXPORTS']
    assert len(lines) - 2 == len(entries) == 66
    for declaration, item in zip(lines[2:], entries):
        public = item['name']
        internal = public[1:]
        assert declaration.strip() == f'{public} = {internal} @{item["ordinal"]}', declaration
    print('OK: all 66 MinGW aliases match the source export manifest')


if __name__ == '__main__':
    main()
