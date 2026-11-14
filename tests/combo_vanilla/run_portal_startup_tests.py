#!/usr/bin/env python3
"""Compile production portal/opening bodies; I/O, hook dispatch and scene loading are boundaries."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    begin = source.index('{', start)
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def read(path):
    return (ROOT / path).read_text(encoding='utf-8-sig')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--game', choices=('mm', 'oot'))
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    mm = read('mm/2s2h/BenPort.cpp')
    skip = read('mm/2s2h/Enhancements/Cutscenes/SkipIntroSequence.cpp')
    title = read('mm/src/code/title_setup.c')
    regional = read('combo/entrances/RegionalPassages.h')
    sram = read('mm/src/code/z_sram_NES.c')
    deferred_signature = 'extern "C" void MM_ApplyComboPortalAfterIntroSkip('
    mm_bodies = '\n'.join(function(regional, signature) for signature in (
        'static inline int ComboRegionalPassages_IsRoute(', 'static inline int ComboPassages_IsRoute('))
    mm_bodies += '\n' + function(title, 'static s32 ComboVanilla_PrepareMMOpening(')
    mm_bodies += '\n' + function(sram, 'void Sram_ClearFlagsAtDawnOfTheFirstDay(')
    mm_bodies += '\n' + function(sram, 'void Sram_IncrementDay(')
    mm_bodies += '\n' + function(mm, 'extern "C" void MM_ApplyComboPortalArrival(')
    mm_bodies += '\n' + function(mm, 'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(')
    if deferred_signature in mm:
        mm_bodies += '\n' + function(mm, deferred_signature)
    mm_bodies += '\n' + '\n'.join(line for line in skip.splitlines() if line.startswith('#define '))
    helper_signature = 'static void ApplyDeferredComboPortalAfterIntroSkip('
    mm_bodies += '\n' + function(skip, helper_signature)
    mm_bodies += '\n' + function(skip, 'void RegisterSkipIntroSequence(')
    mm_bodies = mm_bodies.replace('this->', 'self->')
    oot_title = read('soh/src/code/title_setup.c')
    entrances = read('soh/soh/Enhancements/randomizer/randomizer_entrance.c')
    oot_bodies = '\n'.join(function(entrances, signature) for signature in (
        's16 Entrance_GetOverride(', 'static s16 Entrance_TraceResolved(', 's16 Entrance_OverrideNextIndex('))
    oot_bodies += '\n' + function(oot_title, 'void TitleSetup_InitImpl(')
    flags = ['-std=c++20', '-Wall', '-Wextra', '-Wpedantic', '-Werror', '-Wno-unused-parameter',
             '-DCOMBO_BUILD', '-I', str(ROOT)]
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                  '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
    with tempfile.TemporaryDirectory(prefix='vanilla-portal-startup-') as temporary:
        for game, bodies in (('mm', mm_bodies), ('oot', oot_bodies)):
            if args.game and args.game != game:
                continue
            fixture = read('tests/combo_vanilla/portal_startup_' + game + '_test.cpp')
            source = Path(temporary) / (game + '.cpp')
            source.write_text(fixture.replace('/* PRODUCTION_BODIES */', bodies))
            binary = source.with_suffix('')
            subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags,
                            str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)

        if args.game != 'oot':
            # Compile the actual hook without COMBO_BUILD. It must neither need
            # the new bridge symbol nor change standalone MM's native landing.
            fixture = read('tests/combo_vanilla/portal_startup_mm_test.cpp').split('/* PRODUCTION_BODIES */')[0]
            native_skip = '\n'.join(line for line in skip.splitlines() if line.startswith('#define '))
            native_skip += '\n' + function(skip, helper_signature)
            native_skip += '\n' + function(skip, 'void RegisterSkipIntroSequence(')
            fixture += native_skip + r'''
int main() {
    gSaveContext = {}; gSaveContext.inventory.fill(0xFF);
    sComboPortalRoute = COMBO_PORTAL_LOST_WOODS; sComboPortalArrivalPending = true;
    RegisterSkipIntroSequence(); introHook();
    require(gSaveContext.save.entrance == 0xD800 && grants == 3 && saves == 1 &&
                sComboPortalRoute == COMBO_PORTAL_LOST_WOODS && sComboPortalArrivalPending &&
                gComboEntryIsResume == 0 && !titlePending,
            "standalone native skip must not invoke combined portal routing");
    std::puts("PASS standalone MM native intro skip and bridge linkage guard");
}
'''
            source = Path(temporary) / 'standalone.cpp'
            source.write_text(fixture)
            binary = source.with_suffix('')
            subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')),
                            *[flag for flag in flags if flag != '-DCOMBO_BUILD'],
                            str(source), '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
