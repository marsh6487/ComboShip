"""Exercise the production barrier draw and color commands without running the game."""
from pathlib import Path
import subprocess
import tempfile

from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "soh/mods/transformation_masks/mm_player_form.cpp").read_text()
    bodies = functions(source)
    patch = bodies.get("MmForm_PatchZoraBarrierColors", "static void MmForm_PatchZoraBarrierColors(Gfx*, size_t) {}")
    with tempfile.TemporaryDirectory(prefix="zora-barrier-") as directory:
        out = Path(directory)
        (out / "zora_barrier_production.inc").write_text(patch + "\n" + bodies["MmForm_DrawZoraBarrier"])
        binary = out / "test"
        subprocess.run([
            "c++", "-std=gnu++20", "-O1", "-g", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
            '-DCVAR_PREFIX_COSMETIC="gCosmetics"', "-Isoh/include", "-Isoh/src", "-Isoh/assets", "-Isoh",
            "-Ilibultraship/include", "-I" + str(out), "soh/tests/zora_barrier_cosmetics_test.cpp",
            "-o", str(binary),
        ], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
