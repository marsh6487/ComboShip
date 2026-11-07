#!/usr/bin/env python3
"""Actual OoT render/input bodies against real engine types; no game execution."""
import os
from pathlib import Path
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    match = re.search(r'^[^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise RuntimeError('Missing native function ' + name)
    index, depth = match.end(), 1
    while depth:
        depth += (source[index] == '{') - (source[index] == '}')
        index += 1
    return source[match.start():index] + '\n'

def conditions(source):
    result = []
    for match in re.finditer(r'\bif\s*\(', source):
        index, depth = match.end(), 1
        while depth:
            depth += (source[index] == '(') - (source[index] == ')')
            index += 1
        result.append(source[match.end():index - 1].strip())
    return result

def run(*args):
    subprocess.run(list(map(str, args)), cwd=ROOT, check=True,
                   env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})

def main():
    flags = ['-std=gnu11', '-g', '-O1', '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DLOG_LEVEL_GAME_PRINTS=0', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
             '-fno-pie', '-no-pie', '-Wno-incompatible-pointer-types', '-Wno-int-conversion', '-Wno-discarded-qualifiers', '-Wno-discarded-array-qualifiers',
             *['-I' + str(ROOT / p) for p in ('soh/include','soh/src','soh/assets','soh','soh/mods',
                                            'libultraship/include','combo','combo/menu')]]
    flags += ['-D' + name + '="' + value + '"' for name, value in re.findall(
        r'set\((CVAR_PREFIX_\w+) "([^"\n]+)"\)', (ROOT / 'CMake/soh-cvars.cmake').read_text())]
    source = (ROOT / 'soh/src/code/z_play.c').read_text()
    fixture = (ROOT / 'tests/ui_visibility/oot_draw_test.c').read_text().replace(
        '/* PRODUCTION_OVERLAY */', function(source, 'Play_DrawOverlayElements'))
    input_source = (ROOT / 'soh/src/code/game.c').read_text()
    parameter = (ROOT / 'soh/src/code/z_parameter.c').read_text()
    minimap = (ROOT / 'soh/src/code/z_map_exp.c').read_text()
    input_fixture = (ROOT / 'tests/ui_visibility/oot_input_test.c').read_text().replace(
        '/* PRODUCTION_INPUT */', function(input_source, 'GameState_UpdateUiVisibility'))
    fade_start = parameter.index('    if (interfaceCtx->unk_244 != 0) {', parameter.index('void Interface_Draw('))
    fade_end = parameter.index('\n    }', fade_start) + len('\n    }')
    fixture = fixture.replace('/* PRODUCTION_INTERFACE_FADE */', parameter[fade_start:fade_end])
    full_ui = re.search(r'bool fullUi = .*?;', parameter, re.S).group(0)
    input_fixture = input_fixture.replace('/* PRODUCTION_FULL_UI */',
        'static int FullUi(PlayState* play) {' + full_ui + ' return fullUi; }')
    l_conditions = [condition for condition in conditions(minimap) if condition.startswith(
        'CHECK_BTN_ALL(play->state.input[0].press.button, BTN_L)')]
    assert len(l_conditions) == 2
    input_fixture = input_fixture.replace('/* PRODUCTION_MAP_L */', '\n'.join(
        'static int LegacyMapL' + str(index) + '(PlayState* play, bool enableMapToggle) { return ' + cond + '; }'
        for index, cond in enumerate(l_conditions)))
    map_conditions = [condition for condition in conditions(minimap) if condition.startswith(
        '(!R_MINIMAP_DISABLED')]
    assert len(map_conditions) == 2
    input_fixture = input_fixture.replace('/* PRODUCTION_MAP_VISIBILITY */', '\n'.join(
        'static int MapVisible' + str(index) + '(void) { return ' + cond + '; }'
        for index, cond in enumerate(map_conditions)))
    native_ui_fixture = (ROOT / 'tests/ui_visibility/oot_native_draw_test.c').read_text()
    ui_functions = [
        ('soh/src/overlays/gamestates/ovl_title/z_title.c', ['Title_Draw']),
        ('soh/src/code/z_actor.c', ['Attention_SetReticlePos', 'Attention_Draw', 'TitleCard_Draw']),
        ('soh/src/overlays/actors/ovl_End_Title/z_end_title.c', ['EndTitle_DrawFull','EndTitle_DrawNintendoLogo']),
        ('soh/src/overlays/actors/ovl_En_Mag/z_en_mag.c', ['EnMag_Draw']),
        ('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c', ['FileChoose_Main'])]
    native_ui_fixture = native_ui_fixture.replace('/* PRODUCTION_NATIVE_UI */', '\n'.join(
        function((ROOT / path).read_text(), name) for path, names in ui_functions for name in names))
    graph = (ROOT / 'soh/src/code/graph.c').read_text()
    native_ui_fixture = native_ui_fixture.replace('/* PRODUCTION_GRAPH_MARKERS */',
        function(graph, 'Graph_OpenDisps') + function(graph, 'Graph_CloseDisps'))
    clock_fixture = (ROOT / 'tests/ui_visibility/oot_hud_clock_test.cpp').read_text()
    for marker, path, name in [
        ('SM64_UPDATE','soh/expansions/sm64/Sm64CapsHud.cpp','UpdateElement'),
        ('TRIFORCE_TICK','soh/soh/Network/Harpoon/TriforceThief/TriforceThief.cpp','TickHud'),
        ('HARPOON_UPDATE','soh/soh/Network/Harpoon/HarpoonGamemodeHud.cpp','UpdateElement')]:
        clock_fixture = clock_fixture.replace('/* PRODUCTION_' + marker + ' */',
            function((ROOT / path).read_text(), name))
    with tempfile.TemporaryDirectory(prefix='oot-hidden-ui-') as directory:
        test = Path(directory) / 'native.c'
        test.write_text(fixture)
        for combo in (False, True):
            binary = Path(directory) / ('combo' if combo else 'solo')
            run('cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), test, '-lm', '-o', binary)
            run(binary)
            input_test = Path(directory) / 'input.c'
            input_test.write_text(input_fixture)
            run('cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), input_test, '-lm', '-o', binary)
            run(binary)
            native_ui_test = Path(directory) / 'native-ui.c'
            native_ui_test.write_text(native_ui_fixture)
            run('cc', *flags, *(['-DCOMBO_BUILD'] if combo else []), native_ui_test, '-lm', '-o', binary)
            run(binary)
        clock_test = Path(directory) / 'clock.cpp'
        clock_test.write_text(clock_fixture)
        clock_flags = [flag for flag in flags if flag != '-std=gnu11' and not flag.startswith('-Wno-')]
        run('c++', '-std=c++20', *clock_flags, clock_test, '-lm', '-o', Path(directory) / 'clock')
        run(Path(directory) / 'clock')

if __name__ == '__main__':
    main()
