#!/usr/bin/env python3
"""Native production Logic inventory isolation and combined fill/replay regression."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-rev', help='Extract complete production bodies from this git revision')
    parser.add_argument('--json-include', type=Path, help='Directory containing nlohmann/json.hpp and spdlog')
    parser.add_argument('--imgui-include', type=Path, help='Optional real ImGui headers; default uses GUI declaration boundary')
    parser.add_argument('--output-dir', type=Path)
    parser.add_argument('--mode', choices=['original', 'combo'], help='Build just one guard variant (default both)')
    parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    def read(path):
        if args.source_rev:
            return subprocess.check_output(['git', 'show', f'{args.source_rev}:{path}'], cwd=ROOT, text=True)
        return (ROOT / path).read_text()
    logic = read('soh/soh/Enhancements/randomizer/logic.cpp')
    signatures = ['Logic::Logic()', 'void Logic::InitSaveContext()', 'void Logic::NewSaveContext()',
                  'void Logic::SetSaveContext(', 'uint8_t Logic::InventorySlot(',
                  'uint32_t Logic::CurrentInventory(', 'bool Logic::CheckInventory(', 'void Logic::SetInventory(',
                  'bool Logic::BAllowed()', 'bool Logic::CheckDungeonItem(', 'bool Logic::ItemUseAllowed(', 'bool Logic::BombchusEnabled(', 'bool Logic::BombchuRefill(', 'bool Logic::HasExplosives(', 'uint8_t Logic::OcarinaButtons(', 'bool Logic::HasItem(', 'bool Logic::CanUse(', 'bool Logic::HasStrength(',
                  'void Logic::ApplyItemEffect(', 'void Logic::SetContext(', 'void Logic::Reset(',
                  'bool Logic::Get(', 'void Logic::Set(', 'SaveContext* Logic::GetSaveContext(',
                  'uint32_t Logic::CurrentUpgrade(', 'void Logic::SetUpgrade(',
                  'bool Logic::CheckEquipment(', 'bool Logic::CheckQuestItem(', 'void Logic::SetQuestItem(',
                  'bool Logic::CheckRandoInf(', 'void Logic::SetRandoInf(', 'void Logic::SetSmallKeyCount(',
                  'void Logic::SetDungeonItem(', 'uint8_t Logic::GetAmmo(', 'void Logic::SetAmmo(',
                  'uint8_t Logic::BottleCount(', 'bool Logic::HasBottle(']
    bodies = '\n'.join(block_from(logic, logic.index(s)) for s in signatures)
    if '// Solver inventory helpers begin' in logic:
        bodies += logic.split('// Solver inventory helpers begin', 1)[1].split('// Solver inventory helpers end', 1)[0]
    bodies = logic[logic.index('std::map<RandomizerGet, uint32_t> Logic::RandoGetToEquipFlag'):logic.index('// Solver inventory helpers begin') if '// Solver inventory helpers begin' in logic else logic.index('void Logic::ApplyItemEffect')] + bodies
    item = read('soh/soh/Enhancements/randomizer/item.cpp')
    for sig in ['Item::Item()', 'ItemType Item::GetItemType()', 'RandomizerGet Item::GetRandomizerGet()', 'LogicVal Item::GetLogicVal()', 'void Item::ApplyEffect() const']:
        bodies += block_from(item, item.index(sig)) + '\n'
    option = read('soh/soh/Enhancements/randomizer/option.cpp')
    for sig in ['OptionValue::OptionValue(uint8_t', 'uint8_t OptionValue::Get()', 'void OptionValue::Set(', 'OptionValue::operator bool() const']:
        bodies += block_from(option, option.index(sig)) + '\n'
    starting = read('soh/soh/Enhancements/randomizer/3drando/starting_inventory.cpp')
    starting_body = 'std::vector<RandomizerGet> StartingInventory;\n' + block_from(starting, starting.index('void ApplyStartingInventory()'))
    inventory = read('soh/src/code/z_inventory.c')
    arrays = ''
    for name in ['gItemSlots', 'gUpgradeMasks', 'gUpgradeNegMasks', 'gUpgradeShifts', 'gBitFlags']:
        matches = re.search(r'^[^\n]*\b' + name + r'\[.*?\n};', inventory, re.M | re.S)
        if matches: arrays += matches[0] + '\n'
    maze_source = read('soh/soh/Enhancements/randomizer/location_access/overworld/goron_city.cpp')
    maze = next(line.strip() for line in maze_source.splitlines() if line.strip().startswith('LOCATION(RC_GC_MAZE_LEFT_CHEST,'))
    temporary = tempfile.TemporaryDirectory(prefix='oot-solver-inventory-') if not args.output_dir else None
    output = args.output_dir.resolve() if args.output_dir else Path(temporary.name)
    output.mkdir(parents=True, exist_ok=True)
    (output / 'arrays.inc').write_text(arrays)
    extended = read('soh/mods/extended_inventory.c')
    registry = read('soh/mods/extended_player.c')
    # Only presentation/action fields of the native registry are omitted. Item and slot
    # metadata are copied from the actual table, not recreated in the fixture.
    rows = re.findall(r'^\s*\{ (\w+), [^,\n]+, [^,\n]+, (\w+),', registry, re.M)
    reduced = 'static const NeiItem sNeiItems[] = {\n' + ''.join('{ .item = ' + i + ', .slot = ' + slot + ' },\n' for i, slot in rows) + '};\n'
    reduced += '#define NEI_ITEMS_COUNT (sizeof(sNeiItems)/sizeof(sNeiItems[0]))\n'
    reduced += block_from(registry, registry.index('const NeiItem* Nei_FindByItem(')) + '\n'
    reduced += re.search(r'const uint8_t gPage3MaskItems\[24\] = \{.*?\n};', extended, re.S)[0] + '\n'
    reduced += re.search(r'static const uint8_t sWandQuest\[WAND_MODE_COUNT\] = \{.*?\n};', extended, re.S)[0] + '\n'
    for sig in ['uint8_t ExtInv_GetItemSlot(', 'uint8_t Slate_RuneOwned(', 'uint8_t TravelTools_IsOwned(', 'uint8_t MagicMirror_IsOwned(', 'uint8_t GraceHourglass_IsOwned(', 'uint8_t Wand_ModeOwned(', 'uint8_t Sw97_BombArrowsOwned(']:
        reduced += block_from(extended, extended.index(sig)) + '\n'
    equip = read('soh/mods/extended_equipment.c')
    for sig in ['static u32 ExtEquip_GetBit(', 'u8 ExtEquip_HasItem(', 'u8 ExtEquip_CapeOwned(']:
        reduced += block_from(equip, equip.index(sig)) + '\n'
    nei = read('soh/mods/nei_save.cpp')
    for sig in ['extern "C" uint16_t Nei_GetOwnedItem(', 'extern "C" void Nei_SetOwnedItem(']:
        reduced += block_from(nei, nei.index(sig)) + '\n'
    (output / 'runtime.inc').write_text('#define Nei_SetOwnedItem FixtureRawNei_SetOwnedItem\n' + reduced + '\n#undef Nei_SetOwnedItem\n')
    text_source = read('soh/soh/Enhancements/custom-message/text.cpp')
    (output / 'text.inc').write_text(block_from(text_source, text_source.index('Text::Text()')))

    (output / 'maze.inc').write_text(maze.rstrip(','))
    (output / 'starting.inc').write_text(starting_body)
    (output / 'production.inc').write_text('namespace Rando {\n' + bodies + '\n}\n')
    flags = ['-std=c++20', '-O0', '-g', '-ffunction-sections', '-fdata-sections', '-DF3DEX_GBI_2', '-DGBI_FLOAT_MTX', '-DGBI_S32_VTX', '-DCONTROLLERBUTTONS_T=uint32_t',
             '-DIMGUI_DEFINE_MATH_OPERATORS=', '-DLOG_LEVEL_GAME_PRINTS=0', '-DFMT_HEADER_ONLY', '-Wno-narrowing', '-DSPDLOG_ACTIVE_LEVEL=6']
    if '// Solver inventory helpers begin' not in logic:
        flags += ['-DLEGACY_SOLVER_INVENTORY']
    if args.sanitize:
        flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags += ['-D' + k + '=\"' + v + '\"' for k, v in re.findall(r'set\((CVAR_PREFIX_\w+)\s+\"([^\"]+)\"\)', read('CMake/soh-cvars.cmake'))]
    includes = [output, Path(__file__).with_name("stubs"), ROOT, ROOT / 'soh', ROOT / 'soh/include', ROOT / 'soh/assets', ROOT / 'libultraship/include', ROOT / 'combo', ROOT / 'combo/menu']
    if args.imgui_include:
        includes.insert(1, args.imgui_include.resolve())
    if args.json_include:
        includes += [args.json_include.resolve()]
    results = []
    for mode in ([args.mode] if args.mode else ['original', 'combo']):
        binary = output / ('native_test_' + mode)
        command = [os.environ.get('CXX', 'g++'), *flags, *(['-DCOMBO_BUILD'] if mode == 'combo' else []), *['-I' + str(p) for p in includes], str(Path(__file__).with_name('native_test.cpp')), '-Wl,--gc-sections', '-o', str(binary)]
        compiled = subprocess.run(command, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (output / f'{mode}_compile.log').write_text(compiled.stdout)
        result = {'mode': mode, 'sourceRev': args.source_rev, 'compileCommand': command, 'compileExitCode': compiled.returncode}
        if compiled.returncode:
            print(compiled.stdout)
        else:
            env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0')
            run = subprocess.run([str(binary)], cwd=output, env=env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (output / f'{mode}_run.log').write_text(run.stdout)
            result.update(testExitCode=run.returncode)
            summary = [line for line in run.stdout.splitlines() if line.startswith('FAIL:') or 'native solver inventory isolation (' in line]
            print(mode + ':\n' + '\n'.join(summary))
        results.append(result)
    (output / 'result.json').write_text(json.dumps(results, indent=2) + '\n')
    print('Evidence:', output)
    return int(any(r.get('testExitCode', r['compileExitCode']) != 0 for r in results))

if __name__ == '__main__':
    raise SystemExit(main())
