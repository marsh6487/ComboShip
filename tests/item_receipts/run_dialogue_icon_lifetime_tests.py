#!/usr/bin/env python3
"""Run actual tutorial builders and soul branch at message-transport boundaries.

Regression: the captured fire must select the icon, and a soul's image must not
depend on a still-live get-item actor. Native layout/rendering has its own gate.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def body(source, signature):
    begin = source.index('{', source.index(signature))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]


source = (ROOT / 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp').read_text()
mm = (ROOT / 'mm/mods/items/logic/lantern_message.cpp').read_text()
soul = source[source.index('    auto soulItem =', source.index('void BuildCustomItemMessage')):]
soul = soul[:soul.index('#ifdef COMBO_BUILD')]
code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include "combo/menu/ComboItemReceiptPresentation.h"
#include "soh/include/z64item.h"
using u8 = uint8_t;
constexpr int TEXTBOX_TYPE_BLUE = 2;
using RandomizerGet = int;
u8 gLanternCatchPending = 0;
static CwItemReceiptPresentation emitted;
static std::vector<std::string> emittedText;
struct CustomMessage {
    CwItemReceiptPresentation receiptPresentation{};
    std::vector<std::string> text;
    CustomMessage() = default;
    CustomMessage(const char* en, const char* de, const char* fr, int) : text{en,de,fr} {}
    void AutoFormat() {}
    void AutoFormat(ItemID) {}
    void LoadIntoFont() { emitted = receiptPresentation; emittedText = text; }
};
namespace ComboBossSoulReceiptText {
enum class Language { German, French };
struct Soul {};
static Soul soul;
static const Soul* Find(const std::string&) { return &soul; }
static const char* Body(const Soul&, Language = Language::German) { return "A boss soul"; }
}
namespace Rando::StaticData {
struct Name { std::string GetEnglish() const { return "Gohma's Soul"; } };
struct Item {
    Name GetName() const { return {}; }
    bool HasCustomIcon() const { return true; }
    int GetItemID() const { return 0x1E0; }
    const char* GetCustomIcon() const { return "__OTR__textures/parameter_static/gBossSoul"; }
};
static Item RetrieveItem(int) { return {}; }
}
'''
helper = ROOT / 'combo/menu/ComboLanternReceiptIcon.h'
if helper.exists():
    code += '#include "combo/menu/ComboLanternReceiptIcon.h"\n'
code += '\nvoid BuildLanternCatchMessage(uint16_t* textId, bool* loadFromMessageTable) ' + body(source, 'void BuildLanternCatchMessage')
code += '\nvoid BuildSoul(CustomMessage& msg, int rgid) {\n' + soul + '\n}\n'
code += r'''
namespace MM {
namespace CustomMessage {
struct Entry { int textboxType = 0; CwItemReceiptPresentation receiptPresentation{}; };
void StartTextbox(const char* text, const Entry& entry) {
    emitted = entry.receiptPresentation; emittedText = {text};
}
}
'''
code += '\nvoid Lantern_OpenCatchTextbox(void) ' + body(mm, 'void Lantern_OpenCatchTextbox') + '\n}\n'
code += r'''
int main(int argc, char**) {
    if (argc > 1) {
        CustomMessage message;
        BuildSoul(message, 0x1E0);
        // No get-item actor is available to the receipt renderer after this point.
        // Its complete resource identity must travel with the built message.
        assert(ComboReceipt_HasIcon(&message.receiptPresentation));
        assert(!std::strcmp(message.receiptPresentation.iconPath,
                            "__OTR__textures/parameter_static/gBossSoul"));
        assert(message.receiptPresentation.iconWidth == 32 &&
               message.receiptPresentation.iconHeight == 32 && !message.receiptPresentation.iconIA8);
        std::cout << "PASS boss-soul receipt owns its RGBA32 image independently of get-item lifetime\n";
        return 0;
    }
    const char* expected[] = {
        "__OTR__textures/icon_item_custom/gItemIconLanternTex",
        "__OTR__textures/icon_item_custom/gItemIconLanternFireTex",
        "__OTR__textures/icon_item_custom/gItemIconLanternBlueTex",
        "__OTR__textures/icon_item_custom/gItemIconLanternPoeTex",
        "__OTR__textures/icon_item_custom/gItemIconLanternGreenTex",
        "__OTR__textures/icon_item_custom/gItemIconLanternTex"
    };
    for (unsigned fire = 0; fire < 6; ++fire) {
        gLanternCatchPending = fire == 5 ? 255 : fire;
        uint16_t id = 0xF9;
        bool fromTable = true;
        BuildLanternCatchMessage(&id, &fromTable);
        assert(!fromTable);
        assert(ComboReceipt_HasIcon(&emitted));
        assert(!std::strcmp(emitted.iconPath, expected[fire]));
        assert(emitted.iconWidth == 32 && emitted.iconHeight == 32 && !emitted.iconIA8);
        assert(emittedText.size() == 3);
        for (const auto& text : emittedText) assert(text.find("\x13\xB4") == std::string::npos);
        gLanternCatchPending = 0; // Rendering must retain the caught state after handoff.
        assert(!std::strcmp(emitted.iconPath, expected[fire]));
        gLanternCatchPending = fire == 5 ? 255 : fire;
        MM::Lantern_OpenCatchTextbox();
        assert(ComboReceipt_HasIcon(&emitted));
        assert(!std::strcmp(emitted.iconPath, expected[fire]));
        assert(emitted.iconWidth == 32 && emitted.iconHeight == 32 && !emitted.iconIA8);
    }
    std::cout << "PASS OoT/MM captured lantern icons: four fires, empty/invalid fallback, three OoT languages\n";
}
'''
flags = ['-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
              '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='dialogue-icons-') as directory:
    path = Path(directory)
    (path / 'icons.cpp').write_text(code)
    subprocess.run([*shlex.split(os.environ.get('CXX', 'c++')), *flags, '-I', str(ROOT),
                    str(path / 'icons.cpp'), '-o', str(path / 'icons')], check=True)
    results = [subprocess.run([str(path / 'icons'), *args]) for args in ([], ['soul'])]
    if any(result.returncode for result in results):
        raise SystemExit(1)
