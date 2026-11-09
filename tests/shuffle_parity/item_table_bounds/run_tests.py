#!/usr/bin/env python3
"""Check native item IDs and initialize the real Item array under sanitizers."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
RANDO = ROOT / 'soh/soh/Enhancements/randomizer'


def function(source, signature):
    begin = source.index(signature)
    end = source.index('\n}', begin) + 2
    return source[begin:end]


def native_fixture():
    items = (RANDO / 'item.cpp').read_text()
    catalog = (RANDO / 'item_list.cpp').read_text()
    draw = (RANDO / 'draw.h').read_text()
    text = (ROOT / 'soh/soh/Enhancements/custom-message/text.cpp').read_text()
    init = function(catalog, 'void Rando::StaticData::InitItemTable()')
    code = r'''
#include "global.h"
#include "soh_assets.h"
#include "soh/Enhancements/randomizer/static_data.h"
#include "textures/icon_item_24_static/icon_item_24_static.h"
#include "textures/icon_item_static/icon_item_static.h"
#include "z64object.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/Enhancements/randomizer/draw.h"
#include "mods/extended_equipment.h"
#include "mods/extended_inventory.h"
#include <cassert>
#include <cstdio>
using namespace Rando;
#ifdef COMBO_BUILD
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#endif
std::array<Item, RG_MAX> Rando::StaticData::itemTable;
std::unordered_map<std::string, RandomizerGet> Rando::StaticData::itemNameToEnum;
// Only the unused context query and external draw registry are service seams.
// Item, Text, array storage, constructors, assignment and initialization are real.
struct ItemTableContextBoundary {
    static ItemTableContextBoundary* GetInstance() { static ItemTableContextBoundary context; return &context; }
    int GetLogic() { return 0; }
};
extern "C" const NeiItem* Nei_FindByRg(int16_t) { return nullptr; }
extern "C" void MagicMirror_DrawGI(PlayState*) { assert(false); }
'''
    names = sorted(set(re.findall(r'SetCustomDrawFunc\((Randomizer_\w+)\)', init)))
    for name in names:
        declaration = re.search(r'void ' + name + r'\([^;]+;', draw).group(0)
        code += declaration[:-1] + ' { assert(false && "item initialization called a renderer"); }\n'
    code += text[text.index('Text::Text()'):text.index('static bool IsMissing')]
    for signature in ('static bool IsMissing(', 'const std::string& Text::GetEnglish(',
                      'const std::string& Text::GetFrench(', 'const std::string& Text::GetGerman('):
        code += function(text, signature) + '\n'
    code += 'namespace Rando {\n'
    code += items[items.index('Item::Item()'):items.index('void Item::ApplyEffect()')]
    for signature in ('const Text& Item::GetName(', 'RandomizerGet Item::GetRandomizerGet(',
                      'void Item::SetCustomDrawFunc(', 'Item Item::CustomIcon(',
                      'int Item::GetItemID(', 'ItemType Item::GetItemType(', 'LogicVal Item::GetLogicVal(',
                      'GetItemCategory Item::GetCategory('):
        code += function(items, signature) + '\n'
    code += '}\n#define Context ItemTableContextBoundary\n' + init + '\n#undef Context\n'
    code += r'''
static_assert(RG_RESERVED_PRE_KEY_RING_MAX == 682);
static_assert(RG_SOUL_MISC_BUSINESS_SCRUB == 681);
static_assert(RG_MM_KEY_RING_WOODFALL == 683 && RG_MM_KEY_RING_SNOWHEAD == 684);
static_assert(RG_MM_KEY_RING_GREAT_BAY == 685 && RG_MM_KEY_RING_STONE_TOWER == 686);
int main() {
    Rando::StaticData::InitItemTable();
    // Native accessor definitions are included below, preserving the real array.
    auto& table = Rando::StaticData::GetItemTable();
    const RandomizerGet rings[] = { RG_MM_KEY_RING_WOODFALL, RG_MM_KEY_RING_SNOWHEAD,
                                   RG_MM_KEY_RING_GREAT_BAY, RG_MM_KEY_RING_STONE_TOWER };
    for (int i = 0; i < 4; ++i) {
        auto& item = table[rings[i]];
        assert(item.GetRandomizerGet() == rings[i]);
        assert(!item.GetName().english.empty());
        assert(item.GetItemID() == 0x1b1 + i);
    }
    auto& reserved = table[RG_RESERVED_PRE_KEY_RING_MAX];
    assert(reserved.GetName().english.empty());
    assert(reserved.GetItemType() == ITEMTYPE_EVENT);
    assert(reserved.GetLogicVal() == LOGIC_NONE);
    assert(reserved.GetCategory() == ITEM_CATEGORY_JUNK);
    // Repeat startup to exercise real strings/shared pointers during replacement.
    Rando::StaticData::InitItemTable();
    std::puts("PASS complete native InitItemTable with real Item/Text array, stable ring IDs/GI/callbacks and safe reserved slot");
}
'''
    code += function(catalog, 'std::array<Item, RG_MAX>& Rando::StaticData::GetItemTable()') + '\n'
    return code


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--enum-only', action='store_true')
    parser.add_argument('--combo', action='store_true', help='compile the actual combined-build item initializer')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='native-item-table-bounds-') as temporary:
        build = Path(temporary)
        cpp = build / 'bounds.cpp'
        binary = build / 'bounds'
        checks = '''
#include "soh/Enhancements/randomizer/randomizerEnums.h"
static_assert(RG_MM_KEY_RING_WOODFALL < RG_MAX, "Woodfall row exceeds native item table");
static_assert(RG_MM_KEY_RING_SNOWHEAD < RG_MAX, "Snowhead row exceeds native item table");
static_assert(RG_MM_KEY_RING_GREAT_BAY < RG_MAX, "Great Bay row exceeds native item table");
static_assert(RG_MM_KEY_RING_STONE_TOWER < RG_MAX, "Stone Tower row exceeds native item table");
'''
        if args.enum_only:
            cpp.write_text(checks + 'int main() {}\n')
        else:
            cpp.write_text(checks + native_fixture())
        includes = [ROOT, ROOT / 'soh', ROOT / 'soh/include', ROOT / 'soh/include/PR',
                    ROOT / 'soh/src', ROOT / 'soh/assets', ROOT / 'libultraship/include', ROOT / 'combo', ROOT / 'combo/menu']
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-g', '-O0',
                        '-DF3DEX_GBI_2', '-DCONTROLLERBUTTONS_T=uint32_t',
                        *(['-DCOMBO_BUILD'] if args.combo else []),
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-fno-omit-frame-pointer', '-fno-pie', '-no-pie',
                        *['-I' + str(path) for path in includes],
                        str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
        print('PASS all four native key-ring IDs fit the actual RG_MAX item-table extent')


if __name__ == '__main__':
    main()
