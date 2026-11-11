#!/usr/bin/env python3
"""Execute the production Din cast/setup/recovery actions for both native hosts.

The host's real Player/Actor layouts and animation advance functions are compiled.
The fixture supplies animation resources and engine boundaries; this is source
behavior verification, not a claim about the pose in an unavailable game archive.
"""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def declaration(source, name):
    match = re.search(r"^static [^;{}]*\b" + re.escape(name) + r"\b[^;]*;", source, re.M | re.S)
    if match is None:
        raise RuntimeError("Missing production declaration: " + name)
    return match[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--standalone", action="store_true", help="Compile without the combined-game guard")
    parser.add_argument("--host", choices=("soh", "mm"))
    parser.add_argument("--case", choices=("cast", "ownership", "fallback", "world", "camera", "cancellation", "audio"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dins-gameplay-") as directory:
        build = Path(directory)
        for host in ((args.host,) if args.host else ("soh", "mm")):
            flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                     "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX",
                     "-D" + ("MM_BUILD_DLL" if host == "mm" else "SOH_BUILD_DLL"),
                     "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-Werror=implicit-function-declaration",
                     "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
            if not args.standalone:
                flags.append("-DCOMBO_BUILD")
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                          # Native player flags include the pre-existing signed
                          # (1 << 31) definition; retain every other UB check.
                          "-fno-sanitize=shift-base", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            flags += ["-I" + str(ROOT / path) for path in
                      (".", host + "/include", host + "/include/PR", host + "/src", host + "/assets", host,
                       host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "libultraship/src", "combo")]
            flags += ["-I" + str(build)]
            for name, value in re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                                          (ROOT / "CMake/soh-cvars.cmake").read_text()):
                flags.append(f'-D{name}="{value}"')
            native_player = (ROOT / host / "src/overlays/actors/ovl_player_actor/z_player.c").read_text()
            sfx_start = native_player.index("typedef enum AnimSfxType {")
            sfx_end = native_player.index("} AnimSfxEntry;", sfx_start) + len("} AnimSfxEntry;")
            (build / "dins_gameplay_sfx_types.inc").write_text(native_player[sfx_start:sfx_end])
            sfx_dispatch = "Player_PlayAnimSfx" if host == "mm" else "Player_ProcessAnimSfxList"
            (build / "dins_gameplay_sfx_dispatch.inc").write_text(function(native_player, sfx_dispatch))
            player_path = ("mm/mods/items/logic/item_oot_spells.c" if host == "mm" else
                           "soh/src/overlays/actors/ovl_player_actor/z_player.c")
            source = (ROOT / player_path).read_text()
            if host == "mm":
                flags.append("-DTEST_HOST_MM")
                start = source.index("typedef enum {", source.index("// Cast animations"))
                end = source.index("// ---------------------------------------------------------------------------\n// Spawn", start)
                pieces = [source[start:end], declaration(source, "sOotMagicSpellCosts"),
                          declaration(source, "sOotSw97SpellActive")]
                names = ["OotSpells_ActionToMagicSpell", "OotSpells_SceneAllowsFarores",
                         "OotSpells_SpawnMagicSpell", "OotSpells_TryUseItem", "OotSpells_ReleasePendingDinMagic",
                         "Player_DinsFireRemakeSetDuration",
                         "Player_DinsFireRemakeProtect",
                         "OotSpells_SetupCast", "Player_UpdateDinsFireRemakeCast", "Player_Action_OotMagicSpell"]
                animation_prefix = "PlayerAnimation_"
            else:
                pieces = [source[source.index("static LinkAnimationHeader* D_80854A58[]"):
                                 source.index("static void Player_UpdateDinsFireRemakeCast(")],
                          declaration(source, "sMagicSpellCosts"), declaration(source, "sSw97SpellActive")]
                names = ["func_80832340", "Player_SetupAction", "Player_SetupActionPreserveItemAction", "func_80839FFC",
                         "Player_SpawnMagicSpell", "Player_DinsFireRemakeSetDuration", "Player_DinsFireRemakeProtect",
                         "func_8083AF44",
                         "Player_UpdateDinsFireRemakeCast", "Player_Action_808507F4"]
                animation_prefix = "LinkAnimation_"
            pieces += [declaration(source, "sDinsFireRemakeCast"), declaration(source, "sDinsFireRemakeCastTicks"),
                       declaration(source, "sDinsFireRemakePendingMagic"),
                       declaration(source, "sDinsFireRemakeAudioHandle")]
            actor = (ROOT / host / "src/code/z_actor.c").read_text()
            freeze_name = "sCategoryFreezeMasks" if host == "mm" else "D_80116068"
            pieces += [re.search(r"^u32 " + freeze_name + r"\[ACTORCAT_MAX\] = \{.*?^\};", actor, re.M | re.S)[0],
                       "#define TEST_FREEZE_MASKS " + freeze_name]
            player_lib = (ROOT / host / "src/code/z_player_lib.c").read_text()
            if host == "soh":
                pieces += [function(player_lib, name) for name in ("func_8008EC70", "Player_ActionToMagicSpell")]
            else:
                pieces += [function(player_lib, "func_80123C58")]
            pieces += [function(player_lib, name) for name in ("Player_InBlockingCsMode", "Player_InCsMode")]
            names.append("Player_DrawDinsFireRemakeCharge")
            pieces += [function(source, name) for name in names]
            if host == "mm":
                native_player = (ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
                pieces += [function(native_player, name) for name in (
                    "Player_SetAction", "Player_SetAction_PreserveItemAction", "func_8085B384", "func_80836988")]
            (build / "dins_gameplay_production.inc").write_text("\n".join(pieces))
            animations = (ROOT / host / "src/code/z_skelanime.c").read_text()
            (build / "dins_gameplay_animation.inc").write_text("\n".join(
                function(animations, name) for name in
                (animation_prefix + "Update", animation_prefix + "Once", animation_prefix + "Loop",
                 "Animation_OnFrameImpl", animation_prefix + "OnFrame")))
            executable = build / host
            command = [*shlex.split(os.environ.get("CC", "cc")), *flags,
                       "combo/tests/dins_fire_native_gameplay_test.c", "-Wl,--gc-sections", "-lm", "-o", str(executable)]
            compiled = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
            if compiled.returncode:
                raise RuntimeError(compiled.stdout + compiled.stderr)
            subprocess.run([str(executable), *([args.case] if args.case else [])], check=True, cwd=ROOT)


if __name__ == "__main__":
    main()
