#!/usr/bin/env python3
# Symbolizes a botrace report: replaces module+0xoffset with function names (addr2line).
import os, re, subprocess, sys

SEARCH = sys.argv[2:] or ['.']
cache = {}

def find_module(name):
    for root in SEARCH + ['/usr/lib/x86_64-linux-gnu', '/jhbuild/install/lib']:
        for candidate in (os.path.join(root, name), os.path.join(root, 'lib', name)):
            if os.path.exists(candidate):
                return candidate
    return None

def symbolize(module, offset):
    key = (module, offset)
    if key in cache:
        return cache[key]
    path = find_module(module)
    name = f'{module}+{offset}'
    if path:
        out = subprocess.run(['addr2line', '-f', '-C', '-e', path, offset], capture_output=True, text=True).stdout.split('\n')
        if out and out[0] and out[0] != '??':
            name = re.sub(r'\(.*\)', '', out[0])
    cache[key] = name
    return name

for line in open(sys.argv[1]):
    print(re.sub(r'([\w.+-]+\.so[\w.]*)\+(0x[0-9a-f]+)', lambda m: symbolize(m.group(1), m.group(2)), line), end='')
