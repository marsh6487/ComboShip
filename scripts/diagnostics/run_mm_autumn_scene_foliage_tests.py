"""Exercise reversible seasonal scene materials with real GBI/MM headers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def production_function(source, name):
    match = re.search(r"^(?:static )?(?:bool|void\*?|int32_t|F3DGfx\*&?)\s+" +
                      re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise RuntimeError(f"Missing production function: {name}")
    masked = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',
                    lambda m: " " * len(m.group()), source, flags=re.S)
    pos, depth = match.end(), 1
    while depth:
        depth += (masked[pos] == "{") - (masked[pos] == "}")
        pos += 1
    return source[match.start():pos] + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--materials", type=Path, help="Optional serialized Termina foliage material directory")
    parser.add_argument("--poc4", type=Path, help="Optional supplied POC4 archive for exact private brush coverage")
    parser.add_argument("--mm", type=Path, help="Optional native MM archive for both forest-layer controls")
    parser.add_argument("--all-scene-materials", action="store_true",
                        help="Check frozen grass/forest and negative material cases from the full --mm archive")
    args = parser.parse_args()
    if args.all_scene_materials and not args.mm:
        parser.error("--all-scene-materials requires --mm")
    material_paths = sorted(args.materials.glob("*DL_*")) if args.materials else []
    if args.materials and not material_paths:
        parser.error("--materials must contain serialized *DL_* fixtures")
    coverage_cases = []
    scene_cases = []
    texture_paths = {}
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
            if args.all_scene_materials:
                # Use actual archive identities for the extracted texture-hash handlers.
                from_paths = [p for p in archive.namelist() if "Tex_" in p or "TLUT_" in p]
                # CRC64 is computed by the C++ fixture; resolve only the curated paths below.
                texture_paths = from_paths
            for suffix, triangles in (("034098", 14), ("037098", 8)):
                texture = f"scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKUTex_{suffix}"
                coverage_cases.append((path, texture, triangles, archive.read(path)))
            if args.all_scene_materials:
                reference = json.loads((ROOT / "mm/tests/autumn_scene_material_cases.json").read_text())
                for case in reference["cases"]:
                    data = archive.read(case["path"])
                    if hashlib.sha256(data).hexdigest() != case["resource_sha256"]:
                        raise ValueError(f"Native scene fixture hash differs: {case['path']}")
                    scene_cases.append((case, reference["palette"], data))
                if not scene_cases:
                    raise ValueError("The requested all-scene material reference has no cases")
    source = (ROOT / "mm/2s2h/Enhancements/Graphics/AutumnSceneFoliage.cpp").read_text()
    renderer = (ROOT / "libultraship/src/fast/interpreter.cpp").read_text()
    manager = (ROOT / "libultraship/src/ship/resource/ResourceManager.cpp").read_text()
    start = source.find("namespace {")
    if start < 0:
        start = source.index("void MMAutumnSceneFoliage_Update")
    flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
             "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING"]
    flags += ["-I" + str(ROOT / p) for p in ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h",
              "mm/assets", "mm/tests", "mm/2s2h/Enhancements/Graphics", "libultraship/include", "libultraship/src", "combo")]
    flags += shlex.split(os.environ.get("MM_FOLIAGE_TEST_CXXFLAGS", ""))
    with tempfile.TemporaryDirectory(prefix="mm-autumn-scene-foliage-") as temporary:
        build = Path(temporary)
        (build / "autumn_scene_foliage_production.inc").write_text(source[start:])
        renderer_functions = ["GfxExecStack::start", "GfxExecStack::stop", "GfxExecStack::currCmd",
                              "GfxExecStack::branch", "GfxExecStack::call", "GfxExecStack::ret",
                              "Interpreter::SegAddr", "gfx_check_image_signature",
                              "ComboIsUnresolvedSegmentTarget", "ComboResolveDisplayListTarget",
                              "gfx_dl_handler_common", "gfx_end_dl_handler_common", "gfx_marker_handler_otr",
                              "gfx_vtx_hash_handler_custom", "gfx_set_timg_otr_hash_handler_custom",
                              "gfx_set_grayscale_handler_custom", "gfx_set_intensity_handler_custom",
                              "Interpreter::GfxDpSetGrayscaleColor", "gfx_quad_handler_f3dex2", "gfx_tri1_handler_f3dex2"]
        (build / "autumn_scene_renderer_production.inc").write_text(
            "\n".join(production_function(renderer, name) for name in renderer_functions))
        (build / "autumn_scene_renderer_signature.inc").write_text(
            production_function(manager, "ResourceManager::OtrSignatureCheck"))
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
        for case, palette, data in scene_cases:
            if len(data) < 72 or data[4:8] != b"TLDO" or (len(data) - 72) % 8:
                raise ValueError(f"Invalid serialized scene display list: {case['path']}")
            fixtures += '{ const std::vector<Gfx> source = {\n'
            fixtures += ",\n".join(f"Command(0x{a:08X}, 0x{b:08X})" for a, b in struct.iter_unpack("<II", data[72:]))
            expected = ', '.join('0x' + palette[i] for i in case['triangle_palettes'])
            fixtures += f'}}; CheckActualSceneMaterial({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{expected}}});\n'
            if case['path'] in (
                    'scenes/nonmq/Z2_00KEIKOKU/Z2_00KEIKOKU_room_00DL_005EC8',
                    'scenes/nonmq/Z2_BACKTOWN/Z2_BACKTOWN_room_00DL_004BE0',
                    'scenes/nonmq/Z2_ALLEY/Z2_ALLEY_room_00DL_003FF0',
                    'scenes/nonmq/Z2_ROMANYMAE/Z2_ROMANYMAE_room_00DL_001CB8'):
                # Include the scene's texture identities plus shared scene textures;
                # the native lists keep their exact reference hashes and offsets.
                folder = '/'.join(case['path'].split('/')[:3]) + '/'
                textures = ', '.join(json.dumps(p) for p in texture_paths
                                     if p.startswith(folder) or p.startswith('misc/scene_texture_'))
                fixtures += f'CheckActualGrassRenderer({json.dumps(case["path"])}, {case["scene_id"]}, source, {{{expected}}}, {{{textures}}});\n'
            fixtures += '}\n'
        if scene_cases:
            fixtures += f'std::puts("PASS {len(scene_cases)} frozen native scene materials: exact triangle palettes, native/Alt, both namespaces and Off restoration");\n'
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
