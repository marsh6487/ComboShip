"""Run MM Demise's real item scheduling, native lightning, and effect-pool code."""
from pathlib import Path
import os
import argparse
import shlex
import subprocess
import tempfile

from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    flags = ["-std=gnu17", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
             "-ffunction-sections", "-fdata-sections", "-Werror=implicit-function-declaration",
             "-Wno-incompatible-pointer-types", "-Wno-discarded-qualifiers", "-Wno-int-conversion",
             "-include", "global.h"]
    flags += ["-I" + str(ROOT / path) for path in
              ("mm/include", "mm/include/PR", "mm/src", "mm/assets", "mm", "mm/2s2h", "mm/mods",
               "libultraship/include", "libultraship/src", "combo")]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined,float-cast-overflow", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="mm-demise-lightning-") as temp:
        temp = Path(temp)
        # Keep the real spawn argument packaging; unrelated particle allocators
        # are external boundaries recorded by the fixture, not replacement item logic.
        spawn = functions((ROOT / "mm/src/code/z_effect_soft_sprite_old_init.c").read_text())[
            "EffectSsLightning_Spawn"]
        wrapper = temp / "lightning_spawn.c"
        wrapper.write_text('#include "overlays/effects/ovl_Effect_Ss_Lightning/z_eff_ss_lightning.h"\n' + spawn)
        helper = temp / "fx_helper.c"
        helper.write_text('#include "mods/nei_oot_compat.h"\n#include "soh/_nei_compat_core.h"\n'
                          '#include "mods/items/helpers/fx_helper.c"\n')
        player_draw = functions((ROOT / "mm/src/overlays/actors/ovl_player_actor/z_player.c").read_text())["Player_Draw"]
        player_draw = player_draw.split("    Matrix_Push();")[0] + "\n}\n"
        (temp / "demise_player_draw_prefix.inc").write_text(player_draw)
        flags.append("-I" + str(temp))
        binary = temp / "test"
        compiled = subprocess.run([*shlex.split(os.environ.get("CC", "cc")), *flags,
                        "mm/tests/demise_lightning_test.c", str(helper),
                        "mm/src/overlays/effects/ovl_Effect_Ss_Lightning/z_eff_ss_lightning.c",
                        "mm/src/code/z_effect_soft_sprite.c", "mm/src/code/z_skin_matrix.c", str(wrapper),
                        "-Wl,--gc-sections", "-lm", "-o", str(binary)], cwd=ROOT, capture_output=True, text=True)
        if compiled.returncode:
            raise RuntimeError(compiled.stdout + compiled.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
