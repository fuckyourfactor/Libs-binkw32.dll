#!/usr/bin/env python3
"""Read and compare export name+ordinal manifest; optionally verify PE DLL."""
import json, pathlib, re, subprocess, sys
spec = json.loads(pathlib.Path(sys.argv[2]).read_text())
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
parsed=[]
for line in lines:
 m=re.fullmatch(r'\s*(_\w+@\d+)\s+@(\d+)\s*',line)
 if m:parsed.append(dict(name=m.group(1),ordinal=int(m.group(2))))
expected=[dict(name=x['name'],ordinal=x['ordinal']) for x in spec]
assert parsed == expected, 'DEF manifest differs from original PE exports'
print(f'OK: {len(parsed)} exact named/ordinal exports in .def')
if len(sys.argv)>3:
 result=subprocess.check_output(['objdump','-p',sys.argv[3]],text=True)
 body=result.split('[Ordinal/Name Pointer] Table -- Ordinal Base 1')[-1]
 actual=[]
 for line in body.splitlines():
  m=re.search(r'\+base\[\s*(\d+)\]\s+[0-9a-fA-F]+\s+(_\w+@\d+)',line)
  if m:actual.append(dict(name=m.group(2),ordinal=int(m.group(1))))
 assert actual == expected, f'Compiled DLL differs: {len(actual)} exports vs {len(expected)}'
 print(f'OK: compiled PE exports match original names and ordinals ({len(actual)})')
