#!/usr/bin/env python3

import re

WINDOWS_RUNTIME_DLLS = frozenset({
    'kernel32.dll', 'user32.dll', 'gdi32.dll', 'winmm.dll',
    'msvcrt.dll', 'ucrtbase.dll', 'advapi32.dll', 'bcrypt.dll',
    'ws2_32.dll', 'secur32.dll', 'ole32.dll', 'shell32.dll',
    'ntdll.dll', 'shlwapi.dll', 'version.dll', 'crypt32.dll',
    'rpcrt4.dll', 'kernelbase.dll',
})

API_SET_DLL = re.compile(r'(?:api-ms-win|ext-ms)-[a-z0-9_-]+\.dll', re.I | re.ASCII)

def is_windows_runtime_dll(name: str) -> bool:

    return name.casefold() in WINDOWS_RUNTIME_DLLS or API_SET_DLL.fullmatch(name) is not None
