#!/usr/bin/env python3
"""Audit the Soul of Majora routing boundaries and compile native API usage.

The executable recipe/consumer regressions live in the boss-model and foreign-soul
runners. This audit checks the name/dispatch boundaries those fixtures replace.
It does not prove archive selection or a configured in-game get-item cutscene.
"""
import os
from pathlib import Path
import re
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def read(path):
    return (ROOT / path).read_text()


def main():
    mm = read('combo/menu/ComboItemDrawMM.h')
    items = read('mm/2s2h/Rando/StaticData/Items.cpp')
    row = re.search(r'RI\(RI_SOUL_BOSS_MAJORA,[^\n]+', items).group()
    assert '"Soul of Majora"' in row and 'GID_NONE' in row
    dispatch = function(read('mm/2s2h/Rando/DrawItem.cpp'), 'Rando::DrawResolvedItem')
    assert re.search(r'case RI_SOUL_BOSS_MAJORA:\s*DrawMajora\(\);\s*break;', dispatch)
    assert 'RI_SOUL_BOSS_MAJORA' not in function(mm, 'MM_FillGidAliasDrawInfo')
    static = function(mm, 'MM_GetItemDrawInfo')
    assert static.index('MM_HasAnimDraw(id)') < static.index('MM_FillItemDrawInfo(id, out)')
    animated = function(mm, 'MM_GetItemAnimDrawInfo')
    assert animated.index('GetItemIdFromDisplayName(itemName)') < animated.index('MM_FillAnimDrawInfo(animId, out)')
    assert 'GetItemIdFromName(itemName)' in animated
    assert 'MM_FillBossSoulAnim(id, out)' in function(mm, 'MM_FillAnimDrawInfo')
    foreign = function(read('combo/menu/ComboForeignDrawOOT.h'), 'ComboFillForeignDrawInfo')
    assert foreign.index('sGetItemDrawInfo(drawName, &raw)') < foreign.index('sGetItemAnimDrawInfo(drawName, &info.anim)')
    assert 'info.animOk = true;' in foreign
    registry = read('soh/mods/extended_player.c')
    assert 'Player_InitDefaultIA, Randomizer_DrawMmSoul, RG_MM_SOUL_MAJORA,' in registry
    item_list = read('soh/soh/Enhancements/randomizer/item_list.cpp')
    assert 'setNeiDraw(RG_MM_SOUL_MAJORA);' in item_list
    for fleet in ['soh/soh/FleetShipCombo/FleetComboItems.h', 'mm/2s2h/FleetShipCombo/FleetComboItems.h']:
        assert re.search(r'X\(FCI_MM_SOUL_MAJORA, 1, FCI_F_NONE, RG_MM_SOUL_MAJORA, RI_SOUL_BOSS_MAJORA, "Soul of Majora"', read(fleet))
    assert '{ "Soul of Majora", RI_SOUL_BOSS_MAJORA }' in read('combo/menu/ComboMmNativeImports.h')
    native = read('soh/soh/Enhancements/randomizer/draw.cpp')
    assert re.search(r'RG_MM_SOUL_MAJORA, "objects/object_boss07/gMajorasMaskSkel", "objects/object_boss07/gMajorasMaskFloatingAnim", 1, 0,', native)
    print('PASS Majora friendly/raw identity, native dispatcher, animated export precedence and removal of remains alias')

    consumer = read('combo/menu/ComboForeignAnim.h')
    start = consumer.index('        if (info->postModelDlPath != nullptr) {', consumer.index('if (skelEntry.nonFlexSkeleton) {', consumer.index('// -- OPA/skeletal path')))
    end = consumer.index('        gSPComboRMPop(POLY_OPA_DISP++);', start)
    postpass = consumer[start:end]
    with tempfile.TemporaryDirectory(prefix='majora-syntax-') as temporary:
        for game in ['mm', 'soh']:
            unit = '#include <cstdint>\n'
            if game == 'mm':
                unit += '#include "ultra64.h"\n'
                for key in ['A', 'B', 'Z', 'START', 'DUP', 'DDOWN', 'DLEFT', 'DRIGHT', 'L', 'R', 'CUP', 'CDOWN', 'CLEFT', 'CRIGHT']:
                    unit += '#undef BTN_' + key + '\n'
                unit += '#include "global.h"\n'
            else:
                unit += '#include "z64.h"\n#include "macros.h"\n#include "functions.h"\n'
            unit += '#include "ComboItemDrawABI.h"\nGfx* CfaRouteLimbDList(Gfx*);\n'
            unit += 'void FinalModelPass(PlayState* play, const CwItemAnimDrawInfo* info) {\nOPEN_DISPS(play->state.gfxCtx);\n' + postpass + '\nCLOSE_DISPS(play->state.gfxCtx);\n}\n'
            if game == 'mm':
                unit += '#include "Rando/Types.h"\n'
                for obj in ['gameplay_keep', 'object_boss_hakugin', 'object_boss03', 'object_boss01', 'object_boss02', 'object_boss07']:
                    unit += '#include "objects/' + obj + '/' + obj + '.h"\n'
                unit += '\n'.join(function(mm, name) for name in ['MM_AnimSeg', 'MM_AnimTexSeg', 'MM_AnimSoulFlame', 'MM_FillBossSoulAnim'])
            path = Path(temporary) / (game + '.cpp')
            path.write_text(unit)
            includes = [game, game + '/include', game + '/include/PR', game + '/src', game + '/assets', game + '/2s2h', 'libultraship/include', 'libultraship/src', 'combo/menu']
            flags = ['-std=gnu++20', '-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCONTROLLERBUTTONS_T=uint32_t', '-DNON_EQUIVALENT', '-DNON_MATCHING', '-Wno-comma-subscript', '-fsyntax-only']
            subprocess.run([os.environ.get('CXX', 'c++'), *flags, *['-I' + str(ROOT / p) for p in includes], str(path)], check=True)
            print('PASS Majora production recipe/final material pass against actual ' + game + ' engine headers')


if __name__ == '__main__':
    main()
