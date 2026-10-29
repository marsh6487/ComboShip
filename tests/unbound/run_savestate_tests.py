#!/usr/bin/env python3
"""Exercise external collision ownership and the production savestate load under ASan/UBSan."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/custom_unbound_collision"))
from run_tests import flags, native_queue_owner


def function(source, name):
    match = re.search(r"^[A-Za-z_][^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)(?:\s+noexcept)?\s*\{", source, re.M)
    if not match:
        raise AssertionError("Missing production function: " + name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def native_source():
    source = (ROOT / "soh/src/code/z_bgcheck.c").read_text()
    names = [
        "BgCheck_TableCapacityLimit", "BgCheck_NextTableCapacity", "BgCheck_ReallocTable", "BgCheck_IsSpotScene",
        "BgCheck_SetVanillaSubdivisions", "BgCheck_ScaleSubdivAxis", "BgCheck_ScaleSubdivisionsForPolyCount",
        "BgCheck_SetSubdivisionDimension", "BgCheck_Free", "BgCheck_Allocate", "BgCheck_InitEmpty",
        "DynaPoly_NullPolyList", "DynaPoly_NullVtxList", "DynaPoly_AllocPolyList", "DynaPoly_AllocVtxList",
        "ScaleRotPos_Initialize", "DynaLookup_ResetLists", "DynaLookup_Reset", "DynaLookup_ResetVtxStartIndex",
        "BgActor_Initialize", "DynaPoly_GrowBgActorTable", "DynaPoly_Init", "DynaPoly_Alloc", "SSList_SetNull",
        "SSNodeList_Initialize", "SSNodeList_Alloc", "SSNodeList_Grow", "SSNodeList_GetNextNodeIdx",
        "DynaSSNodeList_Initialize", "DynaSSNodeList_Alloc", "DynaSSNodeList_GetNextNodeIdx",
        "DynaPoly_EnsureListCapacity", "BgCheck_GetStaticLookup", "BgCheck_PosInStaticBoundingBox",
        "BgCheck_GetStaticLookupIndicesFromPos", "BgCheck_RaycastFloorStaticList",
    ]
    end = source.index("} BgCheckSceneSubdivisionEntry;") + len("} BgCheckSceneSubdivisionEntry;")
    subdivision_type = source[source.rindex("typedef struct {", 0, end):end]
    bodies = [function(source, name) for name in names]
    declarations = "\n".join(body[:body.index("{")].strip() + ";" for body in bodies)
    defines = "\n".join(re.findall(r"^#define (?:SS_NULL|UNBOUND_)[^\n]+", source, re.M))
    return '''#include "global.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#undef LOG_HUNGUP_THREAD
#define LOG_HUNGUP_THREAD() abort()
void Math_Vec3i_ToVec3f(Vec3f* out, Vec3i* in) { out->x=in->x;out->y=in->y;out->z=in->z; }
// Grid geometry is outside this lifecycle fixture; empty lookup initialization matches the zero-poly path.
u32 BgCheck_InitializeStaticLookup(CollisionContext* context, PlayState* play, StaticLookup* cells) {
    size_t count=(size_t)context->subdivAmount.x*context->subdivAmount.y*context->subdivAmount.z;
    for(size_t i=0;i<count;++i) cells[i].floor.head=cells[i].wall.head=cells[i].ceiling.head=UINT32_MAX;
    return 0;
}
s32 CollisionPoly_CheckYIntersect(CollisionPoly* poly,Vec3i* vertices,f32 x,f32 z,f32* y,f32 distance) { return 0; }
''' + defines + "\n" + subdivision_type + "\n" + declarations + "\n" + "\n".join(bodies) + native_queue_owner() + '''
int Fixture_GrowGeometry(CollisionContext* context) { return DynaPoly_EnsureListCapacity(NULL,&context->dyna); }
void Fixture_GrowTables(CollisionContext* context) {
    DynaPoly_GrowBgActorTable(NULL,&context->dyna,context->dyna.bgActorMax*2);
    context->polyNodes.count=context->polyNodes.max;
    SSNodeList_GetNextNodeIdx(&context->polyNodes);
    context->dyna.polyNodes.count=context->dyna.polyNodes.max;
    DynaSSNodeList_GetNextNodeIdx(&context->dyna.polyNodes);
}
int Fixture_Append(CollisionCheckContext* context,int kind,Collider* collider) {
    return CollisionCheck_AppendQueue(context,kind,collider);
}
'''


def cpp_fixture():
    source = (ROOT / "soh/soh/Enhancements/savestates.cpp").read_text()
    fixture = (ROOT / "tests/unbound/savestate_collision_fixture.cpp").read_text()
    start = source.index("struct QueueSnapshotDeleter")
    end = source.index("class SaveState {", start)
    fixture = fixture.replace("/* ACTUAL SAVESTATE DATA */", source[start:end])
    fixture = fixture.replace("/* ACTUAL LOAD */", function(source, "SaveState::Load"))
    fixture = fixture.replace("/* ACTUAL SAVE */", function(source, "SaveState::Save"))
    fixture = fixture.replace("/* ACTUAL TRANSITION RESTORE */", function(source, "SaveState::LoadTransitionActors"))
    fixture = fixture.replace("/* ACTUAL TRANSITION CAPTURE */", function(source, "SaveState::SaveTransitionActors"))
    scene = (ROOT / "soh/soh/unbound/SceneDB.cpp").read_text()
    bodies = "\n".join(function(scene, name) for name in ["SceneDB_CaptureState", "SceneDB_RestoreState"])
    return fixture.replace("/* ACTUAL SCENE FLAGS */", bodies)


def main():
    with tempfile.TemporaryDirectory(prefix="unbound-savestate-") as directory:
        work = Path(directory)
        native = work / "native.c"
        native.write_text(native_source())
        common = flags("soh") + ["-fno-omit-frame-pointer"]
        objects = []
        for source in [native, ROOT / "soh/src/code/bgcheck_ownership.c"]:
            obj = work / (source.stem + ".o")
            subprocess.run([os.environ.get("CC", "cc"), "-std=c11", *common, "-c", str(source), "-o", str(obj)], check=True)
            objects.append(str(obj))
        cpp = work / "fixture.cpp"
        cpp.write_text(cpp_fixture())
        binary = work / "savestate"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *common, str(cpp),
                        str(ROOT / "soh/soh/Enhancements/BgCheckStateSnapshot.cpp"), *objects,
                        "-no-pie", "-Wl,--gc-sections", "-Wl,--wrap=malloc", "-Wl,--wrap=realloc", "-Wl,--wrap=free",
                        "-Wl,--wrap=_Znwm", "-lm", "-o", str(binary)], check=True)
        # LSan cannot inspect /proc tasks in this environment. The fixture accounts for every external
        # malloc/realloc/free directly and requires zero remaining allocations after teardown.
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        print("PASS production savestate external collision/flags lifecycle, epochs and transactional OOM under ASan/UBSan")


if __name__ == "__main__":
    main()
