"""Execute elemental finish attribution/lifecycle and both native host bindings."""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile
import sys

from run_mm_din_fire_tests import FLAGS as MM_FLAGS
from run_time_pedestal_tests import functions, block_from

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/elemental_swords"))
from finish_adapter_bindings import build as build_adapters


def host_flags(host):
    if host == "mm":
        return [flag for flag in MM_FLAGS if flag != "-DCOMBO_BUILD"] + ["-DSWORD_FINISH_TEST_MM", "-ffunction-sections", "-fdata-sections", "-w"]
    flags = ["-std=gnu2x", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-Werror=implicit-function-declaration", "-ffunction-sections", "-fdata-sections", "-w"]
    flags += ["-I" + str(ROOT / path) for path in
              ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include", "combo")]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    return flags


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="sword-element-finish-") as directory:
        directory = Path(directory)
        core = directory / "core"
        coreflags = ["-std=gnu2x", "-Wall", "-Wextra", "-Werror", "-g", "-O1"]
        if args.sanitize:
            coreflags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-pie", "-no-pie"]
        subprocess.run([os.environ.get("CC", "cc"), *coreflags, "tests/elemental_swords/finish_core_test.c",
                        "-o", str(core)], cwd=ROOT, check=True)
        subprocess.run([str(core)], check=True)
        for host in ("soh", "mm"):
            actor_source = (ROOT / host / "src/code/z_actor.c").read_text()
            native_names = ["Actor_Kill", "Actor_ApplyDamage", "Enemy_StartFinishingBlow"]
            soul_gate = host == "soh" and "Randomizer_SoulCanDamageActor(" in actor_source
            if soul_gate:
                native_names.append("Flags_GetRandomizerInf")
                runtime = (ROOT / "soh/soh/Enhancements/randomizer/SoulShuffleRuntime.cpp").read_text()
                body = block_from(runtime, runtime.index('extern "C" int Randomizer_SoulCanDamageActor('))
                (directory / "finish_soul.cpp").write_text(
                    '#include "global.h"\n'
                    '#include "soh/Enhancements/randomizer/SoulShuffleRuntime.h"\n'
                    '#include "soh/Enhancements/randomizer/SoulShuffle.h"\n'
                    '#include "soh/Enhancements/randomizer/SoulShuffleEnemy.h"\n'
                    'extern "C" u8 Randomizer_GetSettingValue(RandomizerSettingKey);\n' + body + '\n')
            native = functions(actor_source, native_names)
            (directory / "finish_native.inc").write_text("\n".join(native[name] for name in native_names))
            collision_names = (["NEI_PlayerDamageBoostActive", "CollisionCheck_ApplyDamage", "CollisionCheck_GetSwordDamage"] if host == "soh" else
                ["CollisionCheck_InitContext", "CollisionCheck_DestroyContext", "CollisionCheck_ClearContext",
                 "CollisionCheck_GetElementATDamage", "CollisionCheck_GetDamageAndEffectOnElementAC",
                 "CollisionCheck_ApplyElementATDefense", "CollisionCheck_ApplyDamage"])
            collision_text = (ROOT / host / "src/code/z_collision_check.c").read_text()
            collision = functions(collision_text, collision_names)
            lifecycle = ""
            if host == "soh":
                # The real destructor owns host buffers through an external registry.
                # Keep its declarations, helpers and Init/Clear/Destroy together.
                begin = collision_text.index("/* Actor collider queue ownership */")
                ending = functions(collision_text, ["CollisionCheck_DisableSAC"])["CollisionCheck_DisableSAC"]
                lifecycle = collision_text[begin:collision_text.index(ending) + len(ending)] + "\n"
            hook_header = ("soh/Enhancements/game-interactor/GameInteractor_Hooks.h" if host == "soh" else
                           "2s2h/GameInteractor/GameInteractor.h")
            prefix = '#include "global.h"\n#include "din_fire_sword.h"\n#include <assert.h>\n#include <stdint.h>\n#include <stdlib.h>\n#include <string.h>\n'
            prefix += '#include "' + hook_header + '"\n'
            if host == "mm": prefix += '#include "mods/combo_rpg.h"\n'
            collision_source = directory / "finish_collision.c"
            collision_source.write_text(prefix + lifecycle + "\n".join(collision[name] for name in collision_names))
            immune_actor = "En_Rd" if host == "soh" else "En_Am"
            table_source = (ROOT / host / f"src/overlays/actors/ovl_{immune_actor}/z_{immune_actor.lower()}.c").read_text()
            immune = re.search(r'static DamageTable sDamageTable = \{.*?^\};', table_source, re.M | re.S)[0]
            tables = immune.replace("sDamageTable", "sFinishImmuneTable")
            if host == "soh":
                peahat = (ROOT / "soh/src/overlays/actors/ovl_En_Peehat/z_en_peehat.c").read_text()
                tables += "\n" + re.search(r'typedef enum \{[^{}]*\} DamageEffect;', peahat, re.S)[0]
                tables += "\n" + re.search(r'static DamageTable sDamageTable = \{.*?^\};', peahat, re.M | re.S)[0].replace(
                    "sDamageTable", "sFinishPeahatTable")
            (directory / "finish_tables.inc").write_text(tables)
            build_adapters(host, directory)
            for combo in (False, True):
                flags = host_flags(host) + ["-I" + str(directory)] + (["-DCOMBO_BUILD"] if combo else [])
                if args.sanitize:
                    flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                              "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", "-fno-sanitize=shift-base"]
                soul_objects = []
                if soul_gate:
                    flags += ["-DSWORD_FINISH_SOUL_GATE"]
                    soul_object = directory / ("soul-combo.o" if combo else "soul-solo.o")
                    # Compile the exact C++ gate with the same native ABI/host
                    # definitions. C-only language flags have no C++ meaning.
                    cpp_flags = [flag for flag in flags if not flag.startswith("-std=") and
                                 flag != "-Werror=implicit-function-declaration"]
                    subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20", *cpp_flags,
                                    "-c", str(directory / "finish_soul.cpp"), "-o", str(soul_object)],
                                   cwd=ROOT, check=True)
                    soul_objects = [str(soul_object)]
                binary = directory / (host + ("-combo" if combo else "-solo"))
                subprocess.run([os.environ.get("CC", "cc"), *flags,
                                "tests/elemental_swords/finish_override_render.c",
                                f"{host}/src/code/din_fire_sword.c", "-Wl,--gc-sections", "-lm", "-o", str(binary)],
                               cwd=ROOT, check=True)
                subprocess.run([str(binary)], check=True)
                host_binary = str(binary) + "-finish"
                subprocess.run([os.environ.get("CC", "cc"), *flags,
                                "tests/elemental_swords/finish_host_test.c", f"{host}/src/code/din_fire_sword.c",
                                str(collision_source), *soul_objects, "-Wl,--gc-sections", "-lm", "-o", host_binary],
                               cwd=ROOT, check=True)
                subprocess.run([host_binary], check=True)
                adapter_binary = str(binary) + "-adapters"
                subprocess.run([os.environ.get("CC", "cc"), *flags,
                                "tests/elemental_swords/finish_adapters_test.c", f"{host}/src/code/din_fire_sword.c",
                                str(collision_source), *soul_objects, "-Wl,--gc-sections", "-lm", "-o", adapter_binary],
                               cwd=ROOT, check=True)
                subprocess.run([adapter_binary], check=True)
                draw_binary = str(binary) + "-draw"
                subprocess.run([os.environ.get("CC", "cc"), *flags,
                                "tests/elemental_swords/finish_draw_test.c", f"{host}/src/code/din_fire_sword.c",
                                str(collision_source), *soul_objects, "-Wl,--gc-sections", "-lm", "-o", draw_binary], cwd=ROOT, check=True)
                subprocess.run([draw_binary], check=True)
                for source in ("elemental_sword_finish", "din_fire_sword", "z_actor", "z_collision_check"):
                    subprocess.run([os.environ.get("CC", "cc"), *flags, "-c", f"{host}/src/code/{source}.c",
                                    "-o", str(binary) + "-" + source + ".o"], cwd=ROOT, check=True)
                actors = ("En_Bubble", "Boss_Dodongo", "Boss_Va", "En_Fd_Fire") if host == "soh" else (
                    "En_Bubble", "En_Rat", "En_Tanron3", "En_Minideath", "Boss_07", "En_Baguo", "En_Invadepoh")
                for actor in actors:
                    source = f"{host}/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c"
                    subprocess.run([os.environ.get("CC", "cc"), *flags, "-c", source,
                                    "-o", str(binary) + "-" + actor + ".o"], cwd=ROOT, check=True)
                print("PASS native finish objects", host, "Combo" if combo else "solo", "4 core +", len(actors), "actors")


if __name__ == "__main__":
    main()
