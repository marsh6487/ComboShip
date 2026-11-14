#!/usr/bin/env python3
"""Trace the real OoT Chateau catalog, shared-name grant and bottle projection.

The native name resolver, FC receipt bookkeeping, bottle grant arms, bottle-wheel
implementation, frame projection and relevant sync assignments are production
code. Engine persistence/drawing are boundaries; this does not boot either game.
--source-ref runs the same probe against an earlier catalog/grant implementation.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, marker):
    start = source.index(marker)
    opening = source.index("{", start)
    depth = 1
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]', source[opening + 1:]):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if not depth:
                return source[start:opening + 1 + token.end()] + "\n"
    raise ValueError(marker)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-ref")
    parser.add_argument("--json-include", type=Path, default=ROOT.parent / "deps")
    args = parser.parse_args()

    def read(path):
        if args.source_ref:
            return subprocess.check_output(["git", "show", f"{args.source_ref}:{path}"], cwd=ROOT, text=True)
        return (ROOT / path).read_text(encoding="utf-8-sig")

    catalog = read("soh/soh/Enhancements/randomizer/item_list.cpp")
    grant = read("soh/soh/Enhancements/randomizer/randomizer.cpp")
    globals_source = read("soh/soh/OTRGlobals.cpp")
    relay = read("soh/soh/FleetShipCombo/FleetSharedItems.cpp")
    sync = read("soh/soh/FleetShipCombo/FleetSync.cpp")
    glue = read("soh/soh/FleetShipCombo/FleetComboItemsGlue.cpp")
    projection = read("soh/mods/items/mm_bottle_items.cpp")
    native = read("soh/src/code/z_parameter.c")
    fixture = (ROOT / "tests/reward_delivery/chateau_grant_test.cpp").read_text()

    # Missing rows remain missing in the control; do not fabricate their identity.
    rows = re.findall(r'^\s*itemTable\[RG_CHATEAU_ROMANI\][\s\S]*?;', catalog, re.M)
    fixture = fixture.replace("/* CATALOG */", "\n".join(rows) + "\n" +
                              block(catalog, "for (auto& item : itemTable)"))
    fixture = fixture.replace("/* FC_MAP */", block(glue, "std::unordered_map<int, int>& ReverseMap()") +
                              block(glue, 'extern "C" int FcCombo_ItemForNative('))
    fixture = fixture.replace("/* RECEIVE_GUARD */", "\n".join(block(relay, marker) for marker in (
        'extern "C" void FleetShared_BeginReceive(', 'extern "C" void FleetShared_EndReceive(',
        'extern "C" int FleetShared_IsReceiving(', 'extern "C" void FleetShared_OnNativeObtained(')))
    fixture = fixture.replace("/* CAPACITY */", block(native, "bool Inventory_HasEmptyBottleSlot("))
    obtain_start = grant.index("        // Bottles\n", grant.index("ItemObtainability Randomizer::GetItemObtainability"))
    obtain_end = grant.index("        // Bottle Refills", obtain_start)
    fixture = fixture.replace("/* OBTAINABILITY */", grant[obtain_start:obtain_end])
    bottle_start = grant.index("    // bottle items\n", grant.index('extern "C" u16 Randomizer_Item_Give('))
    bottle_end = grant.index("    // dungeon items", bottle_start)
    fixture = fixture.replace("/* NATIVE_GRANT */", block(grant, "if (!FleetSync_IsApplyingFc())") +
                              grant[bottle_start:bottle_end])
    fixture = fixture.replace("/* NAME_GRANT */", block(globals_source, "static bool GrantOotItemByName("))
    fixture = fixture.replace("/* PROJECTION */", "\n".join(block(projection, marker) for marker in (
        "static void BottleItems_SyncSlot(", "static u8 BottleItems_IsVanillaBottle(",
        "static u8 BottleItems_MigrateToWheel(", "static void BottleItems_Enforce(")))
    # Receipt-validation helpers can inspect the same JSON key before ApplyShared.
    # Extract the actual import blocks from their enclosing function.
    shared_apply = block(sync, "void ApplyShared(")
    tools_start = shared_apply.index('        if (inv.value("net", false))')
    tools_end = shared_apply.index('\n    }\n\n    if (sh.contains("bottleSlots")', tools_start)
    fixture = fixture.replace("/* SHARED_BOTTLES */", shared_apply[tools_start:tools_end] + "\n" +
                              block(shared_apply, 'if (sh.contains("bottleSlots") &&') +
                              block(shared_apply, 'if (sh.contains("comboObtainedFc") &&'))

    with tempfile.TemporaryDirectory(prefix="combo-chateau-grant-") as tmp:
        path = Path(tmp)
        source = path / "chateau_grant.cpp"
        source.write_text(fixture)
        # The complete real wheel implementation is compiled, including first-bottle tools.
        wheel = path / "custom_bottles.cpp"
        wheel.write_text(read("soh/mods/items/custom_bottles.cpp"))
        binary = path / "chateau_grant"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-O0",
                        "-I" + str(ROOT), "-I" + str(ROOT / "soh"),
                        "-I" + str(ROOT / "soh/mods/items"), "-I" + str(args.json_include),
                        str(source), str(wheel), "-o", str(binary)], check=True)
        raise SystemExit(subprocess.run([str(binary)], cwd=path).returncode)


if __name__ == "__main__":
    main()
