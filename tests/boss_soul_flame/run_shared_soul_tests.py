#!/usr/bin/env python3
"""Execute native added-soul callbacks and both foreign handlers with real GBI."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/temple_sages'))
from resource_geometry import crc64, words


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}', start) + 2]


def branch(source, condition):
    start = source.index(condition)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def resource_dependencies():
    paths = {
        'OoT-flame-DL': ('objects/object_gi_fire/object_gi_fireTex_000000',
                         'objects/object_gi_fire/object_gi_fireTex_000200'),
        'MM-flame-DL': ('objects/gameplay_keep/gameplay_keep_Tex_01A8B0',
                        'objects/gameplay_keep/gameplay_keep_Tex_01AAB0'),
    }
    for filename, textures in paths.items():
        data = (ROOT / 'tools/boss_soul_flames/native' / filename).read_bytes()
        hashes = {dependency for _, _, _, dependency in words(data)}
        assert all(crc64(path) in hashes for path in textures)
    print('PASS actual native flame DL dependencies match both existing two-texture Alt packs')


def main():
    item_list = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
    assert 'SetCustomDrawFunc(Randomizer_DrawShuffledSoul)' in item_list, \
        'added OoT soul catalogue has no native soul callback'
    resource_dependencies()
    oot = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
    mm = (ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text()
    mm_flame = (ROOT / 'mm/2s2h/Rando/DrawFuncs.cpp').read_text()
    template = (ROOT / 'tests/boss_soul_flame/shared_soul_native_test.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='shared-soul-native-') as temporary:
        for host in ('soh', 'mm'):
            foreign_file = 'ComboForeignDrawOOT.h' if host == 'soh' else 'ComboForeignDrawMM.h'
            foreign = (ROOT / 'combo/menu' / foreign_file).read_text()
            producer = (ROOT / 'combo/menu' / ('ComboItemDrawOOT.h' if host == 'soh' else 'ComboItemDrawMM.h')).read_text()
            info = 'ComboForeignDrawInfo' if host == 'soh' else 'ComboForeignDrawInfoOOT'
            start = foreign.index('struct ' + info + ' {')
            production = foreign[start:foreign.index('\n};', start) + 3] + '\n'
            pin = '#define OOT_FOREIGN_PIN_OPA' if host == 'soh' else '#define MM_FOREIGN_PIN_OPA'
            start = foreign.index(pin)
            production += foreign[start:foreign.index('\n\n//', start)] + '\n'
            if host == 'soh':
                production += '#define COMBO_FOREIGN_MTX(packet) gSPMatrix(packet, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD)\n'
                production += '#include "ComboSoulGiFlameOOT.h"\n'
                production += function(oot, 'extern "C" void Randomizer_DrawShuffledSoul(') + '\n'
                production += function(foreign, 'inline void OOT_RestoreForeignSegs(') + '\n'
                production += function(foreign, 'inline void OOT_DrawForeignShuffledSoul(') + '\n'
                production += 'int DescribeAddedSoul(RandomizerGet rg, CwItemDrawInfo* out) {\n'
                production += branch(producer, 'if (Rando::SoulShuffle::Find(rg)) {') + '\nreturn 0; }\n'
            else:
                production += function(mm_flame, 'static void DrawSoulFlame(') + '\n'
                production += function(mm_flame, 'void DrawOotSoulFlame(') + '\n'
                production += function(mm, 'void DrawShuffledSoul(') + '\n'
                production += function(foreign, 'inline void MM_DrawForeignShuffledSoul(') + '\n'
                production += 'int DescribeAddedSoul(RandoItemId id, CwItemDrawInfo* out) {\n'
                production += branch(producer, 'if (Rando::SoulShuffle::Find(id)) {') + '\nreturn 0; }\n'
            cpp = Path(temporary) / (host + '.cpp')
            cpp.write_text(template.replace('/* PRODUCTION */', production))
            binary = Path(temporary) / host
            flags = ['-std=c++20', '-g', '-O0', '-DF3DEX_GBI_2', '-DCOMBO_BUILD',
                     '-DCONTROLLERBUTTONS_T=uint32_t', '-DHOST_MM=' + str(int(host == 'mm')),
                     '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                     '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
            flags += ['-I' + str(ROOT / path) for path in
                      ('.', host, host + '/include', host + '/include/PR', host + '/src', host + '/assets',
                       'libultraship/include', 'combo', 'combo/menu')]
            if host == 'mm':
                flags += ['-I' + str(ROOT / 'mm/2s2h')]
            subprocess.run([os.environ.get('CXX', 'c++'), *flags, str(cpp), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
