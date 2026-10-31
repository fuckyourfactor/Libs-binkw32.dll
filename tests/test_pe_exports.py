#!/usr/bin/env python3

import json
from pathlib import Path
import struct
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from pe_exports import PEExportError, read_exports
from choose_mingw_def import recommend

def make_pe(exports):

    data = bytearray(0x5000)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, 0x14c, 1, 0, 0, 0, 0xe0, 0x2102)
    opt = 0x98
    struct.pack_into('<H', data, opt, 0x10b)
    struct.pack_into('<I', data, opt + 92, 16)
    struct.pack_into('<II', data, opt + 96, 0x1000, 0x2800)
    section = opt + 0xe0
    data[section:section + 8] = b'.edata\0\0'
    struct.pack_into('<IIII', data, section + 8, 0x3000, 0x1000, 0x3000, 0x200)
    ordinal_base = min(x[1] for x in exports)
    funcs = 0x1100
    names_rva = 0x1300
    ordinals = 0x1500
    str_rva = 0x1600

    def write(rva, fmt, *args):
        struct.pack_into(fmt, data, 0x200 + rva - 0x1000, *args)

    write(0x1000, '<IIHHIIIIIII', 0, 0, 0, 0, 0x13f0,
          ordinal_base, max(x[1] for x in exports) - ordinal_base + 1,
          len(exports), funcs, names_rva, ordinals)
    for index, (name, ordinal) in enumerate(exports):
        write(funcs + (ordinal - ordinal_base) * 4, '<I', 0x2000 + 16 * index)
        write(names_rva + 4 * index, '<I', str_rva)
        write(ordinals + 2 * index, '<H', ordinal - ordinal_base)
        encoded = name.encode('ascii') + b'\0'
        off = 0x200 + str_rva - 0x1000
        data[off:off + len(encoded)] = encoded
        str_rva += len(encoded)
    return bytes(data)

def main():
    expected = [{'name': '_BinkClose@4', 'ordinal': 16},
                {'name': '_BinkOpen@8', 'ordinal': 41}]
    with tempfile.TemporaryDirectory() as directory:
        dll = Path(directory) / 'binkw32.dll'
        dll.write_bytes(make_pe([(x['name'], x['ordinal']) for x in expected]))
        assert read_exports(dll) == expected
        assert recommend(expected, read_exports(dll)) == 'verified'

        missing = [(x['name'][1:], x['ordinal']) for x in expected]
        dll.write_bytes(make_pe(missing))
        assert recommend(expected, read_exports(dll)) == 'double'

        extra = [('_' + x['name'], x['ordinal']) for x in expected]
        dll.write_bytes(make_pe(extra))
        assert recommend(expected, read_exports(dll)) == 'none'

        dll.write_bytes(make_pe([('_Other@4', 16), ('_BinkOpen@8', 41)]))
        assert recommend(expected, read_exports(dll)) is None
        dll.write_bytes(make_pe([('_BinkClose@4', 17), ('_BinkOpen@8', 41)]))
        assert recommend(expected, read_exports(dll)) is None

        dll.write_bytes(b'MZ' + b'\0' * 10)
        try:
            read_exports(dll)
            raise AssertionError('Truncated PE accepted')
        except PEExportError:
            pass

    dll = ROOT / 'bin/binkw32.dll'
    if dll.exists():
        reference = [{'name': x['name'], 'ordinal': x['ordinal']}
                     for x in json.loads((ROOT / 'tools/exports.json').read_text())]
        assert read_exports(dll) == reference
    print('OK: PE32 export parsing, ordinals, MinGW prefix diagnosis, errors')

if __name__ == '__main__':
    main()
