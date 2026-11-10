#!/usr/bin/env python3
"""Compile the real cache/resolver implementations from both receiving hosts."""
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
def block(source, start):
    begin = source.index(start)
    brace = source.index('{', begin)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    if source[end:end+1] == ';': end += 1
    return source[begin:end]
with tempfile.TemporaryDirectory(prefix='soul-cache-') as tmp:
    build = Path(tmp)
    code = ''
    for host, suffix in [('OOT', ''), ('MM', 'OOT')]:
        source = (ROOT / f'combo/menu/ComboForeignDraw{host}.h').read_text()
        cache = 'ComboForeignDrawCache' + suffix
        code += block(source, 'struct ' + cache + ' {') + '\n'
        code += block(source, f'inline {cache}& {cache}Get()') + '\n'
        code += block(source, f'inline const ComboForeignDrawInfo{suffix}* ComboResolveForeignDrawInfo{suffix}(') + '\n'
    (build / 'renderer_cache_production.inc').write_text(code)
    binary = build / 'cache'
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-DCOMBO_BUILD', '-I'+str(build), '-I'+str(ROOT/'combo/menu'), str(ROOT/'tests/soul_mini_models/renderer_cache_test.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
