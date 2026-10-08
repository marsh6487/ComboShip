"""Exercise reversible seasonal scene materials with real GBI/MM headers."""
import argparse
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--materials", type=Path, help="Optional serialized Termina foliage material directory")
    args = parser.parse_args()
    material_paths = sorted(args.materials.glob("*DL_*")) if args.materials else []
    if args.materials and not material_paths:
        parser.error("--materials must contain serialized *DL_* fixtures")
    source = (ROOT / "mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp").read_text()
    start = source.find("namespace {")
    if start < 0:
        start = source.index("void MMAutumnSceneFoliage_Update")
    flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h",
              "mm/assets", "mm/tests", "libultraship/include", "libultraship/src", "combo")]
    flags += shlex.split(os.environ.get("MM_FOLIAGE_TEST_CXXFLAGS", ""))
    with tempfile.TemporaryDirectory(prefix="mm-autumn-scene-foliage-") as temporary:
        build = Path(temporary)
        (build / "autumn_scene_foliage_production.inc").write_text(source[start:])
        fixtures = "static void CheckActualMaterials() {\n"
        if args.materials:
            for path in material_paths:
                data = path.read_bytes()
                fixtures += '{ const std::vector<Gfx> source = {\n'
                fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
                fixtures += '}; const auto variant = BuildVariant(source); assert(!variant.empty());\n'
                fixtures += 'assert(Equal(RemoveTint(variant), source)); }\n'
            fixtures += f'std::puts("PASS {len(material_paths)} actual serialized Termina bed materials and exact command preservation");\n'
        fixtures += "}\n"
        (build / "autumn_scene_foliage_fixtures.inc").write_text(fixtures)
        binary = build / "test"
        result = subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags, "-I" + str(build),
                                 str(ROOT / "mm/tests/autumn_scene_foliage_test.cpp"),
                                 str(ROOT / "libultraship/src/ship/utils/StrHash64.cpp"), "-o", str(binary)],
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
