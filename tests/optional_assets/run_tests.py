"""Exercise optional icon fallback and enforce the asset-only delivery boundary."""
import json
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    assert match, name
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def icon_fallback():
    item = (ROOT / "soh/soh/Enhancements/randomizer/item.cpp").read_text()
    helper = ROOT / "soh/soh/Enhancements/randomizer/OptionalNeiIcons.h"
    with tempfile.TemporaryDirectory(prefix="optional-nei-icons-") as tmp:
        directory = Path(tmp)
        (directory / "soh").mkdir()
        (directory / "soh/ResourceManagerHelpers.h").write_text('''#pragma once
#include <cstdint>
uint8_t ResourceMgr_FileExists(const char*);
uint8_t ResourceMgr_FileAltExists(const char*);
bool ResourceMgr_IsAltAssetsEnabled();
''')
        code = '''#include <cassert>
#include <cstring>
#include <cstdint>
static bool basePresent, altPresent, altEnabled;
uint8_t ResourceMgr_FileExists(const char*) { return basePresent; }
uint8_t ResourceMgr_FileAltExists(const char*) { return altPresent; }
bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
'''
        if helper.exists():
            code += '#include "' + str(helper) + '"\n'
        code += '''enum CustomIconSize { ICON_SIZE_24, ICON_SIZE_32 };
struct Item {
 const char* customIcon;
 CustomIconSize iconSize;
 const char* GetCustomIcon();
 CustomIconSize GetCustomIconSize();
};
'''
        code += function(item, "Item::GetCustomIcon") + '\n'
        code += function(item, "Item::GetCustomIconSize") + '\n'
        code += '''int main() {
 const char* optional = "__OTR__textures/icon_item_static/gStatMagicTex";
 const char* native = "__OTR__textures/icon_item_24_static/gQuestIconMagicJarSmallTex";
 Item magic{optional, ICON_SIZE_32};
 for (int bits = 0; bits < 8; ++bits) {
   basePresent = bits & 1; altPresent = bits & 2; altEnabled = bits & 4;
   const bool selected = basePresent || (altPresent && altEnabled);
   assert(std::strcmp(magic.GetCustomIcon(), selected ? optional : native) == 0);
   assert(magic.GetCustomIconSize() == (selected ? ICON_SIZE_32 : ICON_SIZE_24));
 }
 basePresent = altPresent = altEnabled = false;
 assert(std::strcmp(magic.GetCustomIcon(), native) == 0);
 Item ordinary{"ordinary", ICON_SIZE_32};
 assert(ordinary.GetCustomIcon() == ordinary.customIcon && ordinary.GetCustomIconSize() == ICON_SIZE_32);
 Item none{nullptr, ICON_SIZE_24};
 assert(none.GetCustomIcon() == nullptr && none.GetCustomIconSize() == ICON_SIZE_24);
}
'''
        source, binary = directory / "test.cpp", directory / "test"
        source.write_text(code)
        subprocess.run(["c++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-I", str(directory),
                        str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("Optional Magic icon: native 24px fallback, selected base/Alt 32px override, unload and unrelated items pass")


def optional_layout():
    manifest = json.loads((ROOT / "tools/nei_icons/manifest.json").read_text())
    for host in ("soh", "mm"):
        for entry in manifest["icons"]:
            target = ROOT / host / "assets/custom" / entry["resource"]
            assert not target.exists(), (target, "HD icon is bundled in the game")
        assert not (ROOT / host / "assets/custom/objects/nei_gi_redesign/cojiro").exists(), \
            (host, "optional Cojiro is bundled in the game")
    print("Cojiro and HD icon replacement resources are absent from both built-in game asset trees")


if __name__ == "__main__":
    icon_fallback()
    optional_layout()
