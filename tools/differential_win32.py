#!/usr/bin/env python3
"""Optional conservative differential smoke test on 32-bit Windows Python.

Usage: py -3-32 tools/differential_win32.py C:\\original\\binkw32.dll C:\\new\\binkw32.dll [file.bik]
Tests are intentionally non-destructive. Outputs differences; never asserts parity.
Copies both DLLs to distinct basenames because Win32 loader caches module names.
"""
import ctypes
import os
import pathlib
import shutil
import sys
import tempfile

if os.name != 'nt' or ctypes.sizeof(ctypes.c_void_p) != 4:
    sys.exit('This optional differential test requires 32-bit Python on Windows')
if len(sys.argv)<3:
    sys.exit('Usage: differential_win32.py ORIGINAL_DLL REPLACEMENT_DLL [LEGAL_SAMPLE.BIK]')

class Prefix(ctypes.Structure):
    _fields_ = [(x,ctypes.c_uint32) for x in ('Width','Height','Frames','FrameNum','LastFrameNum','FrameRate','FrameRateDiv')]

with tempfile.TemporaryDirectory(prefix='bink_diff_') as tmp:
    paths=[pathlib.Path(sys.argv[1]).resolve(),pathlib.Path(sys.argv[2]).resolve()]
    dlls=[]
    for i,path in enumerate(paths):
        target=pathlib.Path(tmp)/('original_sample.dll' if i==0 else 'replacement_sample.dll')
        shutil.copy2(path,target)
        dlls.append(ctypes.WinDLL(str(target)))
    def api(lib,name,restype,args):
        f=getattr(lib,'_'+name+'@'+str(len(args)*4))
        f.restype=restype
        f.argtypes=args
        return f
    for label,dll in zip(('original','replacement'),dlls):
        set_error=api(dll,'BinkSetError',None,[ctypes.c_char_p])
        get_error=api(dll,'BinkGetError',ctypes.c_char_p,[])
        set_error(b'INTEROPERABILITY_TEST')
        print(label,'BinkGetError =>',get_error())
        timer=api(dll,'RADTimerRead',ctypes.c_uint32,[])
        print(label,'RADTimerRead =>',timer(),timer())
        open_movie=api(dll,'BinkOpen',ctypes.c_void_p,[ctypes.c_char_p,ctypes.c_uint32])
        close_movie=api(dll,'BinkClose',None,[ctypes.c_void_p])
        source=pathlib.Path(sys.argv[3]).resolve() if len(sys.argv)>3 else None
        if source is not None:
            h=open_movie(os.fsencode(source),0)
            print(label,'BinkOpen =>',bool(h))
            if h:
                pre=ctypes.cast(h,ctypes.POINTER(Prefix)).contents
                print(label,'public fields =>',tuple(getattr(pre,x) for x,_ in Prefix._fields_))
                close_movie(h)
            else:
                print(label,'error =>',get_error())
