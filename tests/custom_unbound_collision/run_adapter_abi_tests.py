#!/usr/bin/env python3
"""Exercise current Granny and Mirror assignment bodies on real engine types."""
from pathlib import Path
import os
import subprocess
import tempfile

from run_tests import ROOT, flags


def main():
    env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
    with tempfile.TemporaryDirectory(prefix="custom-wide-abi-") as directory:
        work = Path(directory)
        # This is also a UI/shared header: retain the original narrow GBI mode.
        for mode, defines in (("narrow", []), ("wide", ["-DGBI_S32_VTX"])):
            binary = work / ("shared-" + mode)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2", *defines,
                            "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include"),
                            "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            str(ROOT / "tests/black_market/granny_refinement_unit_test.cpp"),
                            "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        native = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp").read_text()
        begin = native.index("    static_assert(", native.index("void ApplyGrannyInterior()"))
        end = native.index("    sGrannyShape.polygon0", begin)
        (work / "granny_light_apply.inc").write_text(
            "static void ApplyProductionGrannyLights(PlayState* play) {\n" + native[begin:end] + "\n}\n")
        binary = work / "granny"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-I" + directory,
                        "-Werror=narrowing", "-no-pie", str(Path(__file__).with_name("adapter_abi_test.cpp")),
                        "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env=env)
        mirror = (ROOT / "combo/gameplay/MagicMirrorRuntime.inc.h").read_text()
        begin = mirror.index("    sArrivalSpawn = *sArrivalOriginal;")
        end = mirror.index("    sArrivalSpawn.rot.y = yaw;", begin)
        source = work / "mirror.cpp"
        source.write_text('''#include "global.h"
#include <cassert>
int main() {
    ActorEntry original{};
    auto* sArrivalOriginal = &original;
    ActorEntry sArrivalSpawn{};
    Vec3s position{ 32123, -27901, 1347 };
''' + mirror[begin:end] + '''
    assert(sArrivalSpawn.pos.x == 32123 && sArrivalSpawn.pos.y == -27901 && sArrivalSpawn.pos.z == 1347);
}
''')
        for host in ("soh", "mm"):
            binary = work / host
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags(host), "-Werror=narrowing",
                            *(["-DCOMBO_MAGIC_MIRROR_MM"] if host == "mm" else []), "-no-pie", str(source),
                            "-Wl,--gc-sections", "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
            print("PASS Mirror arrival native " + host + " position assignment", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
