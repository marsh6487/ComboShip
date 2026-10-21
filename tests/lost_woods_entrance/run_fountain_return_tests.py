#!/usr/bin/env python3
"""Exercise the production collision-exit path with native OoT save/player types."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()
player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
begin = player.index("static s16 sReturnEntranceGroupData[]")
tables = player[begin:player.index("s32 Player_HandleExitsAndVoids(", begin)]
begin = player.index("                play->nextEntranceIndex = play->setupExitList[exitIndex - 1];")
exit_body = player[begin:player.index("                play->transitionTrigger = TRANS_TRIGGER_START;", begin)]
route_path = ROOT / "soh/soh/Enhancements/Gameplay/ComboLostWoodsFountain.cpp"
route = route_path.read_text() if route_path.exists() else ""
production = route[route.index("namespace {"):route.rindex("#endif")] if route else ""
fixture = (ROOT / "tests/lost_woods_entrance/fountain_return_test.cpp").read_text()
fixture = fixture.replace("/* PRODUCTION_ROUTE */", production)
fixture = fixture.replace("/* NATIVE_RETURN_TABLES */", tables)
fixture = fixture.replace("/* NATIVE_COLLISION_EXIT */", re.sub(r"\bthis\b", "current", exit_body))
if route:
    fixture = "#define HAS_FOUNTAIN_ROUTE\n" + fixture
with tempfile.TemporaryDirectory(prefix="lost-woods-fountain-") as temp:
    source, binary = Path(temp) / "test.cpp", Path(temp) / "test"
    source.write_text(fixture)
    flags = ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie"] if args.sanitize else []
    includes = ("soh", "soh/include", "soh/include/PR", "soh/soh", "soh/src", "soh/assets",
                "libultraship/include", "combo")
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                    *flags, *["-I" + str(ROOT / path) for path in includes], str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

if route:
    play = (ROOT / "soh/src/code/z_play.c").read_text()
    init = block_from(play, play.index("void Play_Init("))
    assert init.index("ComboLostWoodsFountain_NormalizeEntry();") < init.index("gPlayState = play;"), \
        "saved fountain visits must be normalized before the scene/player load"
