#!/usr/bin/env python3
"""Compile the native melee ABI with Keaton's actual ground/air entry code."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    opening = source.index("{", start)
    depth, end = 1, opening + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    keaton = (ROOT / "soh/mods/transformation_masks/keaton_form.cpp").read_text()
    native = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    header = (ROOT / "soh/include/functions.h").read_text()
    definition = function(native, "void func_80837918(")
    # C calls its player pointer `this`; only rename that reserved C++ token.
    definition = re.sub(r"\bthis\b", "player", definition)
    header_declarations = "\n".join(re.findall(r'^void\s+func_80837918\([^;{}]*;', header, re.M))
    declarations = "\n".join(re.findall(r'^(?:extern "C"\s+)?void\s+func_80837918\([^;{}]*;', header + "\n" + keaton, re.M))
    constants = "\n".join(re.findall(r'^const (?:u32 SLASH_KOKIRI_SWORD|f32 AIR_KICK_SPEED)[^;]*;', keaton, re.M))
    callee = '#include "native_fixture.h"\nextern "C" {\n' + header_declarations + "\n" + definition + "\n}\n"
    # Separate translation units preserve the actual C ABI boundary. The caller
    # must see the shared declaration; the native definition cannot supply it.
    body = ("#include \"native_fixture.h\"\nextern \"C\" {\n" + declarations +
            "\n}\n" + constants + "\n" +
            function(keaton, "void StartAirKick(") + "\n" +
            function(keaton, 'extern "C" u8 KeatonForm_Update(') + r'''
int main() {
    for (int mode = 0; mode != 3; ++mode) {
        for (int scenario = 0; scenario != 3; ++scenario) {
            PlayState play{}; Player player{};
            expectedPlay = &play; expectedPlayer = &player;
            damageCalls = animationCalls = 0;
            giantMask = mode == 1; elementalOverride = mode == 2;
            sState = scenario == 2 ? STATE_LONGJUMP : STATE_IDLE;
            sStep = sJumpDelay = 0;
            player.yaw = 1234;
            player.actor.bgCheckFlags = scenario == 0 ? BGCHECKFLAG_GROUND : 0;
            play.state.input[0].press.button = BTN_B;
            assert(KeatonForm_Update(&player, &play) == 1);
            assert(sState == (scenario == 0 ? STATE_COMBO : STATE_AIRKICK));
            assert(damageCalls == 2 && animationCalls == 1);
            for (const auto& quad : player.meleeWeaponQuads) {
                assert(quad.info.toucher.dmgFlags == (mode == 2 ? 0x400000u : mode == 1 ? 64u : 512u));
                assert(quad.info.toucherFlags == 5);
            }
            if (scenario != 0) {
                assert(player.actor.shape.rot.y == 1234 && player.linearVelocity == 14.f);
            }
        }
    }
    for (bool cutscene : {false, true}) {
        PlayState play{}; Player player{};
        expectedPlay = &play; expectedPlayer = &player; damageCalls = 0;
        sState = STATE_IDLE; player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
        if (cutscene) {
            player.stateFlags1 = PLAYER_STATE1_IN_CUTSCENE;
            play.state.input[0].press.button = BTN_B;
        }
        assert(KeatonForm_Update(&player, &play) == 0 && damageCalls == 0);
    }
}
''')
    # Unrelated charged-shot branches need hand locations to compile, although the
    # scenarios deliberately exercise only actual ground/air melee entry paths.
    body = "#include <initializer_list>\n#define PLAYER_BODYPART_R_HAND 0\n" + body
    with tempfile.TemporaryDirectory(prefix="keaton-melee-") as directory:
        path = Path(directory)
        (path / "native_fixture.h").write_text((ROOT / "tests/keaton_melee/native_fixture.h").read_text())
        (path / "test.cpp").write_text(body)
        (path / "native.cpp").write_text(callee)
        flags = ["-std=c++20", "-O1", "-g"]
        if args.sanitize:
            flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-no-pie"]
        subprocess.run([os.environ.get("CXX", "g++"), *flags, str(path / "test.cpp"), str(path / "native.cpp"),
                        "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True)
    print("PASS: actual Keaton ground/air/jump-kick entries, both quads, native damage overrides and controls")


if __name__ == "__main__":
    main()
