#!/usr/bin/env python3
"""Run native MM worn-mask drawing and recovery against real engine types."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests/ui_visibility'))
from run_control_input_tests import compiler_flags, function


def block(source, start):
    opening = source.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def run_native(name, source):
    with tempfile.TemporaryDirectory(prefix='mm-mask-effects-') as directory:
        path = Path(directory) / (name + '.c')
        path.write_text(source)
        for combo in (False, True):
            binary = Path(directory) / ('combo' if combo else 'original')
            flags = compiler_flags('mm', combo)
            flags += ['-std=gnu17', '-fno-sanitize=shift-base']
            subprocess.run([os.environ.get('CC', 'cc'), *flags, str(path), '-lm', '-o', str(binary)],
                           check=True, cwd=ROOT)
            subprocess.run([str(binary)], check=True, cwd=ROOT,
                           env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
            print(f'PASS {name}: MM {"Combo" if combo else "original"}', flush=True)


def visibility():
    source = (ROOT / 'mm/src/code/z_player_lib.c').read_text()
    callback = function(source, 'Player_PostLimbDrawGameplay')
    start = callback.index('        if (((*dList1 != NULL)')
    worn_draw = block(callback, start)
    persistent = (ROOT / 'mm/2s2h/Enhancements/Masks/PersistentMasks.cpp').read_text()
    start = persistent.index('            if (!gSaveContext.save.shipSaveInfo.persistentBunnyHood)')
    prefix = persistent[start:persistent.index('            OPEN_DISPS(', start)]
    fixture = (ROOT / 'tests/mm_mask_effects/visibility_test.c').read_text()
    fixture = fixture.replace('/* NATIVE_SEGMENT_LOADER */', function(source, 'func_80127438'))
    fixture = fixture.replace('/* NATIVE_WORN_DRAW */', worn_draw)
    fixture = fixture.replace('/* PERSISTENT_WORN_PREFIX */', prefix)
    # The common segment loader also supplies models held during equip/unequip.
    # Running that unmodified body proves the option does not hide those models.
    run_native('visibility', fixture)


def recovery():
    source = (ROOT / 'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    update = function(source, 'Player_Update')
    assert update.count('Player_UpdateCouplesMask(this, play);') == 1, 'Missing native Couple\'s Mask update'
    assert update.index('Player_UpdateCommon(this, play, &input);') < update.index('Player_UpdateCouplesMask(this, play);')
    fixture = (ROOT / 'tests/mm_mask_effects/recovery_test.c').read_text()
    run_native('recovery', fixture.replace('/* NATIVE_RECOVERY */', function(source, 'Player_UpdateCouplesMask')))


def menu():
    source = (ROOT / 'mm/2s2h/BenGui/BenMenu.cpp').read_text()
    registrations = re.findall(
        r'    AddWidget\((path|masksPath), "Invisible Non-Transformation Masks", WIDGET_CVAR_CHECKBOX\).*?;',
        source, re.S)
    assert registrations == ['path', 'masksPath'], 'Missing MM visibility checkbox in native and NEI mask menus'
    blocks = re.findall(
        r'    AddWidget\((?:path|masksPath), "Invisible Non-Transformation Masks", WIDGET_CVAR_CHECKBOX\).*?;',
        source, re.S)
    fixture = (ROOT / 'tests/mm_mask_effects/menu_test.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='mm-mask-menu-') as directory:
        path = Path(directory) / 'menu.cpp'
        path.write_text(fixture.replace('/* NATIVE_WIDGETS */', '\n'.join(blocks)))
        binary = Path(directory) / 'menu'
        subprocess.run(['c++', '-std=c++20', str(path), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    for path in ('soh/soh/SohGui/SohMenuNEI.cpp', 'soh/soh/SohGui/SohMenuEnhancements.cpp'):
        assert '.CVar(CVAR_ENHANCEMENT("HideNonTransformationMasks"))' in (ROOT / path).read_text()
    print('PASS MM native/NEI checkboxes share the existing OoT setting and disabled default', flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('case', choices=('visibility', 'recovery', 'menu', 'all'), default='all', nargs='?')
    args = parser.parse_args()
    for case in ('menu', 'visibility', 'recovery') if args.case == 'all' else (args.case,):
        globals()[case]()
