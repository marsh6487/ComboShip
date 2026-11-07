#!/usr/bin/env python3
"""Native catalogue/presenter boundaries; real engine types, no game/archive execution."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
VIEWER = ROOT / 'soh/src/overlays/actors/ovl_En_Viewer'


def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + name + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing production function ' + name)
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == '{') - (source[index] == '}')
        index += 1
    return source[match.start():index] + '\n'


def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


def main():
    source = (VIEWER / 'z_en_viewer.c').read_text()
    native = (ROOT / 'soh/src/code/z_skelanime.c').read_text()
    fixture = (ROOT / 'tests/river_frogs_dampe/presenter_test.c').read_text()
    fixture = fixture.replace('/* PRODUCTION_INCLUDES */', source[:source.index('#define FLAGS')])
    fixture = fixture.replace('/* PRODUCTION_NATIVE_LIFETIME */', '\n'.join(
        function(native, name) for name in ['SkelAnime_InitFlex', 'SkelAnime_Free']))
    fixture = fixture.replace('/* PRODUCTION_INIT */', '\n'.join(function(source, name) for name in [
        'EnViewerStatic_GetAnimation', 'EnViewerStatic_InitSkeleton', 'EnViewerStatic_Init',
        'EnViewerStatic_WaitForObjects', 'EnViewerStatic_Update', 'EnViewer_Destroy']))
    draws = ['EnViewer_StaticRiverFrogOverrideLimbDraw', 'EnViewer_StaticRiverFrogPostLimbDraw',
             'EnViewer_DrawStaticRiverFrog', 'EnViewer_StaticDampeGhostPostLimbDraw',
             'EnViewer_DrawStaticDampeGhost']
    routing = function(source, 'EnViewerStatic_Draw')
    unrelated = sorted(set(re.findall(r'\b(EnViewer_Draw\w+|StaticStoryKokiri_Draw)\(this, play\)', routing))
                       - set(draws))
    fixture = fixture.replace('/* UNRELATED_DRAW_BOUNDARIES */', '\n'.join(
        ('void ' if name == 'StaticStoryKokiri_Draw' else 'static void ') + name +
        '(EnViewer* actor, PlayState* play) { REQUIRE(false); }'
        for name in unrelated))
    fixture = fixture.replace('/* PRODUCTION_DRAW */', '\n'.join(function(source, name) for name in draws) + routing)
    flags = ['-std=gnu11', '-g', '-O1', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0', '-ffunction-sections', '-fdata-sections',
             '-Wno-int-conversion', '-Wno-incompatible-pointer-types', '-Wno-discarded-qualifiers', '-Wno-discarded-array-qualifiers',
             '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-pie', '-no-pie',
             *['-I' + str(ROOT / p) for p in ('libultraship/include', 'soh/include', 'soh/src', 'soh/assets',
                                           'soh', 'soh/mods', 'combo', 'combo/menu')], '-I' + str(VIEWER)]
    with tempfile.TemporaryDirectory(prefix='river-frogs-dampe-') as directory:
        work = Path(directory)
        run('cc', *flags, ROOT / 'tests/river_frogs_dampe/catalogue_test.c', VIEWER / 'static_story_actor.c',
            '-o', work / 'catalogue')
        run(work / 'catalogue')
        test = work / 'viewer.c'
        test.write_text(fixture)
        for combo in (False, True):
            guards = ['-DCOMBO_BUILD'] if combo else []
            binary = work / ('viewer-combo' if combo else 'viewer-solo')
            run('cc', *flags, *guards, test, *[VIEWER / ('static_story_' + name + '.c')
                for name in ('actor', 'mm_actor', 'ganon')], '-Wl,--gc-sections', '-lm', '-o', binary)
            run(binary)
            run('cc', *flags, *guards, '-c', VIEWER / 'z_en_viewer.c', '-o',
                work / ('full-viewer-combo.o' if combo else 'full-viewer-solo.o'))
            print('PASS real native viewer headers and sanitized presenter: ' + ('Combo' if combo else 'solo SoH'),
                  flush=True)


if __name__ == '__main__':
    main()
