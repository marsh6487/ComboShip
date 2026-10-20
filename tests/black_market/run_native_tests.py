#!/usr/bin/env python3
"""Execute native production transaction and recovery bodies with real MM types."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from
source = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp").read_text()
signatures = ["bool SoloExterior(", "bool Exterior(", "Vec3f Door(", "u16 ReturnEntrance(",
              "void StageSoloReturn(", "bool Interior(",
              "bool Safe(", "void Transition(", "void PlayerUpdate(", "void SceneInit(",
              "const Item* Offer(", "PurchaseResult Status(", "void Purchase(", "void MarketText(",
              "void ComboBlackMarket_SaveState(", "void ComboBlackMarket_LoadState(",
              'extern "C" void ComboBlackMarket_ApplyResume(',
              'extern "C" void ComboBlackMarket_NormalizeEntry(', "void LoadSolo("]
signatures.append('extern "C" int ComboBlackMarket_TryDoorEntrance(')
if "bool MatchesTravel(" in source:
    signatures.insert(0, "bool MatchesTravel(")
if "void RememberTravel(" in source:
    signatures.insert(0, "void RememberTravel(")
if 'extern "C" int ComboBlackMarket_IsAuthoredDoor(' in source:
    signatures.append('extern "C" int ComboBlackMarket_IsAuthoredDoor(')
body = "\n".join(block_from(source, source.index(s)) for s in signatures)
body += "\n" + "\n".join(block_from(source, source.index(s)) for s in
                          ("void LogDoorAttempt(", "void LogEntryRecovery("))
fixture = (ROOT / "tests/black_market/native_test.cpp").read_text().replace("/* PRODUCTION */", body)
player_source = (ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
hook_start = player_source.index("        s32 marketDoor = ComboBlackMarket_TryDoorEntrance(")
hook_end = player_source.index("#endif", hook_start)
fixture = fixture.replace("/* NATIVE_DOOR_OPEN */", re.sub(r"\bthis\b", "current", player_source[hook_start:hook_end]))
call = player_source.index("Player_Door_Knob(play, this, doorActor)", player_source.index("s32 Player_ActionHandler_1("))
begin = player_source.rfind("            } else {\n", 0, call) + len("            } else {\n")
end = player_source.index("\n            }\n", call)
tail = player_source.index("            if (this->actor.category == ACTORCAT_PLAYER)", end)
tail_end = player_source.index("\n            }\n", tail) + len("\n            }\n")
caller = player_source[begin:end] + "\n" + player_source[tail:tail_end]
caller = caller.replace("Player_Door_Knob(play, this, doorActor)", "NativeOpen(current, play, knobDoor)")
fixture = fixture.replace("/* NATIVE_DOOR_CALLER */", re.sub(r"\bthis\b", "current", caller))
door_source = (ROOT / "mm/src/overlays/actors/ovl_En_Door/z_en_door.c").read_text()
offer_begin = door_source.index("else if (", door_source.index("// 0x1800:"))
offer_end = door_source.index(" else if ((this->doorType == ENDOOR_TYPE_SCHEDULE)", offer_begin)
offer = door_source[offer_begin:offer_end].removeprefix("else ")
fixture = fixture.replace("/* NATIVE_DOOR_OFFER */", re.sub(r"\bthis\b", "current", offer))
actor_source = (ROOT / "mm/src/code/z_actor.c").read_text()
table_begin = actor_source.index("s16 D_801AED48[] = {")
table_end = actor_source.index("};", table_begin) + 2
fixture = fixture.replace("/* NATIVE_HALF_DAY_TABLE */", actor_source[table_begin:table_end])
with tempfile.TemporaryDirectory(prefix="market-native-") as temp:
    path = Path(temp) / "test.cpp"
    path.write_text(fixture)
    binary = Path(temp) / "test"
    includes = ("tests/black_market", "mm", "mm/2s2h", "mm/include", "mm/include/PR", "mm/src", "mm/assets",
                "libultraship/include", "combo")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    "-DFMT_HEADER_ONLY", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                    *["-I" + str(ROOT / p) for p in includes], str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
