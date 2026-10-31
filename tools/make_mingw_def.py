#!/usr/bin/env python3

import argparse
import json
from pathlib import Path
import re

PATTERN = re.compile(r'^\s*(_([A-Za-z][A-Za-z0-9_]*)@(\d+))\s+@(\d+)\s*$')

def generate(source: Path, manifest: Path, public_prefix: str = 'single') -> str:

    if public_prefix not in ('single', 'double', 'none'):
        raise ValueError(f'Unknown public name prefix: {public_prefix}')
    public_lead = {'single': '_', 'double': '__', 'none': ''}[public_prefix]
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
        f'    {public_lead}{x["name"][1:]} = {x["internal"]} @{x["ordinal"]}\n' for x in exports))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--public-prefix', choices=('single', 'double', 'none'), default='single')
    args = parser.parse_args()
    data = generate(args.source, args.manifest, args.public_prefix)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(data, encoding='ascii')
    print(f'Created GNU MinGW export aliases in {args.output}')

if __name__ == '__main__':
    main()
