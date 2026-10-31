#!/usr/bin/env python3
"""Create MinGW PE export aliases without changing the public ABI manifest.

PE32 COFF objects use the leading underscore on symbols (e.g. _BinkOpen@8).
GNU ld's .def parser adds that underscore to the *internal* alias automatically,
so referencing _BinkOpen@8 directly from the .def can look for a double
underscore and produce 'cannot export ... symbol not defined'.

The public export name stays _BinkOpen@8, with original ordinal; the internal
reference in the MinGW .def is BinkOpen@8.
"""
# SPDX-License-Identifier: MIT
import argparse
import json
from pathlib import Path
import re

PATTERN = re.compile(r'^\s*(_([A-Za-z][A-Za-z0-9_]*)@(\d+))\s+@(\d+)\s*$')


def generate(source: Path, manifest: Path) -> str:
    expected = json.loads(manifest.read_text(encoding='utf-8'))
    exports = []
    for line in source.read_text(encoding='utf-8').splitlines():
        stripped = line.strip()
        if not stripped or stripped.upper() in ('EXPORTS', 'LIBRARY BINKW32') or stripped.startswith(';'):
            continue
        match = PATTERN.fullmatch(line)
        if not match:
            raise ValueError(f'Unexpected .def declaration: {line}')
        public_name, base_name, stack_size, ordinal = match.groups()
        exports.append({'name': public_name, 'ordinal': int(ordinal),
                        'internal': f'{base_name}@{stack_size}'})
    if [{'name': x['name'], 'ordinal': x['ordinal']} for x in exports] != [
            {'name': x['name'], 'ordinal': x['ordinal']} for x in expected]:
        raise ValueError('Export manifest does not match .def (names/order/ordinals)')
    return ('LIBRARY binkw32\nEXPORTS\n' + ''.join(
        f'    {x["name"]} = {x["internal"]} @{x["ordinal"]}\n' for x in exports))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    data = generate(args.source, args.manifest)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(data, encoding='ascii')
    print(f'Created GNU MinGW export aliases in {args.output}')


if __name__ == '__main__':
    main()
