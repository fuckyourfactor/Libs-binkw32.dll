#!/usr/bin/env python3
"""Verify .def manifest and exact compiled PE export names/ordinals.

Reads the actual export directory, not platform-dependent objdump formatting.
"""
# SPDX-License-Identifier: MIT
import json
from pathlib import Path
import re
import sys

from pe_exports import PEExportError, read_exports


def main():
    if not 3 <= len(sys.argv) <= 4:
        raise SystemExit('Usage: check_def.py ORIGINAL.def exports.json [binkw32.dll]')
    manifest = json.loads(Path(sys.argv[2]).read_text(encoding='utf-8'))
    expected = [{'name': x['name'], 'ordinal': x['ordinal']} for x in manifest]
    parsed = []
    for line in Path(sys.argv[1]).read_text(encoding='utf-8').splitlines():
        match = re.fullmatch(r'\s*(_\w+@\d+)\s+@(\d+)\s*', line)
        if match:
            parsed.append({'name': match.group(1), 'ordinal': int(match.group(2))})
    if parsed != expected:
        raise AssertionError('Original .def does not match the reference export manifest')
    print(f'OK: {len(parsed)} exact named/ordinal exports in .def')
    if len(sys.argv) == 4:
        actual = read_exports(sys.argv[3])
        if actual != expected:
            expected_by_ordinal = {x['ordinal']: x['name'] for x in expected}
            actual_by_ordinal = {x['ordinal']: x['name'] for x in actual}
            differences = [
                f'  ordinal @{ordinal}: expected {name!r}, found {actual_by_ordinal.get(ordinal)!r}'
                for ordinal, name in expected_by_ordinal.items()
                if actual_by_ordinal.get(ordinal) != name
            ]
            extra = [x for x in actual if x['ordinal'] not in expected_by_ordinal]
            print(f'ERROR: compiled PE has {len(actual)} named exports, expected {len(expected)}.', file=sys.stderr)
            if not actual:
                print('  Export table has no named exports.', file=sys.stderr)
            else:
                print('\n'.join(differences[:8]), file=sys.stderr)
                if extra:
                    print(f'  Extra export examples: {extra[:4]}', file=sys.stderr)
                if len(differences) > 8:
                    print(f'  ... plus {len(differences) - 8} additional mismatches', file=sys.stderr)
            raise SystemExit(1)
        print(f'OK: compiled PE exports match original names and ordinals ({len(actual)})')


if __name__ == '__main__':
    try:
        main()
    except (PEExportError, OSError, ValueError) as error:
        raise SystemExit(f'Export validation error: {error}') from error
