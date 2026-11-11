#!/usr/bin/env python3
"""Exercise the real native Din actor lifecycle and ignition on both host layouts."""
import argparse
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--host", choices=("soh", "mm"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="dins-native-actor-") as directory:
        build = Path(directory)
        for host in ((args.host,) if args.host else ("soh", "mm")):
            flags = ["-std=gnu2x", "-O1", "-g", "-ffunction-sections", "-fdata-sections",
                     "-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DCOMBO_BUILD",
                     "-D" + ("MM_BUILD_DLL" if host == "mm" else "SOH_BUILD_DLL"),
                     "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
                     "-DLOG_LEVEL_GAME_PRINTS=0", "-Werror=implicit-function-declaration",
                     "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
            if args.sanitize:
                flags += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                          "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            flags += ["-I" + str(ROOT / path) for path in
                      (".", host + "/include", host + "/include/PR", host + "/src", host + "/assets", host,
                       host + ("/2s2h" if host == "mm" else "/soh"), "libultraship/include", "libultraship/src", "combo")]
            flags += ["-I" + str(build)]
            for name, value in re.findall(r'set\((CVAR_PREFIX_\w+) "([^"]+)"\)',
                                          (ROOT / "CMake/soh-cvars.cmake").read_text()):
                flags.append(f'-D{name}="{value}"')
            path = ("mm/expansions/sw97/actors/spells/z_magic_fire.inc.c" if host == "mm" else
                    "soh/src/overlays/actors/ovl_Magic_Fire/z_magic_fire.c")
            source = (ROOT / path).read_text()
            actor_code = (ROOT / host / "src/code/z_actor.c").read_text()
            scheduler = [function(actor_code, "Actor_AddToCategory")]
            if host == "mm":
                scheduler += [function(actor_code, "Actor_RemoveFromCategory"), function(actor_code, "Actor_ChangeCategory")]
                start = actor_code.index("    // Move actors to a different actorList if it has changed categories.")
                end = actor_code.index("    actor = player->focusActor;", start)
                scheduler += ["static void flushCategories(PlayState* play) { ActorContext* actorCtx = &play->actorCtx; "
                              "s32 category; ActorListEntry* entry; Actor *actor, *next; u8 newCategory;\n" +
                              actor_code[start:end] + "\n}"]
            (build / "dins_native_actor_scheduler.inc").write_text("\n".join(scheduler))
            pieces = []
            if host == "mm":
                flags += ["-DTEST_HOST_MM"]
                pieces += [source[source.index("typedef struct MagicFire {"):
                                  source.index("} MagicFire;") + len("} MagicFire;")],
                           "#define THIS ((MagicFire*)thisx)"]
                prefix = "MagicFireDins_"
            else:
                pieces += ['#include "overlays/actors/ovl_Magic_Fire/z_magic_fire.h"',
                           re.search(r"static InitChainEntry sInitChain\[\] = \{.*?\};", source, re.S)[0]]
                prefix = "MagicFire_"
            pieces += re.findall(r"typedef enum \{.*?\} MagicFire(?:Action|ScreenTint);", source, re.S)
            pieces += [re.search(r"static ColliderCylinderInit sCylinderInit = \{.*?^\};", source, re.S | re.M)[0]]
            names = [prefix + name for name in ("Init", "Destroy", "UpdateBeforeCast", "Update")]
            pieces += [("static " if host == "mm" else "") + "void " + name + "(Actor*, PlayState*);" for name in names]
            if "DinsFireNativeActor.h" in source:
                pieces += ['#include "combo/render/DinsFireNativeActor.h"']
            pieces += [function(source, name) for name in names]
            (build / "dins_native_actor_production.inc").write_text("\n".join(pieces))
            executable = build / host
            compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                                       "combo/tests/dins_fire_native_actor_test.c", "-Wl,--gc-sections", "-lm",
                                       "-o", str(executable)], cwd=ROOT, capture_output=True, text=True)
            if compiled.returncode:
                raise RuntimeError(compiled.stdout + compiled.stderr)
            subprocess.run([str(executable)], check=True, cwd=ROOT)


if __name__ == "__main__":
    main()
