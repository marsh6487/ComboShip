#!/usr/bin/env python3
"""Real soul item rows/accessors, native encoding/export and combined seed hint composition.

Item construction/UI formatting are fixture boundaries; item identities, authored
keys, registration, GetHint/export and combined placement hint generation are production.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'tests/cross_hints'))
import run_item_hint_export_tests as native


def fixture():
    code = native.fixture().split(native.TESTS)[0]
    # Use the actual native language selector enum, including MAX for Replace.
    native_z64=(ROOT/'soh/include/z64.h').read_text()
    end=native_z64.index('} Language;')+len('} Language;')
    begin=native_z64.rfind('typedef enum {',0,end)
    code=code.replace('enum { LANGUAGE_ENG, LANGUAGE_GER, LANGUAGE_FRA };',native_z64[begin:end])
    # Use complete real enum definitions (including append-only catalogue keys).
    code = code.replace('enum RandomizerHintTextKey { RHT_NONE, RHT_AUTHORED };', '''
#define RANDO_ENUM_BEGIN(name) enum name {
#define RANDO_ENUM_ITEM(name) name,
#define RANDO_ENUM_END(...) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerHintTextKey.h"
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_END
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_BEGIN
''')
    code = code.replace('HintText hintTextTable[2]', 'HintText hintTextTable[RHT_MAX]')
    code = code.replace('    Text name;','    Text name;\n    Text article;\n    RandomizerGet randomizerGet = RG_NONE;')
    code = code.replace('    const Text& GetName() const;', '''
    template<class... Extra> Item(RandomizerGet id, Text value, int, int, bool, int,
                                  RandomizerHintTextKey key, Extra&&...)
        : Item(std::move(value), key) { randomizerGet = id; }
    Item& CustomIcon(const char*) { return *this; }
    const Text& GetArticle() const;
    RandomizerGet GetRandomizerGet() const;
    const Text& GetName() const;''')
    # Compile the real downstream generator, not duplicate its formatting policy.
    code = code[:code.index('namespace ComboRando {')]
    item_source = (ROOT / 'soh/soh/Enhancements/randomizer/item.cpp').read_text()
    code += 'namespace Rando {\n' + native.block(item_source, 'const Text& Item::GetArticle()')
    code += native.block(item_source, 'RandomizerGet Item::GetRandomizerGet()') + '}\n'
    text_source = (ROOT / 'soh/soh/Enhancements/custom-message/text.cpp').read_text()
    for marker in ('Text::Text(std::string english_, std::string french_, std::string german_, std::string spanish_)',
                   'const std::string& Text::GetSpanish()', 'Text Text::operator+(const Text& right)'):
        code += native.block(text_source, marker)
    messages = (ROOT / 'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp').read_text()
    code += native.block(messages, 'CustomMessage::CustomMessage(Text text,')
    code += native.block(messages, 'static const std::unordered_map<std::string, std::string> percentColors', True)
    for signature in ('void CustomMessage::Replace(std::string&& oldStr, CustomMessage newMessage)',
                      'void CustomMessage::ReplaceColors(', 'std::string CustomMessage::COLOR('):
        code += native.block(messages, signature)

    code += r'''
using RandomizerCheck = int;
namespace Rando {
struct SoulHintLocation {
    const Item* placed = nullptr;
    RandomizerGet GetPlacedRandomizerGet() const { return placed->GetRandomizerGet(); }
    const Item& GetPlacedItem() const { return *placed; }
};
struct SoulHintOverride { Text GetTrickArticle() const { return {}; } Text GetTrickName() const { return {}; } };
struct Context {
    std::map<int,SoulHintLocation> locations;
    std::map<int,SoulHintOverride> overrides;
    static Context* GetInstance() { static Context context; return &context; }
    SoulHintLocation* GetItemLocation(int check) { return &locations.at(check); }
};
class Hint {
public:
    std::vector<int> locations{1};
    const HintText GetItemHintText(uint8_t slot, bool mysterious = false) const;
};
'''
    hint_source = (ROOT / 'soh/soh/Enhancements/randomizer/hint.cpp').read_text()
    code += native.block(hint_source, 'const HintText Hint::GetItemHintText(') + '}\n'
    code += '\n#include "combo/rando/CrossHints.h"\n'
    items = (ROOT / 'soh/soh/Enhancements/randomizer/item_list.cpp').read_text()
    rows = [line.strip() for line in items.splitlines()
            if re.match(r'\s*itemTable\[RG_(?:SOUL_|MM_SOUL_)\w+\]\s*=\s*Item', line)]
    assert len(rows) == 147
    constants = sorted(set(re.findall(r'\b(?:ITEMTYPE|LOGIC|OBJECT|GID|TEXT|CHEST|ITEM_CATEGORY|MOD)_[A-Z0-9_]+\b', '\n'.join(rows))))
    code += '\nenum { ' + ', '.join(constants) + ' };\nstatic const char* gBossSoulTex = "soul";\n'
    code += 'std::map<RandomizerGet, Rando::Item> itemTable;\nvoid InitItems() { using namespace Rando;\n'
    # map::insert_or_assign avoids requiring a fake default Item constructor.
    for row in rows:
        match = re.match(r'itemTable\[(\w+)\] = (.*);', row)
        assert match, row
        code += 'itemTable.insert_or_assign(' + match[1] + ', ' + match[2] + ');\n'
    code += '}\n'
    item_hints = (ROOT / 'soh/soh/Enhancements/randomizer/3drando/hint_list/hint_list_item.cpp').read_text()
    hook = re.search(r'SoulHints::Register\(hintTextTable\);', item_hints)
    if hook:
        code += '#include "soh/soh/Enhancements/randomizer/SoulHintText.h"\n'
    code += 'void InitSoulHints() { using namespace Rando;\n'
    if hook:
        code += 'auto& hintTextTable = StaticData::hintTextTable;\n' + hook[0]
    code += '\n}\n'
    templates = (ROOT / 'soh/soh/Enhancements/randomizer/3drando/hint_list.cpp').read_text()
    code += 'void InitTemplates() { using namespace Rando; auto& hintTextTable = StaticData::hintTextTable;\n'
    for key in ('RHT_CAN_BE_FOUND_AT', 'RHT_HOARDS'):
        start = templates.index('hintTextTable[' + key + '] =')
        end = templates.index('));', start) + 3
        code += templates[start:end] + '\n'
    code += '}\n'
    code += (ROOT / 'tests/shuffle_parity/souls/hint_cases.inc').read_text()
    return code


def main():
    with tempfile.TemporaryDirectory(prefix='native-soul-hints-') as temporary:
        cpp = Path(temporary) / 'hints.cpp'
        binary = Path(temporary) / 'hints'
        cpp.write_text(fixture())
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-g', '-O1',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                        '-fno-omit-frame-pointer', '-fno-pie', '-no-pie', '-I' + str(ROOT), '-I' + str(ROOT / 'combo'),
                        str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True,
                       env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})


if __name__ == '__main__':
    main()
