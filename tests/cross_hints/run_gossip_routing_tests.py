#!/usr/bin/env python3
"""Run native generation/stone consumers; optionally audit a read-only combo save.

Engine inventory, RNG draws and font submission are controlled boundaries. This
does not launch a game or prove an active mod archive's actor placements/pixels.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

import run_item_hint_export_tests as native

ROOT = Path(__file__).resolve().parents[2]
RANDO = "soh/soh/Enhancements/randomizer/"


def source(path):
    local = ROOT / path
    alternate = Path(os.environ.get("COMBOSHIP_NATIVE_SOURCE_ROOT", ROOT)) / path
    return (local if local.is_file() else alternate).read_text()


def enum(source_text, name):
    match = re.search(r"typedef enum \{(?:(?!typedef enum)[\s\S])*?\} " + name + r";", source_text)
    if not match:
        raise ValueError("Missing enum " + name)
    return match.group()


def macro_enum(text, name):
    if "RANDO_ENUM_BEGIN(" + name + ")" in text:
        text = text.split("RANDO_ENUM_BEGIN(" + name + ")", 1)[1].split("RANDO_ENUM_END(" + name + ")", 1)[0]
    values = re.findall(r"RANDO_ENUM_ITEM\((\w+)\)", text)
    if not values:
        raise ValueError("Missing macro enum " + name)
    return "enum " + name + " {" + ",".join(values) + "};\n"


def build(exporter_override=None):
    old_source = native.source
    native.source = lambda path: (exporter_override.read_text() if exporter_override and path == "soh/soh/OTRGlobals.cpp"
                                   else source(path))
    try:
        # Reuse real Item/Text/HintText/MF_ENCODE definitions, not the picker's unit tests.
        prefix = native.fixture().split("namespace ComboRando {", 1)[0]
    finally:
        native.source = old_source
    block = native.block
    cross = source("combo/rando/CrossHints.h")
    playthrough = source("combo/rando/ComboPlaythrough.h")
    foreign = source("combo/rando/CrossForeign.h")
    custom = source("soh/soh/Enhancements/custom-message/CustomMessageManager.cpp")
    parts = [prefix, "#include <array>\n#include <cstring>\n#include <functional>\n#include <fstream>\n#include <map>\n#include <optional>\n#include <set>\n#include <tuple>\n#include <unordered_set>\n"]
    parts += [block(custom, "CustomMessage::CustomMessage(std::string english_, TextBoxType type_, TextBoxPosition position_)"),
              block(custom, "void CustomMessage::Replace(std::string&& oldStr, std::string&& newStr)"),
              block(custom, "void CustomMessage::SetSingularPlural()")]
    parts.append("namespace ComboRando {\n")
    parts.append("enum GameId : uint8_t { GAME_OOT = 0, GAME_MM = 1 };\n")
    parts += [block(foreign, "inline const char* GameSuffix("), block(foreign, "struct ForeignItem", True),
              block(foreign, "inline std::string ShownForeignName("),
              block(source("combo/rando/CrossWorldRando.h"), "struct CwRng", True),
              block(playthrough, "struct CwPlacedItem", True),
              block(playthrough, "struct RequirednessResult", True),
              block(playthrough, "inline std::vector<CwPlacedItem> ParseSpoilerPlacements(")]
    parts.append("std::string g_comboForeignJson;\n")
    parts += [block(foreign, "struct HintGossipEntry", True), block(foreign, "struct MmHints", True),
              block(foreign, "inline MmHints LoadHintsMM("), "}\n"]
    parts.append(cross[cross.index("namespace ComboRando {"):])
    parts.append(macro_enum(source(RANDO + "randomizerEnums/RandomizerCheck.h"), "RandomizerCheck"))
    parts.append(macro_enum(source(RANDO + "randomizerEnums/RandomizerMiscEnums.h"), "RandomizerHint"))
    parts.append((ROOT / "tests/cross_hints/oot_gossip_boundary.inc").read_text())
    data = source(RANDO + "static_data.cpp")
    parts.append("namespace Rando {\n")
    for key in ("gossipStoneCheckToHint", "stoneParamsToHint", "grottoChestParamsToHint"):
        marker = re.search(r"std::unordered_map<[^\n]+> StaticData::" + key + r"\{", data).group()
        parts.append(block(data, marker, True))
    parts.append("}\n")
    locations = source(RANDO + "location_list.cpp")
    parts.append(block(locations, "std::vector<RandomizerCheck> Rando::StaticData::GetGossipStoneLocations()"))
    checks = []
    for line in locations.splitlines():
        match = re.search(r'locationTable\[(RC_\w+)\].*Location::HintStone\((.*)\);', line)
        if match:
            args = [arg.strip() for arg in match[2].split(',')]
            checks.append((match[1], args[-2]))
    parts.append("void InitLocations() {\n")
    for check, _ in checks:
        parts.append(f"Rando::StaticData::locationTable[{check}] = {{1,{check}}};\n")
    parts.append("}\n")
    parts.append("const std::map<RandomizerCheck,int> locationActorParams={" +
                 ",".join("{" + key + "," + param + "}" for key, param in checks) + "};\n")
    parts.append(block(source("soh/soh/OTRGlobals.cpp"), "static void\nCombo_WalkComboHints("))
    messages = source(RANDO + "Messages/GossipStoneHints.cpp")
    parts += [block(messages, "static CustomMessage ReadHintMessage("),
              block(messages, "void BuildHintStoneMessage(")]
    parts.append(mm_fixture())
    parts.append((ROOT / "tests/cross_hints/gossip_routing_cases.inc").read_text())
    return "\n".join(parts)


def mm_fixture():
    block = native.block
    actor = source("mm/2s2h/Rando/ActorBehavior/EnGs.cpp")
    types = source("mm/2s2h/Rando/Types.h")
    convert = source("mm/2s2h/Rando/ConvertItem.cpp")
    names = source("mm/2s2h/Rando/StaticData/Items.cpp")
    enums = "\n".join(enum(types, name) for name in ("RandoCheckType", "RandoCheckId", "RandoItemType", "RandoItemId"))
    algorithms = []
    for name in ("riToWeight", "rcToWeight", "itemTypeToWeight"):
        marker = re.search(r"std::unordered_map<[^\n]+> " + name + r" =", actor).group()
        algorithms.append(block(actor, marker, True))
    algorithms += [block(convert, "static RandoItemType ForeignTypeFromCategory("),
                   block(convert, "RandoItemType Rando::GetItemTypeForCheck(")]
    algorithms.append("namespace Rando::StaticData {\n" + block(names, "std::string GetItemName(") + "}\n")
    for marker in ("s32 GetNormalizedCost()", "EnGs* GetGossipStone()", "bool ShouldHintMoonMask(",
                   "RandoCheckId GetRandomCheck(", "static void ComboReportStoneHint("):
        algorithms.append(block(actor, marker))
    for native_id, name in (("FIRST_GS_MESSAGE", "FirstMessage"), ("SECOND_GS_MESSAGE", "SecondMessage")):
        section = actor[actor.index("    COND_ID_HOOK(OnOpenText, " + native_id):]
        algorithms.append("auto " + name + " = " + block(section, "[](u16* textId, bool* loadFromMessageTable)") + ";\n")
    boundary = (ROOT / "tests/cross_hints/mm_gossip_boundary.inc").read_text()
    return boundary.replace("/* NATIVE_ENUMS */", enums).replace("/* NATIVE_ALGORITHMS */", "\n".join(algorithms))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--audit-save", type=Path)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--baseline-exporter", type=Path)
    args = parser.parse_args()
    original_hash = hashlib.sha256(args.audit_save.read_bytes()).hexdigest() if args.audit_save else None
    with tempfile.TemporaryDirectory(prefix="gossip-routing-") as temporary:
        root = Path(temporary)
        cpp = root / "test.cpp"
        cpp.write_text(build(args.baseline_exporter))
        binary = root / "test"
        command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-DCOMBO_BUILD",
                   "-I" + os.environ.get("NLOHMANN_JSON_INCLUDE", "/usr/include")]
        if args.sanitize:
            command += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
        subprocess.run([*command, str(cpp), "-o", str(binary)], check=True)
        result = subprocess.run([str(binary), *([str(args.audit_save)] if args.audit_save else [])],
                                text=True, capture_output=True,
                                env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        for line in result.stdout.splitlines():
            if not line.startswith("AUDIT_RESULT "):
                print(line)
        if result.stderr:
            print(result.stderr, end="")
        result.check_returncode()
        if args.audit_save:
            assert hashlib.sha256(args.audit_save.read_bytes()).hexdigest() == original_hash, "Original save changed"
        if args.report:
            record = next(line.removeprefix("AUDIT_RESULT ") for line in result.stdout.splitlines()
                          if line.startswith("AUDIT_RESULT "))
            report = json.loads(record)
            report["save_sha256"] = original_hash
            report["configured_runtime"] = "untested; native algorithms at engine service boundaries"
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
