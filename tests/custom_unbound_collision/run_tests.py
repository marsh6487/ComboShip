#!/usr/bin/env python3
"""Compile real adapters on native game types; run with ASan/UBSan.

Rendering is not executed. Native collision registration and surface-accessor
functions are taken verbatim from the current game sources, with logging as the
only replaced service. These checks are CPU evidence, not configured game proof.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r"^[A-Za-z_][^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise AssertionError("Missing production function: " + name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def flags(host):
    return ["-DF3DEX_GBI_2", "-DGBI_FLOAT_MTX", "-DGBI_S32_VTX", "-DCOMBO_BUILD", "-DLOG_LEVEL_GAME_PRINTS=0",
            '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DCVAR_PREFIX_SETTING="gSettings"',
            *["-I" + str(ROOT / p) for p in (".", host, host + "/include", host + "/include/PR",
              host + "/src", host + "/assets", host + ("/soh" if host == "soh" else "/2s2h"),
              "combo", "libultraship/include", "soh")],
            "-g", "-O1", "-ffunction-sections", "-fdata-sections", "-fno-pie",
            "-fsanitize=address,undefined", "-fno-sanitize-recover=all"]


def native_unit(host, names):
    source = (ROOT / host / "src/code/z_bgcheck.c").read_text()
    actor = (ROOT / host / "src/code/z_bg_item.c").read_text()
    functions = [function(actor if n == "DynaPolyActor_Init" else source, n) for n in names]
    declarations = "\n".join(f[:f.index("{")].strip() + ";" for f in functions)
    hooks = ('#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"\n'
             'bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) { return result; }\n'
             if host == "soh" else '')
    defines = "\n".join(re.findall(r"^#define SS_NULL[^\n]*", source, re.M)) + "\n"
    return ('#include "global.h"\n#include "vt.h"\n#include <assert.h>\n#include <stdlib.h>\n' + hooks +
            '#undef osSyncPrintf\n#define osSyncPrintf(...) ((void)0)\n'
            '#undef LOG_HUNGUP_THREAD\n#define LOG_HUNGUP_THREAD() abort()\n' +
            defines + declarations + "\n" + "\n".join(functions))


def native_queue_owner():
    source = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    begin = source.index("/* Actor collider queue ownership */")
    ending = function(source, "CollisionCheck_DisableSAC")
    end = source.index(ending) + len(ending)
    return ('\n#include <stdint.h>\n#include <stdlib.h>\n#include <string.h>\n'
            '#undef AREG\nstatic s16 sFixtureAregs[24];\n#define AREG(index) sFixtureAregs[index]\n'
            'void DinFireSword_FinishTeardown(PlayState* play) { (void)play; }\n' + source[begin:end])


def native_oc_queue():
    source = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    names = ["Collider_ResetOCBase",
             "Collider_ResetOCInfo", "Collider_ResetJntSphElementOC", "Collider_ResetTrisElementOC",
             "Collider_ResetJntSphOC", "Collider_ResetCylinderOC", "Collider_ResetTrisOC", "Collider_ResetQuadOC"]
    bodies = [function(source, name) for name in names]
    declarations = "\n".join(body[:body.index("{")].strip() + ";" for body in bodies)
    begin = source.index("static ColChkResetFunc sOCResetFuncs[]")
    end = source.index("};", begin) + 2
    return ('\n#include <stdint.h>\n#include <string.h>\n'
            'u8 GameInteractor_SecondCollisionUpdate(void) { return 0; }\n'
            's32 FrameAdvance_IsEnabled(PlayState* play) { return 0; }\n'
            'typedef s32 (*ColChkResetFunc)(PlayState*, Collider*);\n' + native_queue_owner() + declarations + "\n" +
            "\n".join(bodies) + "\n" + source[begin:end] + "\n" + function(source, "CollisionCheck_SetOC"))


def native_ac_queue():
    source = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    names = ["Collider_ResetACBase", "Collider_ResetACInfo", "Collider_ResetJntSphElementAC", "Collider_ResetTrisElementAC",
             "Collider_ResetJntSphAC", "Collider_ResetCylinderAC", "Collider_ResetTrisAC", "Collider_ResetQuadAC"]
    bodies = [function(source, name) for name in names]
    declarations = "\n".join(body[:body.index("{")].strip() + ";" for body in bodies)
    begin = source.index("static ColChkResetFunc sACResetFuncs[]")
    end = source.index("};", begin) + 2
    return ('#include "global.h"\n#include <stdint.h>\n#include <stdlib.h>\n#include <string.h>\n#include <assert.h>\n'
            '#undef osSyncPrintf\n#define osSyncPrintf(...) ((void)0)\n'
            'u8 GameInteractor_SecondCollisionUpdate(void) { return 0; }\n'
            's32 FrameAdvance_IsEnabled(PlayState* state) { return 0; }\n'
            'void TimeCtl_NoteAcCollider(Collider* collider);\n'
            'typedef s32 (*ColChkResetFunc)(PlayState*, Collider*);\n' + native_queue_owner() + declarations + "\n" +
            "\n".join(bodies) + "\n" + source[begin:end] + "\n" + function(source, "CollisionCheck_SetAC"))


def wide_motion():
    hook = (ROOT / "soh/mods/items/logic/item_switchhook.c").read_text()
    mario = (ROOT / "soh/expansions/sm64/sm64_mario.c").read_text()
    begin = mario.index("        u32 dynaSig = 0u;")
    end = mario.index("        sSurfRefresh++;", begin)
    return (function(hook, "SwitchHook_ShiftCollider") +
            "\nstatic u32 ProductionDynaSignature(PlayState* play) {\n" + mario[begin:end] +
            "return dynaSig;\n}\n")


def actor_adapters():
    stasis = (ROOT / "soh/mods/actors/stasis_rune.c").read_text()
    pacci = (ROOT / "soh/mods/actors/cane_pacci.c").read_text()
    magnet = (ROOT / "soh/mods/items/helpers/switch_magnet.c").read_text()
    sand = (ROOT / "soh/mods/items/logic/wand/wand_sand.c").read_text()
    parts = [function(stasis, "Stasis_FindBg"),
             re.search(r"^#define STASIS_WALL_TYPE_CLIMBABLE[^\n]*", stasis, re.M).group(0)]
    parts += [stasis[stasis.index("#define STASIS_MAX_RINGS"):stasis.index("// The body the collision is registered ON.")]]
    parts += ["static DynaPolyActor sStasisCarrier;", function(stasis, "Stasis_CarrierUpdate"),
              function(stasis, "Stasis_CarrierSync")]
    parts += [stasis[stasis.index("#define STASIS_FALLBACK_RADIUS"):stasis.index("// Fills `rings`")]]
    parts += [function(stasis, name) for name in ("Stasis_GetProfile", "Stasis_ProfileFits", "Stasis_BuildCollision")]
    state_end = stasis.index("} StasisState;") + len("} StasisState;")
    parts += [re.search(r"^#define STASIS_MAX_OWN_COLLIDERS[^\n]*", stasis, re.M).group(0),
              stasis[stasis.rindex("typedef struct", 0, state_end):state_end], "static StasisState sStasis;",
              function(stasis, "Stasis_DropCollision")]
    for source, name in ((pacci, "PACCI_UH_MAX_HALF"), (sand, "SAND_SLAB_SCALE")):
        if name == "PACCI_UH_MAX_HALF":
            source = (ROOT / "soh/mods/actors/cane_pacci.h").read_text()
        parts += [re.search(r"^#define " + name + r"[^\n]*", source, re.M).group(0)]
    parts += [function(pacci, name) for name in ("Pacci_UhTooBig", "Pacci_UhDynaTopY")]
    parts += [function(magnet, name) for name in ("SwitchMagnet_DynaTopY", "SwitchMagnet_BodyBottom")]
    parts += [sand[sand.index("static f32 sSandTopOffset"):sand.index("static void WandSand_Measure")],
              function(sand, "WandSand_Measure")]
    parts += [pacci[pacci.index("#define PACCI_DMG_LIGHT"):pacci.index("// `heavy` picks the damage flavour:")],
              function(pacci, "Pacci_BurstSpawn")]
    return "\n".join(parts)


def native_cylinder():
    source = (ROOT / "soh/src/code/z_collision_check.c").read_text()
    names = ["Collider_InitBase", "Collider_InitTouch", "Collider_InitBump", "Collider_InitInfo",
             "Collider_InitCylinderDim", "Collider_InitCylinder", "Collider_SetBase", "Collider_SetTouch",
             "Collider_SetBump", "Collider_SetInfo", "Collider_SetCylinderDim", "Collider_SetCylinder"]
    bodies = [function(source, name) for name in names]
    declarations = "\n".join(body[:body.index("{")].strip() + ";" for body in bodies)
    return declarations + "\n" + "\n".join(bodies)


def bridge_adapters():
    gameplay = ROOT / "soh/soh/Enhancements/Gameplay"
    valley = (gameplay / "ComboFountainValley.cpp").read_text()
    snow = (gameplay / "ComboFountainSnowheadBridge.cpp").read_text()
    lake = (gameplay / "ComboLakeHyliaBridge.cpp").read_text()
    parts = ["namespace ValleyTest { using namespace ComboFountainValley;"]
    start = valley.index("std::array<Vec3i, 43 * 3>")
    end = valley.index("CollisionHeader sCollision{};", start) + len("CollisionHeader sCollision{};")
    parts += [valley[start:end]]
    parts += [function(valley, name) for name in ("SamePoint", "SceneContains", "Build")]
    parts += ["} namespace SnowTest { using namespace ComboFountainSnowhead;"]
    start = snow.index("const Mesh sMesh = BuildMesh();")
    end = snow.index("bool sBuilt = false;", start) + len("bool sBuilt = false;")
    parts += [snow[start:end], function(snow, "Build"), "} namespace LakeTest { namespace Lake = ComboLakeHylia;",
              function(lake, "Geometry")]
    start = lake.index("struct CaveCollision {")
    opening = lake.index("{", start)
    cursor, depth = opening + 1, 1
    while depth:
        depth += (lake[cursor] == "{") - (lake[cursor] == "}")
        cursor += 1
    parts += [lake[start:cursor] + ";", "}"]
    return "\n".join(parts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cases", nargs="*", choices=("shared", "sm64", "water", "grapple", "actors", "bridges", "motion", "timectl", "abi", "enemy"))
    args = parser.parse_args()
    cases = args.cases or ["shared", "sm64", "water", "grapple", "actors", "bridges", "motion", "timectl", "abi", "enemy"]
    env = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
    with tempfile.TemporaryDirectory(prefix="custom-unbound-collision-") as tmp:
        work = Path(tmp)
        if "shared" in cases:
            for host in ("soh", "mm"):
                native_names = ["BgActor_SetActor", "DynaPoly_SetBgActor", "DynaPoly_IsBgIdBgActor",
                                "DynaPoly_GetActor", "DynaPoly_DeleteBgActor", "DynaPolyActor_Init"]
                if host == "soh":
                    native_names += ["BgCheck_ReallocTable", "BgActor_Initialize", "ScaleRotPos_Initialize",
                                     "DynaLookup_Reset", "DynaLookup_ResetLists", "SSList_SetNull",
                                     "DynaLookup_ResetVtxStartIndex", "DynaPoly_GrowBgActorTable",
                                     "DynaPoly_IsBgIdInTable", "DynaPoly_IsBgIdAllocated"]
                native = work / (host + "-registration.c")
                obj, binary = work / (host + ".o"), work / host
                # Compile the consumer first: expanded-type errors belong to
                # the adapter, independently of native helper dependencies.
                consumer = work / (host + "-consumer.o")
                subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags(host),
                    *(["-DTEST_OOT"] if host == "soh" else []), "-c",
                    str(Path(__file__).with_name("shared_fixture_test.cpp")), "-o", str(consumer)], check=True)
                native.write_text(native_unit(host, native_names))
                subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags(host),
                                "-c", str(native), "-o", str(obj)], check=True)
                subprocess.run([os.environ.get("CXX", "c++"), *flags(host), "-no-pie", str(consumer), str(obj),
                                "-Wl,--gc-sections", "-o", str(binary)], check=True)
                subprocess.run([str(binary)], check=True, env=env)
        if "sm64" in cases:
            names = ["DynaPoly_IsBgIdBgActor", "DynaPoly_IsBgIdInTable", "BgCheck_GetCollisionHeader",
                     "SurfaceType_Get", "SurfaceType_GetSceneExitIndex", "SurfaceType_IsIgnoredByEntities"]
            native = work / "surfaces-native.c"
            binary = work / "surfaces"
            consumer = work / "surfaces.o"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"),
                "-Werror=incompatible-pointer-types", "-c", str(Path(__file__).with_name("sm64_surfaces_test.c")),
                "-o", str(consumer)], check=True)
            native.write_text(native_unit("soh", names) + native_oc_queue())
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"), "-no-pie",
                str(consumer), str(native), "-Wl,--gc-sections", "-lm",
                "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "water" in cases:
            source = (ROOT / "soh/mods/season_scene.cpp").read_text()
            begin = source.index("#define SEASON_WATER_GONE")
            end = source.index("typedef enum {", begin)
            production = source[begin:end]
            water = work / "water.cpp"
            water.write_text('''#include "functions.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
''' + production + '''
int main() {
    WaterBox boxes[3]{};
    boxes[0].ySurface = 80000; boxes[1].ySurface = -90000; boxes[2].ySurface = -315;
    boxes[0].room = -1; boxes[1].camera = 500;
    CollisionHeader header{}; header.numWaterBoxes = 3; header.waterBoxes = boxes;
    SeasonScene_SuppressWaterBoxes(&header);
    SeasonScene_SuppressWaterBoxes(&header);
    assert(boxes[0].ySurface == -32000 && boxes[1].ySurface == -32000);
    SeasonScene_RestoreWaterBoxes();
    assert(boxes[0].ySurface == 80000 && boxes[1].ySurface == -90000 && boxes[2].ySurface == -315);
    assert(boxes[0].room == -1 && boxes[1].camera == 500);
    SeasonScene_RestoreWaterBoxes();
    std::puts("PASS water: full-width original heights survive suppression/repeated cleanup");
}
''')
            binary = work / "water"
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-no-pie",
                            str(water), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "grapple" in cases:
            native = work / "grapple-native.c"
            names = ["DynaPoly_IsBgIdBgActor", "DynaPoly_IsBgIdInTable", "BgCheck_GetCollisionHeader",
                     "SurfaceType_Get", "SurfaceType_IsHookshotSurface", "CollisionPoly_GetNormalF"]
            binary = work / "grapple"
            consumer = work / "grapple.o"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"),
                "-Werror=incompatible-pointer-types", "-c", str(Path(__file__).with_name("grapple_test.c")),
                "-o", str(consumer)], check=True)
            native.write_text(native_unit("soh", names))
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"), "-no-pie",
                str(consumer), str(native), "-Wl,--gc-sections", "-lm",
                "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "actors" in cases:
            (work / "actor_adapters.inc").write_text(actor_adapters())
            native = work / "actor-registration.c"
            names = ["BgActor_SetActor", "DynaPoly_SetBgActor", "DynaPoly_IsBgIdBgActor", "DynaPoly_GetActor",
                     "DynaPoly_DeleteBgActor", "DynaPolyActor_Init", "BgCheck_ReallocTable", "BgActor_Initialize",
                     "ScaleRotPos_Initialize", "DynaLookup_Reset", "DynaLookup_ResetLists", "SSList_SetNull",
                     "DynaLookup_ResetVtxStartIndex", "DynaPoly_GrowBgActorTable", "DynaPoly_IsBgIdInTable",
                     "DynaPoly_IsBgIdAllocated"]
            native.write_text(native_unit("soh", names) + native_cylinder())
            binary = work / "actors"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"), "-no-pie", "-I" + str(work),
                str(Path(__file__).with_name("actors_test.c")), str(native), "-Wl,--gc-sections", "-lm",
                "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "bridges" in cases:
            (work / "bridge_adapters.inc").write_text(bridge_adapters())
            binary = work / "bridges"
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags("soh"), "-no-pie", "-I" + str(work),
                str(Path(__file__).with_name("bridges_test.cpp")), "-Wl,--gc-sections", "-lm",
                "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "motion" in cases:
            (work / "wide_motion.inc").write_text(wide_motion())
            binary = work / "motion"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"), "-no-pie", "-I" + str(work),
                str(Path(__file__).with_name("wide_motion_test.c")), "-Wl,--gc-sections", "-lm", "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True, env=env)
        if "timectl" in cases:
            native = work / "timectl-native.c"
            native.write_text(native_ac_queue())
            binary = work / "timectl"
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags("soh"), "-no-pie",
                str(Path(__file__).with_name("timectl_test.c")), str(native), "-Wl,--gc-sections", "-lm",
                "-o", str(binary)], check=True)
            for case in ("growth", "filters", "failure", "lifetime"):
                subprocess.run([str(binary), case], check=True, env=env)
    for case, script in (("abi", "run_adapter_abi_tests.py"), ("enemy", "run_enemy_randomizer_tests.py")):
        if case in cases:
            subprocess.run([sys.executable, "-B", str(Path(__file__).with_name(script))], check=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
