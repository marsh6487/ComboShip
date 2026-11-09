#!/usr/bin/env python3
"""Run real UI input/cutscene bodies with each host's native types and guards."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise RuntimeError('Missing native function ' + name)
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end] + '\n'


def native_fixture(host):
    player_source = (ROOT / host / 'src/code/z_player_lib.c').read_text()
    play_source = (ROOT / host / 'src/code/z_play.c').read_text()
    input_source = (ROOT / host / 'src/code/game.c').read_text()
    bodies = []
    if host == 'soh':
        # This classifier is tiny; keeping the actual body also exercises all
        # six spell actions without a synthetic magic/cutscene predicate.
        bodies.append(function(player_source, 'Player_ActionToMagicSpell'))
    bodies += [function(player_source, 'Player_InBlockingCsMode'),
               function(player_source, 'Player_InCsMode'),
               function(play_source, 'Play_InCsMode'),
               function(input_source, 'GameState_UpdateUiVisibility')]
    fixture = (ROOT / 'tests/ui_visibility/control_input_test.c').read_text()
    marker = '/* PRODUCTION_CONTROL_FUNCTIONS */'
    if fixture.count(marker) != 1:
        raise RuntimeError('The native fixture insertion point is missing or duplicated')
    return fixture.replace(marker, '\n'.join(bodies))


def compiler_flags(host, combo):
    includes = [f'{host}/include', f'{host}/src', f'{host}/assets', host, f'{host}/mods',
                'libultraship/include', 'combo', 'combo/menu', '']
    cvars = ['CMake/soh-cvars.cmake']
    if host == 'mm':
        includes += ['mm/include/PR', 'mm/2s2h']
        cvars = ['libultraship/cmake/cvars.cmake', 'CMake/lus-cvars.cmake', 'CMake/2ship-cvars.cmake']
    flags = ['-std=gnu11', '-g', '-O1', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-Werror=implicit-function-declaration',
             '-Wno-incompatible-pointer-types', '-Wno-int-conversion', '-Wno-discarded-qualifiers',
             '-Wno-discarded-array-qualifiers']
    # The native MM headers redefine equivalent libultra button macros. Mark
    # imported headers as system headers so that known noise does not obscure
    # diagnostics in the extracted production functions or this fixture.
    flags += [argument for path in includes for argument in ('-isystem', str(ROOT / path))]
    for path in cvars:
        if (ROOT / path).exists():
            flags += [f'-D{name}="{value}"' for name, value in re.findall(
                r'set\((CVAR_\w+) "([^"\n]+)"\)', (ROOT / path).read_text())]
    if host == 'mm':
        flags += ['-DUI_CONTROL_TEST_MM', '-DNON_EQUIVALENT', '-DNON_MATCHING']
    if combo:
        flags += ['-DCOMBO_BUILD']
    return flags


def main():
    env = {**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'}
    with tempfile.TemporaryDirectory(prefix='native-ui-control-') as directory:
        for host in ('soh', 'mm'):
            source = Path(directory) / (host + '.c')
            source.write_text(native_fixture(host))
            for combo in (False, True):
                binary = Path(directory) / (host + ('-combo' if combo else '-original'))
                subprocess.run([os.environ.get('CC', 'cc'), *compiler_flags(host, combo), str(source),
                                '-lm', '-o', str(binary)], cwd=ROOT, env=env, check=True)
                subprocess.run([str(binary)], cwd=ROOT, env=env, check=True)
    print('PASS all four native-header UI input configurations (OoT/MM, original/Combo)')


if __name__ == '__main__':
    main()
