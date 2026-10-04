#!/usr/bin/env python3
"""Exercise editor grants with production item data, grant arms, stores and FC recording."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function
from run_mm_nei_tests import flags


def source(path):
    return (ROOT / path).read_text()


def main():
    items = source("mm/2s2h/Rando/StaticData/Items.cpp")
    macro = items[items.index("#define RI("):items.index("// clang-format off")]
    item_map = items[items.index("std::map<RandoItemId, RandoStaticItem> Items ="):
                     items.index("// clang-format on")]
    production = "namespace Rando::StaticData {\n" + macro + item_map + "\n}\n"
    nei = source("mm/mods/nei_save.cpp")
    for name in ["Nei_Save", "Nei_GetOwnedItem", "Nei_SetOwnedItem", "Nei_InitNewSave"]:
        production += function(nei, name) + "\n"
    production += nei[nei.index("#define NEI_CANE_SKILL_MAX"):nei.index('extern "C" uint8_t Nei_CaneActiveSkill')]
    production += function(source("mm/mods/items/logic/item_cane_of_somaria.c"), "Cane_GiveSkill")
    inventory = source("mm/mods/extended_inventory.c")
    production += inventory[inventory.index("static const uint8_t sWandQuest"):
                            inventory.index("static void* const sWandNameTex")]
    for name in ["Wand_RandoMode", "Wand_ModeOwned", "Wand_GrantMode", "Slate_RuneOwned", "Slate_GrantRune",
                 "Seasons_SeasonOwned", "Seasons_GrantSeason"]:
        production += function(inventory, name) + "\n"
    equipment = source("mm/mods/extended_equipment.c")
    for name in ["ExtEquip_GetBit", "ExtEquip_HasItem", "ExtEquip_CapeOwned", "ExtEquip_GiveCape", "ExtEquip_GiveItem"]:
        production += function(equipment, name) + "\n"
    glue = source("mm/2s2h/FleetShipCombo/FleetComboItemsGlue.cpp")
    production += glue[glue.index("namespace {"):glue.index('extern "C" const char* FcCombo_NativeNameForItem')]

    give = function(source("mm/2s2h/Rando/GiveItem.cpp"), "Rando::GiveItem")
    switch_start = give.index("    switch (randoItemId) {\n        case RI_CLOCK_TOWN_STRAY_FAIRY:")
    labels = list(re.finditer(r"^        (?:case RI_[A-Z0-9_]+:|default:)", give[switch_start:], re.M))
    # Every selected arm is copied intact from the real switch. All other native/receipt
    # branches are excluded so an accidental ordinary-inventory grant fails immediately.
    selected = []
    for i, match in enumerate(labels):
        name_match = re.search(r"RI_[A-Z0-9_]+", match.group())
        if not name_match:
            continue
        name = name_match.group()
        if name.startswith(("RI_OOT_NEI_", "RI_OOT_EXT_", "RI_OOT_MEDALLION_")):
            end = labels[i + 1].start() if i + 1 < len(labels) else len(give) - switch_start
            selected.append(give[switch_start + match.start():switch_start + end])
    production += give[:switch_start] + "    switch (randoItemId) {\n" + "\n".join(selected)
    production += '\n        default: fprintf(stderr, "unexpected editor grant %d\\n", (int)randoItemId); abort();\n'
    production += "    }\n}\n"

    editor_header = ROOT / "mm/2s2h/DeveloperTools/NeiEditorItems.h"
    if editor_header.exists():
        production += '#include "2s2h/DeveloperTools/NeiEditorItems.h"\n'
    else:
        # RED control: execute the editor's current Give All Custom Items block and
        # actual debug helper. Registry data are projected from production into the
        # real NeiItem struct; only .item/.slot are read by this ownership-only path.
        registry = source("mm/mods/extended_player.c")
        registry = registry[registry.index("static const NeiItem sNeiItems[]"):
                            registry.index("#define NEI_ITEMS_COUNT")]
        entries = re.findall(r"\{\s*(ITEM_[A-Z0-9_]+).*?\b(SLOT_[A-Z0-9_]+|NEI_NO_SLOT),\s*AGE_REQ", registry, re.S)
        production += "static const NeiItem sNeiItems[] = {\n"
        production += "\n".join("{.item=" + item + ", .slot=" + slot + "}," for item, slot in entries)
        production += "\n};\n#define NEI_ITEMS_COUNT (sizeof(sNeiItems)/sizeof(sNeiItems[0]))\n"
        production += function(source("mm/mods/extended_player.c"), "Nei_FindByItem") + "\n"
        production += function(inventory, "ExtInv_GetItemSlot") + "\n"
        production += function(source("mm/mods/extended_player.c"), "ExtInv_DebugGiveAll") + "\n"
        editor = source("mm/2s2h/DeveloperTools/SaveEditor.cpp")
        start = editor.index('    if (ImGui::Button("Give All Custom Items"))')
        end = editor.index("    // NOTE: visual slots", start)
        production += 'namespace ImGui { bool Button(const char* label) { return strcmp(label, "Give All Custom Items") == 0; } void SameLine() {} }\n'
        production += "namespace NeiEditor { void GrantAll() {\n" + editor[start:end] + "\n} }\n"

    with tempfile.TemporaryDirectory(prefix="mm-editor-") as temporary:
        build = Path(temporary)
        (build / "editor_production.inc").write_text(production)
        binary = build / "editor_grant_test"
        cxx = shlex.split(os.environ.get("CXX", "c++"))
        subprocess.run([*cxx, "-std=c++20", "-fpermissive", "-O0", "-g",
                        "-fsanitize=address,undefined,bounds", "-fno-sanitize-recover=all",
                        *flags(), "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                        "-I" + str(build), "-include", "nlohmann/json.hpp",
                        "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-missing-field-initializers",
                        str(ROOT / "tests/mm_editor/grant_test.cpp"), "-o", str(binary)], check=True)
        environment = {**os.environ, "ASAN_OPTIONS": os.environ.get("ASAN_OPTIONS", "") + ":detect_leaks=0"}
        subprocess.run([str(binary)], check=True, env=environment)
        native_flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
                        "-DNON_EQUIVALENT", "-DNON_MATCHING"]
        native_flags += ["-I" + str(ROOT / p) for p in
                         ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
                          "libultraship/include", "libultraship/src", "combo", "combo/menu")]
        for path in ["mm/2s2h/DeveloperTools/SaveEditor.cpp", "mm/2s2h/Rando/GiveItem.cpp"]:
            subprocess.run([*cxx, "-std=c++20", "-fpermissive", *native_flags,
                            "-include", "nlohmann/json.hpp", "-include", str(ROOT / "mm/include/global.h"),
                            "-include", "spdlog/spdlog.h", "-include", "ship/Context.h", "-fsyntax-only",
                            str(ROOT / path)], check=True)
            print("PASS real-header syntax: " + path)


if __name__ == "__main__":
    main()
