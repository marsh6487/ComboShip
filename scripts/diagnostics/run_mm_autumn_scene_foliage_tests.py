"""Exercise reversible seasonal scene materials with real GBI/MM headers."""
import argparse
import json
import os
from pathlib import Path
import shlex
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--materials", type=Path, help="Optional serialized Termina foliage material directory")
    parser.add_argument("--poc4", type=Path, help="Optional supplied POC4 archive for exact private brush coverage")
    parser.add_argument("--mm", type=Path, help="Optional native MM archive for both forest-layer controls")
    args = parser.parse_args()
    material_paths = sorted(args.materials.glob("*DL_*")) if args.materials else []
    if args.materials and not material_paths:
        parser.error("--materials must contain serialized *DL_* fixtures")
    coverage_cases = []
    if args.poc4:
        with zipfile.ZipFile(args.poc4) as archive:
            for suffix, texture, triangles in (("0153A8", "leaves_rgba", 3576), ("015DF0", "leaves_rgba", 997),
                                                ("016598", "leaves_rgba", 2296), ("0158E8", "stems_rgba", 244),
                                                ("016180", "stems_rgba", 142), ("016A38", "stems_rgba", 200)):
                path = f"alt/scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_{suffix}"
                texture = f"scenes/nonmq/Z2_00KEIKOKU/foliage_poc3/{texture}"
                coverage_cases.append((path, texture, triangles, archive.read(path)))
    if args.mm:
        path = "scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_0241B0"
        with zipfile.ZipFile(args.mm) as archive:
            for suffix, triangles in (("034098", 14), ("037098", 8)):
                texture = f"scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_{suffix}"
                coverage_cases.append((path, texture, triangles, archive.read(path)))
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
        for path, texture, triangles, data in coverage_cases:
            if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                raise ValueError(f"Invalid serialized display list: {path}")
            fixtures += '{ const std::vector<Gfx> source = {\n'
            fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
            fixtures += f'}}; CheckActualMaterial({json.dumps(path)}, source, {json.dumps(texture)}, {triangles}); }}\n'
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
