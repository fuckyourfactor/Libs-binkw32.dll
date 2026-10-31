#!/usr/bin/env python3

from pathlib import Path
import struct

class PEExportError(ValueError):
    """Invalid, unsupported, or malformed PE export directory."""

def read_exports(path: str | Path) -> list[dict[str, int | str]]:
    data = Path(path).read_bytes()

    def unpack(fmt: str, off: int):
        if off < 0 or off + struct.calcsize(fmt) > len(data):
            raise PEExportError(f'PE read outside file at 0x{off:x} ({fmt})')
        return struct.unpack_from(fmt, data, off)

    if data[:2] != b'MZ':
        raise PEExportError('Not an MZ executable')
    pe_offset, = unpack('<I', 0x3c)
    if data[pe_offset:pe_offset + 4] != b'PE\0\0':
        raise PEExportError('Invalid PE signature')

    machine, section_count, _, _, _, opt_size, _ = unpack('<HHIIIHH', pe_offset + 4)
    if machine != 0x14c:
        raise PEExportError(f'Expected Win32 x86 (0x14c), got machine 0x{machine:x}')
    opt_off = pe_offset + 24
    magic, = unpack('<H', opt_off)
    if magic != 0x10b or opt_size < 104:
        raise PEExportError('Expected PE32 with export data directory')
    data_directory_count, = unpack('<I', opt_off + 92)
    if data_directory_count < 1:
        raise PEExportError('No export data directory')
    export_rva, export_size = unpack('<II', opt_off + 96)
    if not export_rva or not export_size:
        return []

    section_off = opt_off + opt_size
    sections = []
    for index in range(section_count):
        off = section_off + index * 40
        virtual_size, rva, raw_size, raw_pointer = unpack('<IIII', off + 8)
        sections.append((rva, max(virtual_size, raw_size), raw_pointer, raw_size))

    def rva_offset(rva: int, length: int) -> int:
        for start, span, file_start, raw_size in sections:
            if start <= rva and rva + length <= start + span:
                delta = rva - start
                if delta + length > raw_size:
                    raise PEExportError(f'RVA 0x{rva:x} extends beyond section raw data')
                offset = file_start + delta
                if offset + length > len(data):
                    raise PEExportError(f'RVA 0x{rva:x} extends beyond end of file')
                return offset
        raise PEExportError(f'RVA 0x{rva:x} is not backed by a PE section')

    def read_c_string(rva: int) -> str:
        offset = rva_offset(rva, 1)

        end = data.find(b'\x00', offset, min(len(data), offset + 4096))
        if end < 0:
            raise PEExportError(f'Unterminated PE export name at RVA 0x{rva:x}')
        rva_offset(rva, end - offset + 1)
        try:
            return data[offset:end].decode('ascii')
        except UnicodeDecodeError as exc:
            raise PEExportError('Non-ASCII PE export name') from exc

    directory = unpack('<IIHHIIIIIII', rva_offset(export_rva, 40))
    _, _, _, _, _, ordinal_base, function_count, name_count, funcs_rva, names_rva, ordinals_rva = directory
    if name_count > function_count or function_count > 100000:
        raise PEExportError('Inconsistent export function/name counts')
    if name_count == 0:
        return []
    names = []
    for i in range(name_count):
        name_rva, = unpack('<I', rva_offset(names_rva + i * 4, 4))
        ordinal_index, = unpack('<H', rva_offset(ordinals_rva + i * 2, 2))
        if ordinal_index >= function_count:
            raise PEExportError(f'Export ordinal index {ordinal_index} is out of range')
        function_rva, = unpack('<I', rva_offset(funcs_rva + ordinal_index * 4, 4))
        if not function_rva:
            raise PEExportError(f'Export {i} has a null target RVA')
        names.append({'name': read_c_string(name_rva), 'ordinal': ordinal_base + ordinal_index})
    return sorted(names, key=lambda item: (item['ordinal'], item['name']))
