#!/usr/bin/env python3
"""Execute wand modes against native MM headers and production function bodies.

The fixture replaces resource/archive, heap and presentation boundaries. Native
spawn/init/kill, mode dispatch, input/magic gates and each tested mode stay real.
"""
import re
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_REF = None
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags


def source(path):
    if SOURCE_REF:
        return subprocess.check_output(['git', 'show', SOURCE_REF + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def functions(text, names=None):
    # Native actor spawn contains an old commented-out opening brace. Ignore
    # comments and string literals for balancing, while retaining original code.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
        lambda match: re.sub(r'[^\n]', ' ', match[0]), text, flags=re.S)
    pattern = r'^(?:static[ \t]+)?[A-Za-z_][\w *]*[ \t]+(\w+)\s*\([^;{}]*\)\s*\{'
    found = {}
    for match in re.finditer(pattern, masked, re.M):
        if names is not None and match[1] not in names:
            continue
        end, depth = match.end(), 1
        while depth:
            depth += (masked[end] == '{') - (masked[end] == '}')
            end += 1
        found[match[1]] = text[match.start():end]
    return found


def bodies(path, names):
    found = functions(source(path), names)
    return '\n'.join(re.sub(r'\bthis\b', 'self', found[name]) for name in names)


def declaration(path, start):
    text = source(path)
    begin = text.index(start)
    end = begin + re.search(r'\n}[^;\n]*;', text[begin:]).end()
    return text[begin:end]


def main():
    global SOURCE_REF
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=['native-storm', 'shadow', 'modes', 'oot-sand'])
    parser.add_argument('--source-ref', help='run the same expectations against an earlier source revision')
    args = parser.parse_args()
    SOURCE_REF = args.source_ref
    actor = 'mm/src/code/z_actor.c'
    storm = 'mm/src/overlays/actors/ovl_En_Okarina_Effect/z_en_okarina_effect.c'
    storm_functions = functions(source(storm))
    storm_body = '\n'.join(body[:body.index('{')] + ';' for body in storm_functions.values())
    storm_body += '\n' + re.search(r'ActorProfile En_Okarina_Effect_Profile = \{.*?\};', source(storm), re.S)[0]
    storm_body += '\n' + '\n'.join(storm_functions.values())
    storm_body = re.sub(r'\bthis\b', 'self', storm_body)

    production = bodies(actor, ['Actor_Kill', 'Actor_SetWorldToHome', 'Actor_SetShapeRotToWorld',
        'Actor_SetFocus', 'Actor_SetScale', 'Actor_SetObjectDependency', 'Actor_Init', 'Actor_AddToCategory',
        'Actor_SpawnAsChildAndCutscene', 'Actor_Spawn'])
    production += '\n' + bodies('mm/src/code/z_scene.c', ['Object_GetSlot', 'Object_IsLoaded'])
    production += '\n' + storm_body
    production += '\n' + bodies('mm/mods/items/logic/wand/wand_storm.c', ['WandStorm_CallStorm'])

    shadow_path = 'mm/mods/items/logic/wand/wand_shadow.c'
    shadow = re.sub(r'^#include[^\n]*\n', '', source(shadow_path), flags=re.M)
    shadow = shadow.replace(functions(shadow)['WandShadow_Draw'], '')
    shadow = bodies('mm/mods/items/helpers/target_select_helper.c', ['TargetSelect_FindNearest']) + '\n' + shadow

    helper = source('mm/mods/items/helpers/equip_helper.c')
    modes = helper[helper.index('typedef struct {'):helper.index('u8 ItemEquip_GetItemOnSlot')]
    modes += '\n' + bodies('mm/mods/items/helpers/equip_helper.c', ['ItemEquip_GetItemOnSlot', 'EquipCache_Update',
        'ItemInput_GetEquippedButton', 'ItemInput_Update', 'ItemInput_CheckDamage', 'ItemInput_CheckOtherButtons',
        'ItemInput_IsBlockedEx', 'ItemInput_IsBlocked', 'ItemMagic_IsInfinite', 'ItemMagic_Consume', 'ItemMagic_HasEnough'])
    inventory = 'mm/mods/extended_inventory.c'
    modes += '\n' + re.search(r'static const uint8_t sWandQuest\[WAND_MODE_COUNT\] = \{.*?\};', source(inventory), re.S)[0]
    modes += '\n' + bodies(inventory, ['Wand_RandoMode', 'Wand_ModeOwned', 'Wand_ModeCount', 'Wand_ModeAt',
        'Wand_GetMode', 'Wand_SetMode'])
    modes += '\n' + bodies('mm/mods/items/helpers/target_select_helper.c', ['TargetSelect_FindNearest'])
    modes += '\n' + bodies('mm/src/code/z_actor.c', ['Actor_SetScale', 'Flags_GetSwitch', 'Flags_SetSwitch', 'Flags_UnsetSwitch'])
    modes += '\n' + bodies('mm/src/code/z_scene.c', ['Object_GetSlot', 'Object_IsLoaded'])
    modes += '\n' + bodies('mm/src/code/z_player_lib.c', ['Player_CheckHostileLockOn', 'Player_FriendlyLockOnOrParallel'])
    modes += '\n' + bodies('mm/src/overlays/actors/ovl_player_actor/z_player.c', ['Player_IsZTargeting'])
    modes += '\n' + bodies('mm/src/code/sys_matrix.c', ['Matrix_MtxFCopy', 'Matrix_Push', 'Matrix_Pop',
        'Matrix_RotateYF', 'Matrix_RotateXF', 'Matrix_MultVec3f'])
    modes += '\n' + bodies('mm/mods/items/helpers/combat_helper.c', ['Combat_InitCylinder',
        'Combat_UpdateCylinder', 'Combat_RegisterCollider'])

    wand = source('mm/mods/items/logic/item_elemental_wand.c')
    for slug in ['sand', 'wind', 'water', 'meteor', 'storm', 'shadow']:
        mode = source('mm/mods/items/logic/wand/wand_' + slug + '.c')
        # Drawing is a boundary: the gameplay cast/update bodies remain verbatim.
        for name, function in functions(mode).items():
            if 'Draw' in name:
                mode = mode.replace(function, '')
        wand = wand.replace('#include "wand/wand_' + slug + '.c"', mode)
    wand = wand.replace(functions(wand)['Wand_Draw'], '')
    wand = re.sub(r'^#include[^\n]*\n', '', wand, flags=re.M)
    modes += '\n' + re.sub(r'\bthis\b', 'self', wand)

    player_path = 'mm/src/overlays/actors/ovl_player_actor/z_player.c'
    lib_path = 'mm/src/code/z_player_lib.c'
    extended = 'mm/mods/extended_player.c'
    for path, start in [(player_path, 's8 sItemItemActions['), (lib_path, 'u8 sActionModelGroups['),
            (lib_path, 'PlayerModelIndices gPlayerModelTypes['), (player_path, 'typedef enum ItemChangeType {'),
            (player_path, 's8 sPlayerItemChangeTypes[')]:
        modes += '\n' + declaration(path, start)
    # This registry slice retains the actual columns consumed by native item
    # action/model dispatch. Other rows and presentation columns are unrelated.
    wand_row = re.search(r'\{ ITEM_ELEMENTAL_WAND,\s*[^,]+,\s*[^,]+,', source(extended))[0].rstrip(',') + ' }'
    modes += '\nstatic const NeiItem sNeiItems[] = {' + wand_row + '};\n#define NEI_ITEMS_COUNT 1\n'
    modes += bodies(extended, ['Nei_FindByItem', 'ExtPlayer_FindByIA', 'ExtPlayer_GetItemAction', 'ExtPlayer_GetActionModelGroup'])
    modes += '\n' + bodies(player_path, ['Player_ItemToItemAction', 'Player_UseItem', 'func_8082FDC4',
        'func_Dpad_8082FDC4', 'Player_ItemIsInUse', 'Player_ProcessItemButtons'])
    modes += '\n' + bodies(lib_path, ['Player_ActionToModelGroup', 'Player_GetItemOnButton', 'Player_Dpad_GetItemOnButton'])
    update = functions(source(player_path), ['Player_Update'])['Player_Update']
    begin = update.index('    if (play->actorCtx.isOverrideInputOn')
    end = update.index('    GameInteractor_ExecuteOnPassPlayerInputs(&input);', begin)
    end += len('    GameInteractor_ExecuteOnPassPlayerInputs(&input);')
    selection = re.sub(r'\bthis\b', 'player', update[begin:end])
    modes += '\nvoid CompleteWandFrame(Player* player,PlayState* play,u16 press,u16 held){\n' + \
        '++play->gameplayFrames;play->state.input[0].press.button=press;play->state.input[0].cur.button=held;Input input;\n' + \
        selection + '\nsPlayerControlInput=&input;Wand_TickInput(play,player);Player_ProcessItemButtons(player,play);\n}\n'

    oot_helper = 'soh/mods/items/helpers/equip_helper.c'
    oot_text = source(oot_helper)
    oot = oot_text[oot_text.index('typedef struct {'):oot_text.index('static void EquipCache_Update')]
    oot += '\n' + bodies(oot_helper, ['EquipCache_Update', 'ItemInput_GetEquippedButton',
        'ItemInput_ButtonIsClaimed', 'ItemInput_CheckOtherButtons', 'ItemInput_Update',
        'ItemInput_IsBlockedEx', 'ItemInput_IsBlocked', 'ItemMagic_IsInfinite', 'ItemMagic_Consume', 'ItemMagic_HasEnough'])
    oot_sand = re.sub(r'^#include[^\n]*\n', '', source('soh/mods/items/logic/wand/wand_sand.c'), flags=re.M)
    oot_sand = oot_sand.replace(functions(oot_sand)['WandSand_SlabDraw'], '')
    oot += '\n' + oot_sand
    oot_wand = 'soh/mods/items/logic/item_elemental_wand.c'
    oot += '\n' + re.search(r'static const s16 sWandMagicCost\[WAND_MODE_COUNT\] = \{.*?\};', source(oot_wand), re.S)[0]
    oot += '\n' + bodies(oot_wand, ['Wand_IsDrawn', 'Wand_OnWheelConfirm', 'Wand_BuildWheel',
        'Wand_ActiveWheelIndex', 'Wand_Cast', 'Wand_TickInput'])

    with tempfile.TemporaryDirectory(prefix='mm-wand-') as directory:
        temp = Path(directory)
        (temp / 'production.inc').write_text(production)
        (temp / 'shadow.inc').write_text(shadow)
        (temp / 'modes.inc').write_text(modes)
        (temp / 'oot_sand.inc').write_text(oot)
        for name, test_path in [('native-storm', 'native_storm_test.cpp'), ('shadow', 'shadow_test.cpp'),
                ('modes', 'modes_test.cpp'), ('oot-sand', 'oot_sand_test.cpp')]:
            if args.case and args.case != name:
                continue
            binary = temp / name
            compiler_flags = flags()
            if name == 'oot-sand':
                sys.path.insert(0, str(ROOT / 'tests/nei_held'))
                from run_articulated_tests import flags as oot_flags
                compiler_flags = oot_flags()[1:]
            subprocess.run(['c++', '-std=c++20', '-fpermissive', *compiler_flags, '-I' + str(temp),
                '-ffunction-sections', '-fdata-sections', str(ROOT / 'tests/wand_modes' / test_path),
                '-Wl,--gc-sections', '-o', str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    main()
