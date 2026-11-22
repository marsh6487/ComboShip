#!/usr/bin/env python3
"""Verify the optional native rupee HUD hiding without changing inventory updates."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_scene_randomization_tests import function


def block(source, start):
    opening = source.index("{", start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


sources = {host: (ROOT / host / "src/code/z_parameter.c").read_text() for host in ("soh", "mm")}
tycoon = (ROOT / "mm/2s2h/Rando/MiscBehavior/TycoonWallet.cpp").read_text()
for name, source in [*sources.items(), ("tycoon", tycoon)]:
    count = source.count("if (PickupCounter_ShouldDrawWallet(play))")
    assert count == (2 if name == "soh" else 3), (name, count)
    if name != "tycoon":
        for method in ("Interface_Update", "Rupees_ChangeBy", "Inventory_ChangeAmmo"):
            assert "PickupCounter_ShouldDrawWallet" not in function(source, method), (name, method)

native = sources["mm"]
guard = re.search(r"\bif\s*\([^;{}]*\bVB_DRAW_RUPEE_COUNTER\b[^;{}]*\)\s*\{", native)
production = "void TestNativeDraw(PlayState* play, bool fullUi) {\n"
production += "InterfaceContext* interfaceCtx = &play->interfaceCtx;\n"
production += "s16 counterDigits[5], sp2CC, sp2C8, sp2CE, sp2CA, magicAlpha;\n"
production += "OPEN_DISPS(play->state.gfxCtx);\n" + block(native, guard.start())
production += "\nCLOSE_DISPS(play->state.gfxCtx);\n}\n"
production += function(tycoon, "DrawTycoonRupeeCounter")
fixture = (ROOT / "tests/mm_wallet/rupee_draw_test.c").read_text()
fixture = fixture.replace("../../combo/hud/PickupCounterWallet.h", "combo/hud/PickupCounterWallet.h")
fixture = fixture.replace("static bool nativeFullUi = true;", "static bool nativeFullUi = true;\nstatic int pickup, hideHud;")
fixture = fixture.replace("int32_t CVarGetInteger(const char* name, int32_t defaultValue) { return defaultValue; }", '''
int32_t CVarGetInteger(const char* name, int32_t defaultValue) {
    if (!strcmp(name, "gEnhancements.PickupCounter")) return pickup;
    if (!strcmp(name, "gEnhancements.PickupCounterHideHudRupees")) return hideHud;
    return defaultValue;
}
''')
extra = '''
    play.state.running = true;
    play.sceneId = SCENE_20SICHITAI2;
    for (pickup = 0; pickup <= 1; ++pickup) {
        for (hideHud = 0; hideHud <= 2; ++hideHud) {
            const bool show = !pickup || hideHud != 1;
            checkDraw(1667, 2, 0, show ? "1667" : "");
            checkDraw(5000, 3, 1, show ? "5000" : "");
        }
    }
    pickup = hideHud = 1;
    play.sceneId = SCENE_8ITEMSHOP;
    checkDraw(1667, 2, 0, "1667");
    checkDraw(5000, 3, 1, "5000");
    nativeFullUi = false;
    checkDraw(1667, 2, 0, "");
    nativeFullUi = true;
    play.sceneId = SCENE_20SICHITAI2;
    static Actor merchant = { .id = ACTOR_EN_AKINDONUTS };
    play.msgCtx.talkActor = &merchant;
    play.msgCtx.msgMode = MSGMODE_TEXT_DISPLAYING;
    checkDraw(1667, 2, 0, "1667");
    checkDraw(5000, 3, 1, "5000");
    play.msgCtx.msgMode = MSGMODE_NONE;
    checkDraw(1667, 2, 0, "");
    checkDraw(5000, 3, 1, "");
    play.msgCtx.talkActor = NULL;
    pickup = hideHud = 0;
'''
fixture = fixture.replace("    // The same native draw block", extra + "\n    // The same native draw block")

with tempfile.TemporaryDirectory(prefix="pickup-hud-") as directory:
    temp = Path(directory)
    (temp / "wallet_draw_production.inc").write_text(production)
    (temp / "test.c").write_text(fixture)
    compiler = shlex.split(os.environ.get("CC", "cc"))
    flags = ["-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DLOG_LEVEL_GAME_PRINTS=0", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"']
    for host in ("soh", "mm"):
        includes = [host + "/include", host + "/include/PR", host + "/src", host + "/assets", host,
                    host + ("/soh" if host == "soh" else "/2s2h"), host + "/mods",
                    "libultraship/include", "combo", "combo/menu", ""]
        hostflags = flags + ["-I" + str(ROOT / path) for path in includes]
        if host == "soh":
            hostflags += ['-D' + key + '="' + value + '"' for key, value in re.findall(
                r'set\((CVAR_PREFIX_\w+) "([^"\n]+)"\)', (ROOT / "CMake/soh-cvars.cmake").read_text())
                          if key != "CVAR_PREFIX_ENHANCEMENT"]
        for combo in (False, True):
            mode = ["-DCOMBO_BUILD"] if combo else []
            subprocess.run([*compiler, "-std=gnu11", *hostflags, *mode,
                            "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-Wno-discarded-qualifiers",
                            "-Werror=implicit-function-declaration", "-fsyntax-only",
                            str(ROOT / host / "src/code/z_parameter.c")], check=True)
            wallet_policy = temp / "wallet_policy"
            host_policy = ["-DPICKUP_COUNTER_WALLET_MM"] if host == "mm" else []
            subprocess.run([*compiler, "-std=gnu11", *hostflags, *mode, *host_policy,
                            str(ROOT / "tests/pickup_counter/wallet_shop_test.c"), "-o", str(wallet_policy)], check=True)
            subprocess.run([str(wallet_policy)], check=True)
            if host == "mm":
                binary = temp / "test"
                subprocess.run([*compiler, "-std=gnu11", "-g", "-O1", *hostflags, *mode, "-I" + str(temp),
                                "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie",
                                str(temp / "test.c"), "-o", str(binary)], check=True)
                subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
print("PASS both native HUD compile guards, eight actual draw guards, MM/Tycoon rendering and draw-only option scope")
